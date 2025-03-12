#pragma once
#include "editor.h"
#include "lexer.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <filesystem>
#include <string>
#include <unistd.h>
class file
{
public:
    file(editor &editor_ref, lexer &lexer_ref);
    bool createFile(std::string path);
    bool openFile(std::string path);
    bool saveFile(std::string path);
    bool isDirectory(std::string path);
    bool isFile(std::string path);
    std::vector<std::string> getFileContent(std::string path);
    std::vector<std::string> parseDirectory(std::string path);
    std::string getFilePrefix(std::string path);
    std::string getDirectoryPrefix(std::string path);
    std::string findMatch(const std::string &dirPath, const std::string &filePrefix);
    std::string fullPath;
    std::string directory;

private:
    std::string filename;
    editor &editor_ref;
    lexer &lexer_ref;
};