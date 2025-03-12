#include "lexer.h"
// Constructor
lexer::lexer()
    : pos(0), lineNumber(0) {}

// Destructor
lexer::~lexer() {}

// Peeks at the current character without advancing the position
char lexer::peek()
{
    if (pos < input.length())
    {
        return input[pos];
    }
    return '\0'; // Return null char if out of bounds
}
void lexer::setInput(std::string input)
{
    this->input = input;
}
// Advances the current position and returns the next character
char lexer::advance()
{

    if (pos < input.length())
    {
        return input[pos++];
    }
    return '\0';
}

// Skips over whitespace characters (spaces, tabs, newlines)
void lexer::skipWhitespace()
{

    while (isspace(peek()))
    {
        char currentChar = advance();

        // ugly hack but works
        if (currentChar != '\n' && currentChar != ' ')
        {
            continue;
        }

        if (currentChar == '\n')
        {
            lineNumber++;
            continue;
        }

        tokens.push_back(createToken(std::string(1, currentChar), TOKEN_WHITESPACE));
    }
}

lexer::Token lexer::createToken(std::string value, TokenType type)
{
    Token token;
    token.type = type;
    token.value = value;
    token.lineNumber = lineNumber;
    return token;
}
bool isIdentifierChar(char c)
{
    return isalnum(c) || c == '_';
}
bool isTypeKeyword(const std::string &str)
{
    return str == "int" || str == "float" || str == "string" || str == "char" || str == "double";
}

void lexer::updateInput(std::vector<std::string> input)
{
    this->tokens.clear();
    this->input.clear(); // Clear previous input
    this->lineNumber = 0;
    this->pos = 0;
    this->xPos = 0;
    for (size_t i = 0; i < input.size(); i++)
    {
        this->input += input[i]; // Append the line
        if (i != input.size() - 1)
        {
            this->input += '\n'; // Preserve newlines between lines
        }
    }
    // std::cout << this->input << std::endl;
}

void lexer::parse()
{

    while (pos < input.length())
    {
        skipWhitespace(); // Skip whitespace outside of strings

        char currentChar = peek();
        // Handle integers
        if (isdigit(currentChar))
        {
            std::string value;
            while (isdigit(peek()))
            {
                value += advance();
            }
            Token token = createToken(value, TOKEN_INT);
            tokens.push_back(token);
        }
        // Handle identifiers and keywords
        else if (isalpha(currentChar))
        {
            std::string value;
            while (isIdentifierChar(peek()))
            {
                value += advance();
            }

            // Check if its a function or variable
            TokenType type;
            if (peek() == '(')
            { // Check if the next character is an opening parenthesis
                type = TOKEN_FUNCTION;
                // advance(); // Skip the '('
            }
            else
            {
                type = TOKEN_VARIABLE;
            }

            Token token = createToken(value, type);
            tokens.push_back(token);
        }
        // Handle string literals
        else if (currentChar == '"')
        {
            try
            {
                Token tokenstr2 = createToken("\"", TOKEN_STRING);
                tokens.push_back(tokenstr2);
                std::string value;
                advance();
                while (peek() == '\n')
                {
                    lineNumber++;
                    advance();
                }
                while (peek() != '"' && peek() != '\0')
                {
                    value = advance();
                    Token tokenstr = createToken(value, TOKEN_STRING);
                    tokens.push_back(tokenstr);
                    while (peek() == '\n')
                    {

                        lineNumber++;
                        advance();
                    }
                }

                if (peek() == '"')
                {
                    Token tokenstrd = createToken("\"", TOKEN_STRING);
                    tokens.push_back(tokenstrd);
                    advance();
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << " at line " << lineNumber << std::endl;
                Token token = createToken("\"", TOKEN_UNKNOWN);
                tokens.push_back(token);
            }
        }
        // Handle parentheses
        else if (currentChar == '(')
        {
            advance();
            Token token = createToken("(", TOKEN_LPAREN);
            tokens.push_back(token);
        }
        else if (currentChar == ')')
        {
            advance();
            Token token = createToken(")", TOKEN_RPAREN);
            tokens.push_back(token);
        }
        else if (currentChar == '/' && peek() == '/')
        {
            std::string value;
            value += advance();

            while (peek() != '\n' && peek() != '\0')
            {
                value += advance(); // Collect characters until the end of the comment
            }

            Token token = createToken(value, TOKEN_COMMENT);
            tokens.push_back(token);
        }

        // Handle other symbols, errors, etc.
        else
        {
            std::string value(1, advance()); // Record the unknown character
            Token token = createToken(value, TOKEN_UNKNOWN);
            tokens.push_back(token);
        }
    }
    // Optionally create a final EOF token
    Token eofToken = createToken("", TOKEN_EOF);
    tokens.push_back(eofToken);
}

std::vector<lexer::Token> lexer::getTokens() const
{
    return tokens;
}
void lexer::printTokens()
{
    for (const auto &token : tokens)
    {
        std::string typeStr;
        switch (token.type)
        {
        case TOKEN_TYPE:
            typeStr = "TYPE";
            break;
        case TOKEN_STRING:
            typeStr = "STRING";
            break;
        case TOKEN_INT:
            typeStr = "INT";
            break;
        case TOKEN_VARIABLE:
            typeStr = "VARIABLE";
            break;
        case TOKEN_LPAREN:
            typeStr = "LPAREN";
            break;
        case TOKEN_WHITESPACE:
            typeStr = "WHITESPACE";
            break;
        case TOKEN_RPAREN:
            typeStr = "RPAREN";
            break;
        case TOKEN_FUNCTION:
            typeStr = "FUNCTION";
            break;
        case TOKEN_COMMENT:
            typeStr = "COMMENT";
            break;
        case TOKEN_EOF:
            typeStr = "EOF";
            break;
        }
        std::cout << "Token: " << typeStr << " " << token.value << " at line " << token.lineNumber << std::endl;
    }
}
