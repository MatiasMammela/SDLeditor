#include "lsp.h"
using json = nlohmann::json;
lsp::lsp()
{
    int inPipe[2], outPipe[2];
    pipe(inPipe);
    pipe(outPipe);

    pid = fork();
    if (pid == 0)
    {
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        close(inPipe[1]);
        close(outPipe[0]);
        execlp("clangd", "clangd", "--log=error", "--compile-commands-dir=/home/masa/sdlEditor/build", "--query-driver=/usr/bin/clang", "--background-index", NULL);

        exit(1);
    }

    close(inPipe[0]);
    close(outPipe[1]);

    lspin = fdopen(inPipe[1], "w");
    lspout = fdopen(outPipe[0], "r");
    setbuf(lspout, NULL);

    sendRequest(R"({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
          "processId": null,
          "rootUri": "file:///home/masa/sdlEditor/",
          "capabilities": {}
        }
      })");

    waitForInitialization();
    sendInitialized();
}

void lsp::sendRequest(const std::string &request)
{
    std::string header = "Content-Length: " + std::to_string(request.length()) + "\r\n\r\n";
    fputs((header + request).c_str(), lspin);
    fflush(lspin);
}
void lsp::waitForInitialization()
{
    while (true)
    {
        std::string response = readResponse();
        if (response.find("\"capabilities\"") != std::string::npos)
        {
            break;
        }
    }
}

void lsp::didOpen(const std::string &filePath, const std::vector<std::string> &fileContent)
{
    std::string uri = "file://" + filePath;

    // Convert vector to a single string
    std::string content;
    for (const auto &line : fileContent)
    {
        content += line + "\n";
    }

    // Construct the JSON object
    json requestJson = {
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didOpen"},
        {"params", {{"textDocument", {{"uri", uri}, {"languageId", "cpp"}, {"version", 1}, {"text", content}}}}}};

    // Serialize JSON to string
    std::string request = requestJson.dump();

    sendRequest(request);
}
LSPDefinitionResult lsp::goToDefinition(const std::string &filePath, int line, int character)
{
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 3},
        {"method", "textDocument/definition"},
        {"params", {{"textDocument", {{"uri", "file://" + filePath}}}, {"position", {{"line", line}, {"character", character}}}}}};

    sendRequest(request.dump());

    int maxRetries = 3;
    int delayMs = 100; // Start with 100ms delay, will double each retry

    for (int attempt = 1; attempt <= maxRetries; ++attempt)
    {
        auto futureResponse = std::async(std::launch::async, [&]()
                                         { return readResponse(); });

        // Wait for the response with a timeout
        if (futureResponse.wait_for(std::chrono::milliseconds(500)) == std::future_status::ready)
        {
            std::string response = futureResponse.get();
            if (!response.empty())
            {
                json jsonResponse;
                try
                {
                    jsonResponse = json::parse(response);
                    if (jsonResponse.contains("result") && !jsonResponse["result"].empty())
                    {
                        // Extract the first location
                        auto location = jsonResponse["result"][0];
                        std::string uri = location["uri"];
                        int targetLine = location["range"]["start"]["line"];
                        int targetCharacter = location["range"]["start"]["character"];

                        std::string targetFile = uri.substr(7); // Remove "file://"
                        std::cout << "Jumping to: " << targetFile << " at (" << targetLine << ", " << targetCharacter << ")" << std::endl;

                        return LSPDefinitionResult{targetFile, targetLine, targetCharacter};
                    }
                }
                catch (const nlohmann::json::parse_error &e)
                {
                    std::cerr << "JSON parse error: " << e.what() << std::endl;
                }
            }
        }

        std::cerr << "Attempt " << attempt << " failed, retrying in " << delayMs << "ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        delayMs *= 2; // Increase delay for next retry
    }

    std::cerr << "Definition not found after retries!" << std::endl;
    return LSPDefinitionResult{"", -1, -1};
}

void lsp::sendInitialized()
{
    sendRequest(R"({
        "jsonrpc": "2.0",
        "method": "initialized",
        "params": {}
    })");
}

std::string lsp::readResponse()
{
    std::string buffer;
    char temp[256];

    int fd = fileno(lspout); // fileno returns the file descriptor
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN; // Wait for data to read

    const int timeout_ms = 1000; // 3 seconds timeout
    int elapsed_time = 0;
    const int chunk_timeout = 100; // Check every 100ms

    while (elapsed_time < timeout_ms)
    {
        int ret = poll(&pfd, 1, chunk_timeout);
        if (ret == 0)
        {
            elapsed_time += chunk_timeout; // Update elapsed time
            continue;                      // Keep waiting until timeout
        }
        else if (ret < 0)
        {
            std::cerr << "Poll error\n";
            break;
        }

        // Data is available, read it
        ssize_t bytesRead = read(fd, temp, sizeof(temp) - 1);
        if (bytesRead <= 0)
        {
            break;
        }

        temp[bytesRead] = '\0';
        buffer += temp; // Append to the response buffer

        // Check for "Content-Length"
        size_t contentPos = buffer.find("Content-Length: ");
        if (contentPos == std::string::npos)
        {
            continue;
        }
        size_t endPos = buffer.find("\r\n\r\n", contentPos);
        if (endPos == std::string::npos)
        {
            continue;
        }

        // Extract content length
        int contentLength = std::stoi(buffer.substr(contentPos + 16, endPos - (contentPos + 16)));
        size_t messageStart = endPos + 4;

        if (buffer.size() >= messageStart + contentLength)
        {
            return buffer.substr(messageStart, contentLength); // Return complete response
        }
    }

    std::cerr << "Timeout reached while reading response.\n";
    return "";
}
