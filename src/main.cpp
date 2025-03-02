#include "editor.h"
#include "input.h"
int main(void)
{
    SDL_Event event;
    editor editor;
    input input(editor);

    while (SDL_WaitEvent(&event) && editor.running == true)
    {
        input.handleInput(event);
        editor.render();
    }
    // for (auto line : editor.lines)
    // {
    //     std::cout << line << std ::endl;
    // }
}