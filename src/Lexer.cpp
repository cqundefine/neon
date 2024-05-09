#include <Lexer.h>

TokenStream CreateTokenStream(uint32_t fileID)
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 39, "Not all tokens are handled in Lexer::NextToken()");

    std::vector<Token> tokens;
    size_t index = 0;
    auto file = g_context->files.at(fileID).content;

    while (true)
    {
        if (index >= file.size())
        {
            tokens.push_back({ .type = TokenType::Eof, .location = { fileID, index } });
            break;
        }

        bool reachedEndOfLine = false;
        while (file[index] == ' ' || file[index] == '\t' || file[index] == '\n' || file[index] == '\r')
        {
            index++;
            if (index >= file.size())
            {
                tokens.push_back({ .type = TokenType::Eof, .location = { fileID, index } });
                reachedEndOfLine = true;
                break;
            }
        }
        if (reachedEndOfLine)
            break;

        if (file[index] == '(')
        {
            index++;
            tokens.push_back({ .type = TokenType::LParen, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == ')')
        {
            index++;
            tokens.push_back({ .type = TokenType::RParen, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '{')
        {
            index++;
            tokens.push_back({ .type = TokenType::LCurly, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '}')
        {
            index++;
            tokens.push_back({ .type = TokenType::RCurly, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '[')
        {
            index++;
            tokens.push_back({ .type = TokenType::LSquareBracket, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == ']')
        {
            index++;
            tokens.push_back({ .type = TokenType::RSquareBracket, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == ':')
        {
            index++;
            tokens.push_back({ .type = TokenType::Colon, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == ';')
        {
            index++;
            tokens.push_back({ .type = TokenType::Semicolon, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '+')
        {
            index++;
            tokens.push_back({ .type = TokenType::Plus, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '-')
        {
            index++;
            tokens.push_back({ .type = TokenType::Minus, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '*')
        {
            index++;
            tokens.push_back({ .type = TokenType::Asterisk, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '/')
        {
            index++;
            if (file[index] == '/')
            {
                index++;
                while (file[index] != '\n' && file[index] != '\r' && index < file.size())
                    index++;
                continue;
            }
            else if (file[index] == '*')
            {
                index++;
                while (!(file[index] == '*' && file[index + 1] == '/') && index + 1 < file.size())
                    index++;
                if (index < file.size())
                    index++;
                if (index < file.size())
                    index++;
                continue;
            }
            tokens.push_back({ .type = TokenType::Slash, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == ',')
        {
            index++;
            tokens.push_back({ .type = TokenType::Comma, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '=')
        {
            index++;
            if (file[index] == '=')
            {
                index++;
                tokens.push_back({ .type = TokenType::DoubleEquals, .location = { fileID, index - 2 } });
                continue;
            }

            tokens.push_back({ .type = TokenType::Equals, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '>')
        {
            index++;
            if (file[index] == '=')
            {
                index++;
                tokens.push_back({ .type = TokenType::GreaterThanOrEqual, .location = { fileID, index - 2 } });
                continue;
            }
            tokens.push_back({ .type = TokenType::GreaterThan, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '<')
        {
            index++;
            if (file[index] == '=')
            {
                index++;
                tokens.push_back({ .type = TokenType::LessThanOrEqual, .location = { fileID, index - 2 } });
                continue;
            }
            tokens.push_back({ .type = TokenType::LessThan, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '!')
        {
            index++;
            if (file[index] == '=')
            {
                index++;
                tokens.push_back({ .type = TokenType::NotEqual, .location = { fileID, index - 2 } });
                continue;
            }
            tokens.push_back({ .type = TokenType::ExclamationMark, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '.')
        {
            index++;
            tokens.push_back({ .type = TokenType::Dot, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '#')
        {
            index++;
            tokens.push_back({ .type = TokenType::Hash, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '&')
        {
            index++;
            tokens.push_back({ .type = TokenType::Ampersand, .location = { fileID, index - 1 } });
            continue;
        }
        else if (file[index] == '"')
        {
            uint32_t trueBeginIndex = index;
            std::string value;
            index++;
            while (file[index] != '"')
            {
                if (index >= file.size())
                    g_context->Error({ fileID, index }, "Unexpected end of file");

                char c = file[index];
                if (c == '\\')
                {
                    index++;
                    if (file[index] == 'n')
                    {
                        value += 10;
                    }
                    else if (file[index] == '\\')
                    {
                        value += '\\';
                    }
                    else
                    {
                        g_context->Error({ fileID, index }, "Unknown escape char: %c", file[index]);
                    }
                }
                else
                {
                    value += c;
                }
                index++;
            }
            index++;

            tokens.push_back({ .type = TokenType::StringLiteral,
                .stringValue = value,
                .location = { fileID, trueBeginIndex } });
            continue;
        }

        if (isdigit(file[index]))
        {
            uint32_t trueBeginIndex = index;
            std::string numberString;
            int base = 10;
            if (file[index] == '0')
            {
                index++;
                if (file[index] == 'x')
                {
                    index++;
                    base = 16;
                }
                else if (file[index] == 'b')
                {
                    index++;
                    base = 2;
                }
                else if (file[index] == 'o')
                {
                    index++;
                    base = 8;
                }
                else
                {
                    index--;
                }

                if (base != 10 && index >= file.size())
                {
                    g_context->Error({ fileID, index }, "Unexpected end of file");
                }
            }
            do
            {
                numberString += file[index];
                index++;
            } while (isdigit(file[index]));

            tokens.push_back({ .type = TokenType::Number,
                .intValue = std::strtoull(numberString.c_str(), 0, base),
                .location = { fileID, trueBeginIndex } });
            continue;
        }
        else if (isalpha(file[index]) || file[index] == '_')
        {
            uint32_t trueBeginIndex = index;
            std::string value;
            do
            {
                value += file[index];
                index++;
            } while (isalnum(file[index]) || file[index] == '_');

            if (value == "function")
            {
                tokens.push_back({ .type = TokenType::Function, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "return")
            {
                tokens.push_back({ .type = TokenType::Return, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "if")
            {
                tokens.push_back({ .type = TokenType::If, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "extern")
            {
                tokens.push_back({ .type = TokenType::Extern, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "while")
            {
                tokens.push_back({ .type = TokenType::While, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "include")
            {
                tokens.push_back({ .type = TokenType::Include, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "struct")
            {
                tokens.push_back({ .type = TokenType::Struct, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "var")
            {
                tokens.push_back({ .type = TokenType::Var, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "const")
            {
                tokens.push_back({ .type = TokenType::Const, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "endif")
            {
                tokens.push_back({ .type = TokenType::Endif, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else if (value == "to")
            {
                tokens.push_back({ .type = TokenType::To, .location = { fileID, trueBeginIndex } });
                continue;
            }
            else
                tokens.push_back({ .type = TokenType::Identifier,
                    .stringValue = value,
                    .location = { fileID, trueBeginIndex } });
            continue;
        }

        g_context->Error({ fileID, index }, "Unexpected symbol: `%c`", file[index]);
    }

    return TokenStream(tokens);
}
