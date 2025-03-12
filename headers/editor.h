#pragma once
#include "SDL.h"
#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <algorithm>
#include "lexer.h"
class topBar
{
public:
    topBar(SDL_Renderer *renderer, TTF_Font *font, int windowWidth);
    void render(SDL_Renderer *renderer, int cursorX, int cursorY, int scrollX, int scrollY);
    void updateWindowWidth(int newWidth);
    std::string keys;
    std::string filename;
    char differ;

private:
    SDL_Rect barRect;
    SDL_Renderer *renderer;
    TTF_Font *font;

    int windowWidth;
};

class ido
{
public:
    ido(SDL_Renderer *renderer, TTF_Font *font, int windowWidth, int windowHeight);
    void render(SDL_Renderer *renderer);
    void updateWindowSize(int newWidth, int newHeight);
    std::string buffer;

private:
    SDL_Rect barRect;
    int barHeight;
    SDL_Renderer *renderer;

    TTF_Font *font;
    int windowWidth;
    int windowHeight;
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
    void addChar(char c);
    void removeChar();
    void deleteChar();
    void paste();
    bool checkForScroll(int x, int y);
    void newLine();
    void copy();
    void moveSelection();
    void cursorMoveWordRight();
    void cursorMoveWordLeft();
    void renderToken(std::string text, int x, int y, SDL_Color color);
    void renderHighlight();
    void tab();
    void reloadFont(uint8_t size);
    bool matchBracket(char inputChar);
    editor(lexer &lexer_ref);
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
    ido *idoBar;
    topBar *bar;
    bool idoActive;
    ::cursor cursor;

private:
    int height;
    int width;
    SDL_Window *window;
    SDL_Renderer *renderer;
    lexer &lexer_ref;
    TTF_Font *font;
};
