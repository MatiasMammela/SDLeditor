#include "file.h"

namespace fs = std::filesystem;

file::file(editor &editor_ref, lexer &lexer_ref) : editor_ref(editor_ref), lexer_ref(lexer_ref)
{
}

bool file::isFile(std::string path)
{
    if (fs::is_regular_file(path))
    {
        return true;
    }
    return false;
}

bool file::isDirectory(std::string path)
{
    if (fs::is_directory(path))
    {
        return true;
    }
    return false;
}

std::string file::getDirectoryPrefix(std::string path)
{
    fs::path pathObj = path;
    std::string prefix = pathObj.parent_path();
    return prefix;
}

std::vector<std::string> file::parseDirectory(std::string path)
{
    std::vector<std::string> result;
    if (isDirectory(path))
    {
        fs::path pathObj = path;

        for (auto const &entry : fs::directory_iterator(pathObj))
        {
            result.push_back(entry.path().filename().string());
        }
    }
    return result;
}

std::string file::getFilePrefix(std::string path)
{
    fs::path pathObj = path;
    std::string prefix = pathObj.filename();
    return prefix;
}
std::string file::findMatch(const std::string &dirPath, const std::string &filePrefix)
{
    std::vector<std::string> currentDirectory = this->parseDirectory(dirPath);
    std::string bestMatch;
    bool foundMatch = false;

    for (const std::string &file : currentDirectory)
    {
        fs::path filePath(file);
        std::string fileName = filePath.filename().string();

        if (fileName.find(filePrefix) == 0)
        {
            if (!foundMatch)
            {
                bestMatch = fileName;
                foundMatch = true;
            }
            else
            {
                size_t commonLength = 0;
                while (commonLength < bestMatch.size() &&
                       commonLength < fileName.size() &&
                       bestMatch[commonLength] == fileName[commonLength])
                {
                    commonLength++;
                }
                bestMatch = bestMatch.substr(0, commonLength);
            }
        }
    }
    return bestMatch;
}
bool file::saveFile(std::string path)
{
    std::ofstream stream(path);

    if (!stream.is_open()) // Check if the file could be opened for writing
    {
        std::cerr << "Error: Could not open file for writing: " << path << std::endl;
        return false;
    }

    if (isFile(path))
    {
        for (const std::string &line : editor_ref.lines)
        {
            stream << line << "\n";
        }
        editor_ref.bar->differ = ' ';
        stream.close();
        return true;
    }
    else
    {
        std::cerr << "Error: File does not exist or is not accessible: " << path << std::endl;
        stream.close();
        return false;
    }
}

std::vector<std::string> file::getFileContent(std::string path)
{
    std::vector<std::string> result;

    if (access(path.c_str(), R_OK) != 0)
    {
        std::cerr << "Permission denied: " << path << std::endl;
        return result;
    }

    std::ifstream stream(path);
    if (!stream)
    {
        std::cerr << "Failed to open: " << path << std::endl;
        return result;
    }

    std::ostringstream buffer; // Use ostringstream instead of stringstream
    std::string line;
    while (getline(stream, line))
    {
        result.push_back(line);
        buffer << line << '\n';
    }

    if (result.empty())
    {
        std::cerr << "File is empty or unreadable: " << path << std::endl;
        return result;
    }

    lexer_ref.setInput(buffer.str()); // Set lexer input only if file is valid
    return result;
}

bool file::openFile(std::string path)
{
    if (!isFile(path))
    {
        std::cerr << "Not a valid file: " << path << std::endl;
        return false;
    }

    std::vector<std::string> content = getFileContent(path);
    if (content.empty()) // Prevent crashing if file read fails
    {
        std::cerr << "Error reading file: " << path << std::endl;
        return false;
    }

    editor_ref.lines = std::move(content); // Use move to optimize memory
    lexer_ref.parse();

    fs::path pathObj = path;
    this->fullPath = fs::absolute(pathObj);
    this->filename = pathObj.filename();
    this->directory = pathObj.parent_path();
    editor_ref.bar->filename = this->filename;

    return !fullPath.empty() && !filename.empty();
}
