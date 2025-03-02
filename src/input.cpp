#include "input.h"
#include "editor.h"
input::input(editor &editor_ref) : editor_ref(editor_ref) {};

void input::handleInput(SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        editor_ref.running = false;
        break;

    case SDL_TEXTINPUT:
        if (!(SDL_GetModState() & KMOD_ALT))
        {
            editor_ref.addChar(event.text.text[0]);
        }
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_BACKSPACE)
        {
            editor_ref.removeChar();
            break;
        }
        else if (event.key.keysym.sym == SDLK_RETURN)
        {
            editor_ref.newLine();
            break;
        }
        else if (event.key.keysym.mod & KMOD_CTRL)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_b:
                editor_ref.cursorMoveLeft();
                break;
            case SDLK_f:
                editor_ref.cursorMoveRight();
                break;
            case SDLK_p:
                editor_ref.cursorMoveUp();
                break;
            case SDLK_n:
                editor_ref.cursorMoveDown();
                break;
            case SDLK_a:
                editor_ref.cursorMoveSOL();
                break;
            case SDLK_e:
                editor_ref.cursorMoveEOL();
                break;
            case SDLK_o:
                editor_ref.openLine();
                break;
            case SDLK_d:
                editor_ref.deleteChar();
                break;
            case SDLK_k:
                editor_ref.cutLineFromX();
                break;
            case SDLK_v:
                editor_ref.paste();
                break;
            case SDLK_c:
                editor_ref.copy();
                break;
            case SDLK_g:
                editor_ref.selecting = 0;
                break;
            }
        }
        else if (event.key.keysym.mod & KMOD_ALT)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_f:
                editor_ref.cursorMoveWordRight();
                break;
            case SDLK_b:
                editor_ref.cursorMoveWordLeft();
                break;
            }
        }
        break;

    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT)
        {

            int adjustedY = event.button.y - editor_ref.offsetY;
            if (adjustedY < 0)
                return;

            int adjustedX = event.button.x - editor_ref.offsetX;
            if (adjustedX < 0)
                adjustedX = 0;

            int x = adjustedX / editor_ref.charWidth;
            int y = adjustedY / editor_ref.charHeight;

            if (editor_ref.selecting)
            {
                editor_ref.selecting = false;
            }
            else
            {
                editor_ref.selecting = true;
                editor_ref.selectStartX = editor_ref.selectEndX = x + editor_ref.scrollX;
                editor_ref.selectStartY = editor_ref.selectEndY = y + editor_ref.scrollY;
            }
            // std::cout << "Starting selection from X " << editor_ref.selectStartX << std::endl;

            // std::cout << "Starting selection from Y " << editor_ref.selectStartY << std::endl;

            editor_ref.cursorSet(x, y);
        }
        break;
    case SDL_MOUSEMOTION:
        if (editor_ref.selecting)
        {
            int adjustedY = event.motion.y - editor_ref.offsetY;
            int adjustedX = event.motion.x - editor_ref.offsetX;

            int x = std::max(0, adjustedX / editor_ref.charWidth);
            int y = std::max(0, adjustedY / editor_ref.charHeight);

            editor_ref.selectEndX = x + editor_ref.scrollX;
            editor_ref.selectEndY = y + editor_ref.scrollY;
            // std::cout << " Ending selection on X" << editor_ref.selectEndX << std::endl;
            // std::cout << " Ending selection on Y" << editor_ref.selectEndY << std::endl;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        break;
    }
}