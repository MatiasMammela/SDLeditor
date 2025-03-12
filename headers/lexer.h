// Old lexer. Not originally written for this project but works for this use case
// Not the best implementation.

#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class lexer
{
public:
    lexer();
    ~lexer();

    enum TokenType
    {
        TOKEN_TYPE,
        TOKEN_STRING,
        TOKEN_INT,
        TOKEN_VARIABLE,
        TOKEN_LPAREN,
        TOKEN_RPAREN,
        TOKEN_LBRA,
        TOKEN_RBRA,
        TOKEN_COMMENT,
        TOKEN_FUNCTION,
        TOKEN_WHITESPACE,
        TOKEN_UNKNOWN,
        TOKEN_EOF // End of file
    };
    struct Token
    {
        TokenType type;
        std::string value;
        int lineNumber;
    };
    std::vector<Token> tokens;
    Token createToken(std::string input, TokenType type);
    void parse();
    char peek();
    char advance();
    void skipWhitespace();
    void printTokens();
    std::vector<Token> getTokens() const;
    void updateInput(std::vector<std::string> input);
    void setInput(std::string input);
    int lineNumber = 0;

private:
    std::string input;
    size_t pos = 0; // Position in the input
    int xPos = 0;
};
