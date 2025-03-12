#include "input.h"
#include "editor.h"
#include "file.h"
#include "lsp.h"
input::input(editor &editor_ref, file &file_ref, lsp &lsp_ref) : editor_ref(editor_ref), file_ref(file_ref), lsp_ref(lsp_ref) {};

void input::handleInput(SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        editor_ref.running = false;
        break;

    case SDL_TEXTINPUT:
    {
        char inputChar = event.text.text[0];

        // Prevent Alt+F and Alt+B from triggering unwanted input
        if (!((inputChar == 'f' || inputChar == 'b') && (event.key.keysym.mod & KMOD_ALT)))
        {
            editor_ref.bar->differ = '*';

            if (!editor_ref.matchBracket(inputChar))
            {
                editor_ref.addChar(inputChar);
            }
        }
        break;
    }
    case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL)
        {
            ctrlActive = false;
            keySequence.clear();
            editor_ref.bar->keys = "";
        }
        else if (event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT)
        {
            altActive = false;
            keySequence.clear();
            editor_ref.bar->keys = "";
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
            ctrlActive = true;

            // Store key sequence
            keySequence.push_back(event.key.keysym.sym);

            // Process sequences (e.g., Ctrl+X, F)
            processKeySequence();
            break;
        }
        else if (event.key.keysym.mod & KMOD_ALT)
        {
            altActive = true;
            keySequence.push_back(event.key.keysym.sym);
            processKeySequence();
            break;
        }
        else if (event.key.keysym.sym == SDLK_TAB)
        {
            editor_ref.tab();
            break;
        }
        switch (event.key.keysym.sym)
        {
        case SDLK_LEFT:
            editor_ref.cursorMoveLeft();
            break;
        case SDLK_RIGHT:
            editor_ref.cursorMoveRight();
            break;
        case SDLK_UP:
            editor_ref.cursorMoveUp();
            break;
        case SDLK_DOWN:
            editor_ref.cursorMoveDown();
            break;
        }

    case SDL_MOUSEBUTTONDOWN:

        if (event.button.button == SDL_BUTTON_LEFT)
        {
            // Calculate the precise points
            int adjustedY = event.button.y - editor_ref.offsetY;
            if (adjustedY < 0)
                return;

            int adjustedX = event.button.x - editor_ref.offsetX;
            if (adjustedX < 0)
                adjustedX = 0;

            int x = adjustedX / editor_ref.charWidth;
            int y = adjustedY / editor_ref.charHeight;

            // jmp to definition
            if (SDL_GetModState() & KMOD_CTRL)
            {
                std::cout << "Y " << y << "X " << x << std::endl;
                LSPDefinitionResult result = lsp_ref.goToDefinition(file_ref.fullPath, y, x);
                if (result.line != -1 && result.character != -1)
                {

                    std::ifstream file_check(result.uri);
                    if (!file_check.good())
                    {
                        std::cerr << "Error: File not found -> " << result.uri << std::endl;
                        return; // Prevents crash
                    }

                    file_check.close();

                    file_ref.openFile(result.uri);
                    lsp_ref.didOpen(file_ref.fullPath, editor_ref.lines);
                    editor_ref.cursorSet(result.character, result.line);
                }
                return;
            }

            // selecting
            if (editor_ref.selecting)
            {
                editor_ref.selecting = false;
            }
            else
            {

                editor_ref.selectStartX = editor_ref.selectEndX = x + editor_ref.scrollX;
                editor_ref.selectStartY = editor_ref.selectEndY = y + editor_ref.scrollY;
                editor_ref.selecting = true;
            }

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
        }
        break;
    case SDL_MOUSEBUTTONUP:
        editor_ref.selecting = false;
        break;
    case SDL_MOUSEWHEEL:
        if (SDL_GetModState() & KMOD_CTRL)
        {
            int value = editor_ref.charHeight;
            int scrollChange = event.wheel.y;
            int scrollFactor = 2;
            value += scrollChange * scrollFactor;
            value = std::max(5, std::min(value, 50));
            editor_ref.reloadFont(value);
        }
        break;
    }
}
void input::processKeySequence()
{

    if (keySequence.empty())
        return;

    std::string str = "";
    for (auto key : keySequence)
    {
        str += SDL_GetKeyName(key);
        str += " ";
    }
    editor_ref.bar->keys = str;
    // Handle Ctrl keybindings
    if (ctrlActive && keySequence.size() == 2)
    {

        switch (keySequence[1])
        {
        case SDLK_b:
            editor_ref.cursorMoveLeft();
            editor_ref.moveSelection();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_b), keySequence.end());
            break;
        case SDLK_f:
            editor_ref.cursorMoveRight();
            editor_ref.moveSelection();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_f), keySequence.end());
            break;
        case SDLK_p:
            editor_ref.cursorMoveUp();
            editor_ref.moveSelection();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_p), keySequence.end());
            break;
        case SDLK_n:
            editor_ref.cursorMoveDown();
            editor_ref.moveSelection();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_n), keySequence.end());
            break;
        case SDLK_a:
            editor_ref.cursorMoveSOL();
            editor_ref.moveSelection();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_a), keySequence.end());
            break;
        case SDLK_e:
            editor_ref.cursorMoveEOL();
            editor_ref.moveSelection();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_e), keySequence.end());
            break;
        case SDLK_o:
            editor_ref.openLine();
            editor_ref.moveSelection();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_o), keySequence.end());
            break;
        case SDLK_d:
            editor_ref.deleteChar();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_d), keySequence.end());
            break;
        case SDLK_k:
            editor_ref.cutLineFromX();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_k), keySequence.end());
            break;
        case SDLK_v:
            editor_ref.paste();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_v), keySequence.end());
            break;
        case SDLK_c:
            editor_ref.copy();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_c), keySequence.end());
            break;
        case SDLK_g:
            editor_ref.selecting = false;
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_g), keySequence.end());
            break;
        case SDLK_SPACE:
            editor_ref.selecting = true;
            editor_ref.selectStartX = editor_ref.cursor.x;
            editor_ref.selectStartY = editor_ref.cursor.y;
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_SPACE), keySequence.end());
            break;
        case SDLK_s:
            file_ref.saveFile(file_ref.fullPath);
            keySequence.clear();
            break;
        }
    }

    // Handle Alt keybindings
    else if (altActive && keySequence.size() == 2)
    {
        switch (keySequence[1])
        {
        case SDLK_f:
            editor_ref.cursorMoveWordRight();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_f), keySequence.end());
            editor_ref.moveSelection();
            break;
        case SDLK_b:
            editor_ref.cursorMoveWordLeft();
            keySequence.erase(std::remove(keySequence.begin(), keySequence.end(), SDLK_b), keySequence.end());
            editor_ref.moveSelection();
            break;
        }
    }

    // Multi-key sequences (eg Ctrl+X S)
    else if (ctrlActive && keySequence.size() == 3)
    {

        if (keySequence[1] == SDLK_x)
        {
            switch (keySequence[2])
            {
            case SDLK_s:
                break;
            case SDLK_f:
                keySequence.clear();
                writeToIdo();
                editor_ref.idoBar->buffer = "";
                break;
            }
        }
    }
}

void input::autoCompleteBuffer(std::string &buffer)
{
    std::string bestMatch = file_ref.findMatch(file_ref.getDirectoryPrefix(buffer), file_ref.getFilePrefix(buffer));
    if (bestMatch != "")
    {
        size_t lastSlash = buffer.find_last_of('/', buffer.size() - 2);
        if (lastSlash != std::string::npos)
        {
            buffer.erase(lastSlash + 1);
        }
        buffer += bestMatch;
        if (file_ref.isDirectory(buffer))
        {
            buffer += "/";
        }
    }
    editor_ref.idoBar->buffer = buffer;
    editor_ref.renderHighlight();
}

void input::evaluateIdoBuffer(std::string buffer)
{

    if (file_ref.isFile(buffer))
    {
        file_ref.openFile(buffer);
        lsp_ref.didOpen(file_ref.fullPath, editor_ref.lines);
    }
}
void input::writeToIdo()
{

    std::string buffer;

    SDL_Event event;

    editor_ref.idoBar->buffer.clear(); // Clear existing buffer
    editor_ref.idoActive = true;       // Enable IDO mode
    if (file_ref.fullPath != "")
    {
        buffer = file_ref.fullPath;
        editor_ref.idoBar->buffer = buffer;
        editor_ref.renderHighlight();
    }

    while (editor_ref.idoActive) // Stay in loop until Enter or Ctrl+G
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                editor_ref.running = false;
                return;
            }
            else if (event.type == SDL_TEXTINPUT)
            {
                buffer += event.text.text; // Add typed characters to buffer
                editor_ref.idoBar->buffer = buffer;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_RETURN)
                {
                    evaluateIdoBuffer(buffer);
                    editor_ref.idoActive = false;
                    return;
                }
                else if ((event.key.keysym.mod & KMOD_CTRL) && event.key.keysym.sym == SDLK_g)
                {
                    editor_ref.idoBar->buffer.clear(); // Clear buffer on cancel
                    editor_ref.idoActive = false;
                    return;
                }
                else if (event.key.keysym.sym == SDLK_BACKSPACE && !buffer.empty())
                {
                    if (buffer.back() == '/') // Only remove up to last '/' if last char is '/'
                    {
                        size_t lastSlash = buffer.find_last_of('/', buffer.size() - 2);
                        if (lastSlash != std::string::npos)
                        {
                            buffer.erase(lastSlash + 1); // Keep the last '/'
                        }
                        else
                        {
                            buffer.clear(); // No more '/', clear everything
                        }
                    }
                    else
                    {
                        buffer.pop_back(); // Default behavior, remove last character
                    }

                    editor_ref.idoBar->buffer = buffer;
                }
                else if (event.key.keysym.sym == SDLK_TAB)
                {
                    autoCompleteBuffer(buffer);
                }
            }
        }

        editor_ref.renderHighlight(); // Refresh screen to show IDO input
    }
}
