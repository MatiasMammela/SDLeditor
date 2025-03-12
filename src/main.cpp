#include "editor.h"
#include "input.h"
#include "file.h"
#include "lsp.h"
#include "lexer.h"
int main(void)
{
    SDL_Event event;
    lsp lsp;
    lexer lex;
    editor editor(lex);
    file file(editor, lex);
    input input(editor, file, lsp);
    file.openFile("/home/masa/sdlEditor/resources/welcome.txt");
    while (SDL_WaitEvent(&event) && editor.running == true)
    {

        input.handleInput(event);
        // editor.render();
        lex.updateInput(editor.lines);
        lex.parse();
        editor.renderHighlight();
    }
}



