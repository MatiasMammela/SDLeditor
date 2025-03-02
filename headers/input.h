#pragma once

#include <SDL2/SDL.h>

class editor;

class input
{
public:
    void handleInput(SDL_Event &event);
    input(editor &editor_ref);

private:
    editor &editor_ref;
};