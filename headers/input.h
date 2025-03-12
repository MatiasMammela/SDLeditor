#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>
class editor;
class file;
class lsp;
class input
{
public:
    void handleInput(SDL_Event &event);
    input(editor &editor_ref, file &file_ref, lsp &lsp_ref);
    std::vector<SDL_Keycode> keySequence;
    bool ctrlActive = false;
    bool altActive = false;
    void processKeySequence();
    void writeToIdo();
    void evaluateIdoBuffer(std::string buffer);
    void autoCompleteBuffer(std::string &buffer);
    std::string findMatch(std::string dirPath, std::string filePrefix);

private:
    editor &editor_ref;
    file &file_ref;
    lsp &lsp_ref;
};