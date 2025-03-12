#pragma once
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <future>
#include <chrono>
#include <thread>
struct LSPDefinitionResult
{
    std::string uri;
    int line;
    int character;
};
class lsp
{

public:
    lsp();
    std::string readResponse();
    void sendRequest(const std::string &request);
    void sendInitialized();
    void waitForInitialization();
    void didOpen(const std::string &filePath, const std::vector<std::string> &fileContent);
    LSPDefinitionResult goToDefinition(const std::string &filePath, int line, int character);

private:
    FILE *lspin;
    FILE *lspout;
    pid_t pid;
};