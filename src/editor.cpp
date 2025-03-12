#include "editor.h"

ido::ido(SDL_Renderer *renderer, TTF_Font *font, int windowWidth, int windowHeight)
    : renderer(renderer), font(font), windowWidth(windowWidth)
{
    barHeight = 30;
    barRect = {0, windowHeight - barHeight, windowWidth, barHeight};
}

void ido::updateWindowSize(int newWidth, int newHeight)
{
    windowWidth = newWidth;
    barRect.w = windowWidth;
    barRect.y = newHeight - barHeight;
}

void ido::render(SDL_Renderer *renderer)
{
    if (!buffer.empty())
    {
        SDL_Color textColor = {255, 255, 255, 255};

        SDL_Surface *textSurface = TTF_RenderText_Solid(font, buffer.c_str(), textColor);
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

        SDL_Rect textRect = {50, barRect.y, textSurface->w, textSurface->h};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }
}
bool editor::matchBracket(char inputChar)
{
    if (inputChar == '(')
    {
        this->addChar('(');
        this->addChar(')');
        this->cursorMoveLeft();
        return true;
    }
    else if (inputChar == '[')
    {
        this->addChar('[');
        this->addChar(']');
        this->cursorMoveLeft();
        return true;
    }
    else if (inputChar == '{')
    {
        this->addChar('{');
        this->addChar('}');
        this->cursorMoveLeft();
        return true;
    }
    return false;
}
topBar::topBar(SDL_Renderer *renderer, TTF_Font *font, int windowWidth) : renderer(renderer), font(font), windowWidth(windowWidth)
{
    this->differ = ' ';
    barRect = {0, 0, windowWidth, 30};
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

    std::string positionText = " " + filename + differ + " COL " + std::to_string(cursorX + scrollX) +
                               ", ROW " + std::to_string(cursorY + scrollY) + " " + this->keys;
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, positionText.c_str(), textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_Rect textRect = {50, 0, textSurface->w, textSurface->h}; // Position inside the top bar
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

editor::editor(lexer &lexer_ref) : lexer_ref(lexer_ref)
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

    this->font = TTF_OpenFont("resources/Iosevka-Regular.ttc", 20);
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
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    bar = new topBar(renderer, font, windowWidth);
    idoBar = new ido(renderer, font, windowWidth, windowHeight);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_StartTextInput();
    this->running = true;
    lines.push_back("");
    this->charWidth = 10;
    this->charHeight = 24;
    this->cursor.x = 0;
    this->cursor.y = 0;
    this->scrollX = 0;
    this->scrollY = 0;
    this->offsetX = 50;
    this->offsetY = 50;
    this->height = 0;
    this->width = 0;
    this->idoActive = false;
    this->selecting = false;
}

// I use 4 spaces for tabs
void editor::tab()
{
    std::string &line = lines[cursor.y + scrollY];

    // Insert 4 spaces at the cursor position
    line.insert(cursor.x + scrollX, "    ");

    for (int i = 0; i < 4; i++)
    {
        cursorMoveRight(); // Move cursor right 4 times
    }
}

void editor::moveSelection()
{
    if (selecting)
    {
        selectEndX = cursor.x;
        selectEndY = cursor.y;
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
    {
        return;
    }

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
    static bool stopOnce = false; // Persistent across calls

    // Stop once at the end of the line before continuing to the next line
    if (cursor.x + scrollX >= len)
    {
        if (!stopOnce)
        {
            stopOnce = true; // Stop movement once
            return;
        }
        else
        {
            stopOnce = false; // Allow movement on the next call
            if (cursor.y + scrollY + 1 < lines.size())
            {
                cursor.x = 0;
                scrollX = 0;
                cursorMoveDown();
                return;
            }
        }
    }
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
}

void editor::cursorMoveUp()
{
    if (cursor.y + scrollY == 0)
        return; // Prevent moving beyond the first line

    if (cursor.y > 5 || scrollY == 0) // Move freely if not at top OR fully scrolled up
    {
        cursor.y--;
    }
    else if (scrollY > 0) // Start scrolling when within 5 lines of the top
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

void editor::cursorMoveDown()
{
    if (cursor.y + scrollY + 1 >= lines.size())
        return; // Prevent moving beyond last line

    if (cursor.y < height - 5 || scrollY + height >= lines.size()) // Move freely if not at bottom OR fully scrolled down
    {
        cursor.y++;
    }
    else if (scrollY + height < lines.size()) // Start scrolling when within 5 lines of the bottom
    {
        scrollY++;
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
    if (lines.empty())
    {
        lines.push_back(" ");
    }
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
    if (x >= width)
    {
        // Calculate how much the cursor exceeds the width
        scrollX += x - width + 1; // Add 1 to keep it within the bounds
        x = width - 1;            // Set cursor to the farthest right
    }
    // If the y coordinate exceeds the height
    if (y >= height)
    {
        // Calculate how much the cursor exceeds the height
        scrollY += y - height + 1; // Add 1 to keep it within the bounds
        y = height - 1;            // Set cursor to the farthest bottom
    }

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
        std::string &line = lines[cursor.y + scrollY];

        int tabSize = 4;
        if (cursor.x + scrollX >= tabSize &&
            line.substr(cursor.x + scrollX - tabSize, tabSize) == "    ")
        {
            line.erase(cursor.x + scrollX - tabSize, tabSize);
            for (int i = 0; i < tabSize; i++)
                cursorMoveLeft();
        }
        else
        {
            line.erase(cursor.x + scrollX - 1, 1);
            cursorMoveLeft();
        }
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
void editor::renderText(std::string text, int x, int y, SDL_Color color)
{

    if (text.empty())
    {
        // std::cout << "Empty " << std::endl;
        return;
    }

    // Apply horizontal scrolling (clip text)
    // if (scrollX < text.size())
    // {
    //     text = text.substr(scrollX, std::min(text.size() - scrollX, (size_t)(width)));
    // }
    // else
    // {
    //     text = ""; // Dont render anything if scrollX is beyond text length
    // }

    // Apply horizontal clipping (if scrollX is beyond text width, dont render)
    int textWidth = text.size() * charWidth;  // Width of the entire text
    int visibleTextWidth = width * charWidth; // Max visible text width in the window

    // If the text is off to the left of the window, skip rendering
    if (x + textWidth <= scrollX * charWidth)
    {
        return;
    }

    // clip the text to the right if its off the right of the window
    if (x >= (scrollX + visibleTextWidth) * charWidth)
    {
        return;
    }

    // Apply vertical clipping (check if the texts Y position is within the visible range)
    if (y < scrollY * charHeight || y >= (scrollY + height) * charHeight)
    {
        return; // Dont render the text if its out of the visible vertical range
    }

    // Offset Y position based on scrollY
    int adjustedY = y - (scrollY * charHeight);
    int adjustedX = x - (scrollX * charWidth);
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!textSurface)
    {
        // printf("Failed to render text: %s\n", text.c_str());
        return;
    }

    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {adjustedX + offsetX, adjustedY + offsetY, textSurface->w, textSurface->h};

    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void editor::render()
{
    int windowWidth, windowHeight, paddingX, paddingY;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    bar->updateWindowWidth(windowWidth);
    idoBar->updateWindowSize(windowWidth, windowHeight);
    paddingX = 20;
    paddingY = 80;
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
    idoBar->render(renderer);
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

        // Render text
        renderText(lines[i].c_str(), 0, i * charHeight, textColor);
    }

    cursor.render(renderer, charWidth, charHeight, this->offsetX, this->offsetY);
    SDL_RenderPresent(renderer);
}

void editor::renderHighlight()
{
    int windowWidth, windowHeight, paddingX, paddingY;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    bar->updateWindowWidth(windowWidth);
    idoBar->updateWindowSize(windowWidth, windowHeight);
    paddingX = 20;
    paddingY = 80;
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
    idoBar->render(renderer);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color highlightColor = {50, 50, 200, 255};

    for (int i = scrollY; i < scrollY + height + 1 && i < lines.size(); i++)
    {

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

        int posX = 0; // X position for rendering tokens

        // Iterate over tokens for the current line
        for (const auto &token : lexer_ref.tokens)
        {
            if (token.lineNumber == i)
            {
                SDL_Color color;
                switch (token.type)
                {
                case lexer::TOKEN_TYPE:
                    color = {255, 0, 0, 255};
                    break;
                case lexer::TOKEN_STRING:
                    color = {0, 255, 0, 255};
                    break;
                case lexer::TOKEN_INT:
                    color = {0, 0, 255, 255};
                    break;
                case lexer::TOKEN_COMMENT:
                    color = {128, 128, 128, 255};
                    break;
                case lexer::TOKEN_VARIABLE:
                    color = {255, 255, 255, 255};
                    break;
                case lexer::TOKEN_LPAREN:
                    color = {255, 255, 0, 255};
                    break;
                case lexer::TOKEN_RPAREN:
                    color = {255, 255, 0, 255};
                    break;
                case lexer::TOKEN_LBRA:
                    color = {0, 255, 255, 255};
                    break;
                case lexer::TOKEN_RBRA:
                    color = {0, 255, 255, 255};
                    break;
                case lexer::TOKEN_FUNCTION:
                    color = {255, 105, 180, 255};
                    break;
                default:
                    color = {255, 255, 255, 255};
                    break;
                }

                renderText(token.value, posX, i * charHeight, color);
                posX += token.value.size() * charWidth;
            }
        }
    }

    cursor.render(renderer, charWidth, charHeight, this->offsetX, this->offsetY);
    SDL_RenderPresent(renderer);
}
void editor::reloadFont(uint8_t size)
{
    this->charHeight = size;
    this->charWidth = size / 2;
    // This causes a massive memory leak
    // Closing the fonts with TTF_closefont is fucked and cant be used.
    // Maybe cache the different size fonts to minimize the memory usage.

    TTF_Font *tmpFont = TTF_OpenFont("resources/Iosevka-Regular.ttc", this->charHeight);

    if (tmpFont == NULL)
    {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        exit(1);
        return;
    }

    this->font = tmpFont;

    std::cout << "Font reloaded successfully with size: " << this->charHeight << std::endl;
}

editor::~editor()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
}
