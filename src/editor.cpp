#include "editor.h"
extern "C"
{
    TSLanguage *tree_sitter_c();
}
topBar::topBar(SDL_Renderer *renderer, TTF_Font *font, int windowWidth) : renderer(renderer), font(font), windowWidth(windowWidth)
{
    barRect = {0, 0, windowWidth, 30}; // Top bar height is 30 pixels
}
void topBar::updateWindowWidth(int newWidth)
{
    windowWidth = newWidth;
    barRect.w = windowWidth;
}
void topBar::render(SDL_Renderer *renderer, int cursorX, int cursorY, int scrollX, int scrollY)
{
    // Set background color of the top bar
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &barRect);

    // Create text displaying my cursor pos
    std::string positionText = "COL " + std::to_string(cursorX + scrollX) + ", ROW " + std::to_string(cursorY + scrollY) + " :  " + " cursor.x " + std::to_string(cursorX) + " cursor.y " + std::to_string(cursorY);

    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, positionText.c_str(), textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_Rect textRect = {50, 0, textSurface->w, textSurface->h}; // Position inside the top bar
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}
editor::editor()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        exit(1);
    }

    if (TTF_Init() < 0)
    {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
        exit(1);
    }

    this->font = TTF_OpenFont("resources/Iosevka-Regular.ttc", 24);
    if (font == NULL)
    {
        std::cerr << "Failed to open font: " << TTF_GetError() << std::endl;
        exit(1);
    }

    this->window = SDL_CreateWindow("editor",
                                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    0, 0,
                                    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (!this->window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        exit(1);
    }
    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED);
    if (!this->renderer)
    {
        std::cout << "Renderer failed to initialize" << std::endl;
    }

    bar = new topBar(renderer, font, 100);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_StartTextInput();
    this->running = true;
    lines.push_back("");
    this->charWidth = 12;
    this->charHeight = 24;
    this->cursor.x = 0;
    this->cursor.y = 0;
    this->scrollX = 0;
    this->scrollY = 0;
    this->offsetX = 50;
    this->offsetY = 50;
    this->height = 0;
    this->width = 0;
    this->selecting = false;
}
void editor::cursorMoveUp()
{
    if (cursor.y + scrollY == 0)
        return;
    if (scrollY <= 0)
    {
        cursor.y--;
    }
    if (scrollY > 0)
    {
        scrollY--;
    }

    // Adjust cursor.x if the new line is shorter
    if (cursor.x + scrollX > lines[cursor.y + scrollY].size())
    {
        cursor.x = std::min(cursor.x, (int)lines[cursor.y + scrollY].size());
        scrollX = 0;
    }
}
void editor::paste()
{
    const char *clipboardText = SDL_GetClipboardText();
    if (!clipboardText || clipboardText[0] == '\0')
    {
        std::cout << "Nothing to paste" << std::endl;
        return;
    }

    int y = cursor.y + scrollY;
    int x = cursor.x + scrollX;

    if (y >= lines.size())
        return;

    std::string text = clipboardText;
    std::vector<std::string> linesToInsert;
    size_t pos = 0, found;

    // Split clipboard text by newlines
    while ((found = text.find('\n', pos)) != std::string::npos)
    {
        linesToInsert.push_back(text.substr(pos, found - pos));
        pos = found + 1;
    }
    linesToInsert.push_back(text.substr(pos)); // Add last part

    if (linesToInsert.empty())
        return;

    std::string &currentLine = lines[y];

    // Insert the first part at cursor position
    std::string remainingText = currentLine.substr(x);
    currentLine = currentLine.substr(0, x) + linesToInsert[0];

    // Insert new lines
    for (size_t i = 1; i < linesToInsert.size(); ++i)
    {
        y++;
        if (y < lines.size())
        {
            lines.insert(lines.begin() + y, linesToInsert[i]);
        }
        else
        {
            lines.push_back(linesToInsert[i]);
        }
    }

    // Append remaining text from original line to last inserted line
    lines[y] += remainingText;

    // Move cursor to end of last inserted line
    cursor.x = lines[y].size();
    cursor.y = y - scrollY;
}
void editor::cursorMoveWordLeft()
{
    std::vector<std::string> tokens = {",", ".", "::", "->"};

    if (cursor.x + scrollX == 0 && cursor.y + scrollY > 0)
    {
        cursorMoveUp();
        cursor.x = lines[cursor.y + scrollY].size();
        scrollX = 0;
        return;
    }

    std::string &line = lines[cursor.y + scrollY];

    for (const std::string &token : tokens)
    {
        int tokenLen = token.length();
        if (cursor.x + scrollX >= tokenLen &&
            line.substr(cursor.x + scrollX - tokenLen, tokenLen) == token)
        {
            for (int i = 0; i < tokenLen; i++)
            {
                if (cursor.x == 0 && scrollX > 0)
                    scrollX--;
                else
                    cursor.x--;
            }
            return;
        }
    }

    while (cursor.x + scrollX > 0 && std::isspace(line[cursor.x + scrollX - 1]))
    {
        if (cursor.x == 0 && scrollX > 0)
            scrollX--;
        else
            cursor.x--;
    }
    while (cursor.x + scrollX > 0 && !std::isspace(line[cursor.x + scrollX - 1]))
    {
        for (const std::string &token : tokens)
        {
            int tokenLen = token.length();
            if (cursor.x + scrollX >= tokenLen &&
                line.substr(cursor.x + scrollX - tokenLen, tokenLen) == token)
            {
                return;
            }
        }

        if (cursor.x == 0 && scrollX > 0)
            scrollX--;
        else
            cursor.x--;
    }
}

void editor::cursorMoveWordRight()
{
    std::vector<std::string> tokens = {",", ".", "::", "->"};
    std::string &line = lines[cursor.y + scrollY];
    int len = line.size();

    // Check if we are at a token and move past it first
    for (const std::string &token : tokens)
    {
        int tokenLen = token.length();
        if (cursor.x + scrollX + tokenLen <= len &&
            line.substr(cursor.x + scrollX, tokenLen) == token)
        {
            for (int i = 0; i < tokenLen; i++)
            {
                cursor.x++;
                if (cursor.x >= width) // Handle scrolling
                {
                    scrollX++;
                    cursor.x = width - 1;
                }
            }
            return; // Stop after moving past a token
        }
    }

    // Move past any spaces
    while (cursor.x + scrollX < len && std::isspace(line[cursor.x + scrollX]))
    {
        cursor.x++;
        if (cursor.x >= width) // Handle scrolling
        {
            scrollX++;
            cursor.x = width - 1;
        }
    }

    // Move to the next word boundary, stopping at tokens
    while (cursor.x + scrollX < len && !std::isspace(line[cursor.x + scrollX]))
    {
        for (const std::string &token : tokens)
        {
            int tokenLen = token.length();
            if (cursor.x + scrollX + tokenLen <= len &&
                line.substr(cursor.x + scrollX, tokenLen) == token)
            {
                return; // Stop moving once we hit a token
            }
        }

        cursor.x++;
        if (cursor.x >= width) // Handle scrolling
        {
            scrollX++;
            cursor.x = width - 1;
        }
    }

    // If we reach the end of the line, move to the next line
    if (cursor.x + scrollX >= len && cursor.y + scrollY + 1 < lines.size())
    {
        cursor.x = 0;
        scrollX = 0;
        cursorMoveDown();
    }
}

void editor::cursorMoveDown()
{
    if (cursor.y + scrollY + 1 >= lines.size())
        return; // Prevent moving beyond last line

    if (cursor.y >= height)
    {
        scrollY++;
    }
    else
    {
        cursor.y++;
    }

    // Adjust cursor.x if the new line is shorter
    if (cursor.x + scrollX > lines[cursor.y + scrollY].size())
    {
        cursor.x = std::min(cursor.x, (int)lines[cursor.y + scrollY].size());
        scrollX = 0;
    }
}

void editor::cursorMoveRight()
{
    size_t lineLength = lines[cursor.y + scrollY].size();

    if (cursor.x + scrollX >= lineLength)
    {
        cursorMoveDown();
        return;
    }

    if (cursor.x >= width)
    {
        scrollX++;
    }
    else
    {
        cursor.x++;
    }
}

void editor::cutLineFromX()
{
    std::string &currentLine = lines[cursor.y + scrollY];
    std::string cutLineText = currentLine.substr(cursor.x + scrollX);
    currentLine.erase(cursor.x + scrollX);
    SDL_SetClipboardText(cutLineText.c_str());
    const char *clipboardText = SDL_GetClipboardText();
    if (clipboardText && cursor.x + scrollX != 0)
    {
        return;
    }
    // If there are lines below, delete them and move the lines up
    if (cursor.y + scrollY + 1 < lines.size() && cursor.x + scrollX == 0)
    {
        currentLine += lines[cursor.y + scrollY + 1];        // Append next line
        lines.erase(lines.begin() + cursor.y + scrollY + 1); // Remove the duplicate line
    }
    // If cursor is at the end of the line, merge with the next one
    else if (cursor.y + scrollY + 1 < lines.size() && cursor.x + scrollX >= currentLine.size())
    {
        currentLine += lines[cursor.y + scrollY + 1];
        lines.erase(lines.begin() + cursor.y + scrollY + 1);
    }
    // If the current line is empty and its not the first line, move the cursor up
    if (currentLine.empty() && cursor.y + scrollY > 0)
    {
        std::cout << "Moving up" << std::endl;
        lines.erase(lines.begin() + cursor.y + scrollY);

        cursorMoveUp();
        cursor.x = 0;
        scrollX = 0;
    }
}

void editor::cursorMoveLeft()
{
    if (cursor.x == 0 && scrollX > 0)
    {
        scrollX--;
    }
    else if (cursor.x > 0)
    {
        cursor.x--;
    }
    else
    {
        cursorMoveUp();
    }
}

void editor::cursorMoveSOL()
{
    cursor.x = 0;
    scrollX = 0;
}

void editor::cursorMoveEOL()
{
    int lineLength = lines[cursor.y + scrollY].size();

    if (lineLength <= width)
    {
        // If the whole line fits on screen, set cursor to end
        cursor.x = lineLength;
        scrollX = 0;
    }
    else
    {
        // Scroll so that the last part of the line is visible
        scrollX = std::max(0, lineLength - width);
        cursor.x = lineLength - scrollX;
    }
}

void editor::addChar(char c)
{

    lines[cursor.y + scrollY].insert(cursor.x + scrollX, 1, c);
    cursorMoveRight();
}
void editor::newLine()
{

    std::string &currentLine = lines[cursor.y + scrollY];
    std::string newLineText = currentLine.substr(cursor.x + scrollX);

    currentLine.erase(cursor.x + scrollX);
    lines.insert(lines.begin() + cursor.y + scrollY + 1, newLineText);

    cursorMoveDown();
    cursor.x = 0;
}

void editor::cursorSet(int x, int y)
{
    y += scrollY;
    x += scrollX;

    if (y < 0)
    {
        y = 0;
    }
    else if (y >= lines.size())
    {
        y = lines.size() - 1;
    }

    if (lines[y].empty()) // Prevent accessing out of range in an empty line
    {
        x = 0;
    }
    else if (x < 0)
    {
        x = 0;
    }
    else if (x > lines[y].size())
    {
        x = lines[y].size();
    }

    cursor.x = x - scrollX; // Keep cursor relative to the visible portion
    cursor.y = y - scrollY;
    // std::cout << "Setting cursor to X " << cursor.x << " Y " << cursor.y << std::endl;
}

void editor::openLine()
{
    int y = cursor.y + scrollY;
    int x = cursor.x + scrollX;

    if (y < 0 || y >= lines.size())
    {
        return;
    }

    std::string left = lines[y].substr(0, x);
    std::string right = lines[y].substr(x);
    lines[y] = left;
    lines.insert(lines.begin() + y + 1, right);
}

void editor::deleteChar()
{
    if (cursor.y + scrollY >= lines.size())
    {
        return;
    }

    if (cursor.x + scrollX < lines[cursor.y + scrollY].size())
    {
        lines[cursor.y + scrollY].erase(cursor.x + scrollX, 1);
    }
    else if (cursor.x + scrollX == lines[cursor.y + scrollY].size() && cursor.y + scrollY + 1 < lines.size())
    {
        lines[cursor.y + scrollY] += lines[cursor.y + scrollY + 1];
        lines.erase(lines.begin() + cursor.y + scrollY + 1);
    }
}

void editor::removeChar()
{
    if (cursor.x + scrollX > 0)
    {
        lines[cursor.y + scrollY].erase(cursor.x + scrollX - 1, 1);
        cursorMoveLeft();
    }
    else if (cursor.y + scrollY > 0)
    {
        std::string currentLine = lines[cursor.y + scrollY];

        cursorMoveUp();

        cursor.x = lines[cursor.y + scrollY].size();
        lines[cursor.y + scrollY] += currentLine;
        lines.erase(lines.begin() + cursor.y + scrollY + 1);
        int lineSize = lines[cursor.y + scrollY].size();
        if (lineSize > width)
        {
            scrollX = lineSize - width;
            cursor.x = width;
        }
        else
        {
            scrollX = 0;
        }
    }
}

void cursor::render(SDL_Renderer *renderer, int charWidth, int charHeight, int offsetX, int offsetY)
{
    int adjustmentX = 0;
    int adjustmentY = 2;

    SDL_Rect cursorRect = {(x * charWidth) + (offsetX + adjustmentX), (y * charHeight) + (offsetY + adjustmentY), charWidth, charHeight};

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderFillRect(renderer, &cursorRect);
}

void editor::renderText(std::string text, int x, int y, SDL_Color color)
{

    if (text.empty())
        return;

    // Apply horizontal scrolling (clip text)
    if (scrollX < text.size())
    {
        text = text.substr(scrollX, std::min(text.size() - scrollX, (size_t)(width)));
    }
    else
    {
        text = ""; // Dont render anything if scrollX is beyond text length
    }

    // Offset Y position based on scrollY
    int adjustedY = y - (scrollY * charHeight);

    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!textSurface)
    {
        // printf("Failed to render text: %s\n", text.c_str());
        return;
    }

    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {x + offsetX, adjustedY + offsetY, textSurface->w, textSurface->h};

    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}
void editor::copy()
{
    if (!selecting)
    {
        std::cout << "Nothing selected" << std::endl;
        return;
    }

    std::string copiedText;

    for (int i = selectStartY; i <= selectEndY; i++)
    {
        // Ensure we dont go out of bounds
        if (i < 0 || i >= lines.size())
            continue;

        int startX = (i == selectStartY) ? selectStartX : 0;
        int endX = (i == selectEndY) ? selectEndX : lines[i].size();

        if (startX > endX)
            std::swap(startX, endX);

        if (startX < 0)
            startX = 0;
        if (endX > (int)lines[i].size())
            endX = lines[i].size();

        if (startX < endX) // Ensure theres something to copy
        {
            if (i > selectStartY) // Add newline when copying multiple lines
                copiedText += "\n";

            copiedText += lines[i].substr(startX, endX - startX);
        }
    }

    if (!copiedText.empty())
    {
        SDL_SetClipboardText(copiedText.c_str());
        std::cout << "Copied: " << copiedText << std::endl;
    }
}

void editor::render()
{
    int windowWidth, windowHeight, paddingX, paddingY;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    bar->updateWindowWidth(windowWidth);
    paddingX = 20;
    paddingY = 40;
    this->height = (windowHeight - offsetY - paddingY) / charHeight;
    this->width = (windowWidth - offsetX - paddingX) / charWidth;

    if (cursor.x > width)
    {
        cursor.x = this->width;
        scrollX = 0;
    }
    if (cursor.y > height)
    {
        cursor.y = this->height;
        scrollY = 0;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    bar->render(renderer, cursor.x, cursor.y, scrollX, scrollY);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color highlightColor = {50, 50, 200, 255};

    // Loop through visible lines and render
    for (int i = scrollY; i < scrollY + height + 1 && i < lines.size(); i++)
    {
        std::string line = lines[i];

        // Check if this line contains a selection
        if (selecting && i >= selectStartY && i <= selectEndY)
        {
            int relativeYLine = i - scrollY;
            int relativeYPos = relativeYLine * charHeight + offsetY;
            int relativeXCol = selectStartX - scrollX;
            int relativeXWidth = ((selectEndX - scrollX)) - relativeXCol;
            // std::cout << "Line " << relativeYLine << " has selection " << std::endl;
            // std::cout << "Starting From column" << relativeXCol << std::endl;

            SDL_Rect selectionRect = {(relativeXCol * charWidth) + offsetX, relativeYPos, relativeXWidth * charWidth, charHeight};
            SDL_SetRenderDrawColor(renderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
            SDL_RenderFillRect(renderer, &selectionRect);
        }

        // Render text after highlight
        renderText(lines[i].c_str(), 0, i * charHeight, textColor);
    }

    cursor.render(renderer, charWidth, charHeight, this->offsetX, this->offsetY);
    SDL_RenderPresent(renderer);
}

editor::~editor()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
}