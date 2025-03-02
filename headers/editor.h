
#pragma once
#include "SDL.h"
#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <algorithm>
#include <tree_sitter/api.h>

class topBar
{
public:
    topBar(SDL_Renderer *renderer, TTF_Font *font, int windowWidth);
    void render(SDL_Renderer *renderer, int cursorX, int cursorY, int scrollX, int scrollY);
    void updateWindowWidth(int newWidth);

private:
    SDL_Rect barRect;
    SDL_Renderer *renderer;
    TTF_Font *font;
    int windowWidth;
};

class cursor
{
public:
    void render(SDL_Renderer *renderer, int charWidth, int charHeight, int offsetX, int offsetY);
    int x;
    int y;
    cursor() : x(0), y(0) {}
};

class editor
{

public:
    void cursorMoveLeft();
    void cursorMoveRight();
    void cursorMoveUp();
    void cursorSet(int x, int y);
    void cursorMoveEOL();
    void cutLineFromX();
    void cursorMoveSOL();
    void openLine();
    void cursorMoveDown();
    void renderText(std::string text, int x, int y, SDL_Color color);
    void renderNode(TSNode node, int &x, int lineIndex, uint32_t &lastEnd);
    void addChar(char c);
    void removeChar();
    void deleteChar();
    void paste();
    bool checkForScroll(int x, int y);
    void newLine();
    void copy();
    void cursorMoveWordRight();
    void cursorMoveWordLeft();
    // std::string clipboard;
    editor();
    ~editor();
    void render();
    void run();
    bool running = false;
    std::vector<std::string> lines;
    int charWidth;
    int scrollY;
    int scrollX;
    int charHeight;
    int offsetY;
    int offsetX;
    bool selecting = false;
    int selectStartX, selectStartY, selectEndX, selectEndY;

private:
    int height;
    int width;
    SDL_Window *window;
    SDL_Renderer *renderer;
    ::cursor cursor;
    topBar *bar;
    TTF_Font *font;
};