#include <Lexer.h>
#include <span>

TokenStream CreateTokenStream(uint32_t fileID)
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 39, "Not all tokens are handled in Lexer::NextToken()");

    std::vector<Token> tokens;
    size_t index = 0;
    std::span<char> file = g_context->files.at(fileID).content;

    auto get_current_char = [&]() {
        if (index >= file.size())
            g_context->Error({ fileID, index }, "Unexpected end of file");
        return file[index];
    };

    auto get_next_char = [&]() {
        char c = get_current_char();
        index++;
        return c;
    };

    while (true)
    {
        while (index < file.size() && (get_current_char() == ' ' || get_current_char() == '\t' || get_current_char() == '\n' || get_current_char() == '\r'))
            index++;

        if (index >= file.size())
            break;

        char c = get_next_char();

        switch (c)
        {
            case '(':
                tokens.push_back({ .type = TokenType::LParen, .location = { fileID, index - 1 } });
                break;
            case ')':
                tokens.push_back({ .type = TokenType::RParen, .location = { fileID, index - 1 } });
                break;
            case '{':
                tokens.push_back({ .type = TokenType::LCurly, .location = { fileID, index - 1 } });
                break;
            case '}':
                tokens.push_back({ .type = TokenType::RCurly, .location = { fileID, index - 1 } });
                break;
            case '[':
                tokens.push_back({ .type = TokenType::LSquareBracket, .location = { fileID, index - 1 } });
                break;
            case ']':
                tokens.push_back({ .type = TokenType::RSquareBracket, .location = { fileID, index - 1 } });
                break;
            case ':':
                tokens.push_back({ .type = TokenType::Colon, .location = { fileID, index - 1 } });
                break;
            case ';':
                tokens.push_back({ .type = TokenType::Semicolon, .location = { fileID, index - 1 } });
                break;
            case '+':
                tokens.push_back({ .type = TokenType::Plus, .location = { fileID, index - 1 } });
                break;
            case '-':
                tokens.push_back({ .type = TokenType::Minus, .location = { fileID, index - 1 } });
                break;
            case '*':
                tokens.push_back({ .type = TokenType::Asterisk, .location = { fileID, index - 1 } });
                break;
            case '/': {
                char c = get_next_char();
                if (c == '/')
                {
                    while (index < file.size() && get_current_char() != '\n' && get_current_char() != '\r')
                        index++;
                }
                else if (c == '*')
                {
                    while (!(get_current_char() == '*' && file[index + 1] == '/') && index + 1 < file.size())
                        index++;
                    if (index < file.size())
                        index++;
                    if (index < file.size())
                        index++;
                }
                else
                {
                    index--;
                    tokens.push_back({ .type = TokenType::Slash, .location = { fileID, index - 1 } });
                }
                break;
            }
            case ',':
                tokens.push_back({ .type = TokenType::Comma, .location = { fileID, index - 1 } });
                break;
            case '=':
                if (get_current_char() == '=')
                {
                    index++;
                    tokens.push_back({ .type = TokenType::DoubleEquals, .location = { fileID, index - 2 } });
                    break;
                }

                tokens.push_back({ .type = TokenType::Equals, .location = { fileID, index - 1 } });
                break;
            case '>':
                if (get_current_char() == '=')
                {
                    index++;
                    tokens.push_back({ .type = TokenType::GreaterThanOrEqual, .location = { fileID, index - 2 } });
                    break;
                }

                tokens.push_back({ .type = TokenType::GreaterThan, .location = { fileID, index - 1 } });
                break;
            case '<':
                if (get_current_char() == '=')
                {
                    index++;
                    tokens.push_back({ .type = TokenType::LessThanOrEqual, .location = { fileID, index - 2 } });
                    break;
                }

                tokens.push_back({ .type = TokenType::LessThan, .location = { fileID, index - 1 } });
                break;
            case '!':
                if (get_current_char() == '=')
                {
                    index++;
                    tokens.push_back({ .type = TokenType::NotEqual, .location = { fileID, index - 2 } });
                    break;
                }

                tokens.push_back({ .type = TokenType::ExclamationMark, .location = { fileID, index - 1 } });
                break;
            case '.':
                tokens.push_back({ .type = TokenType::Dot, .location = { fileID, index - 1 } });
                break;
            case '#':
                tokens.push_back({ .type = TokenType::Hash, .location = { fileID, index - 1 } });
                break;
            case '&':
                tokens.push_back({ .type = TokenType::Ampersand, .location = { fileID, index - 1 } });
                break;
            case '"': {
                size_t trueBeginIndex = index;
                std::string value;
                while (get_current_char() != '"')
                {
                    if (index >= file.size())
                        g_context->Error({ fileID, index }, "Unexpected end of file");

                    char c = get_next_char();
                    if (c == '\\')
                    {
                        char escape = get_next_char();
                        switch (escape)
                        {
                            case 'n':
                                value += '\n';
                                break;
                            case '\\':
                                value += '\\';
                                break;
                            default:
                                g_context->Error({ fileID, index }, "Unknown escape char: %c", get_next_char());
                        }
                    }
                    else if (c < 32)
                    {
                        g_context->Error({ fileID, index }, "Unexpected control character: 0x%x", c);
                    }
                    else
                    {
                        value += c;
                    }
                }
                index++;

                tokens.push_back({ .type = TokenType::StringLiteral,
                    .stringValue = value,
                    .location = { fileID, trueBeginIndex } });
                break;
            }
            default: {
                index--;
                if (isdigit(c))
                {
                    size_t trueBeginIndex = index;
                    std::string numberString;

                    int base = 10;
                    if (get_current_char() == '0')
                    {
                        index++;
                        char baseChar = get_next_char();
                        switch (baseChar)
                        {
                            case 'x':
                                base = 16;
                                break;
                            case 'b':
                                base = 2;
                                break;
                            case 'o':
                                base = 8;
                                break;
                            default:
                                index -= 2;
                                break;
                        }

                        if (base != 10 && index >= file.size())
                            g_context->Error({ fileID, index }, "Unexpected end of file");
                    }

                    do
                    {
                        numberString += get_next_char();
                    } while (isdigit(get_current_char()));

                    tokens.push_back({ .type = TokenType::Number,
                        .intValue = std::strtoull(numberString.c_str(), 0, base),
                        .location = { fileID, trueBeginIndex } });
                    break;
                }
                else if (isalpha(c) || c == '_')
                {
                    uint32_t trueBeginIndex = index;
                    std::string value;
                    do
                    {
                        value += get_next_char();
                    } while (isalnum(get_current_char()) || get_current_char() == '_');

                    if (value == "function")
                        tokens.push_back({ .type = TokenType::Function, .location = { fileID, trueBeginIndex } });
                    else if (value == "return")
                        tokens.push_back({ .type = TokenType::Return, .location = { fileID, trueBeginIndex } });
                    else if (value == "if")
                        tokens.push_back({ .type = TokenType::If, .location = { fileID, trueBeginIndex } });
                    else if (value == "extern")
                        tokens.push_back({ .type = TokenType::Extern, .location = { fileID, trueBeginIndex } });
                    else if (value == "while")
                        tokens.push_back({ .type = TokenType::While, .location = { fileID, trueBeginIndex } });
                    else if (value == "include")
                        tokens.push_back({ .type = TokenType::Include, .location = { fileID, trueBeginIndex } });
                    else if (value == "struct")
                        tokens.push_back({ .type = TokenType::Struct, .location = { fileID, trueBeginIndex } });
                    else if (value == "var")
                        tokens.push_back({ .type = TokenType::Var, .location = { fileID, trueBeginIndex } });
                    else if (value == "const")
                        tokens.push_back({ .type = TokenType::Const, .location = { fileID, trueBeginIndex } });
                    else if (value == "endif")
                        tokens.push_back({ .type = TokenType::Endif, .location = { fileID, trueBeginIndex } });
                    else if (value == "to")
                        tokens.push_back({ .type = TokenType::To, .location = { fileID, trueBeginIndex } });
                    else
                        tokens.push_back({ .type = TokenType::Identifier,
                            .stringValue = value,
                            .location = { fileID, trueBeginIndex } });

                    break;
                }

                g_context->Error({ fileID, index }, "Unexpected symbol: `%c`", c);
            }
        }
    }

    tokens.push_back({ .type = TokenType::Eof, .location = { fileID, index } });
    return TokenStream(tokens);
}
