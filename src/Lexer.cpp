#include <Lexer.h>

Token Lexer::NextToken() const
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 30, "Not all tokens are handled in Lexer::NextToken()");

    uint32_t beginIndex = m_index;

    if (m_index >= g_context->fileContent.size())
        return { .type = TokenType::Eof };
    
    while(g_context->fileContent[m_index] == ' ' || g_context->fileContent[m_index] == '\t' || g_context->fileContent[m_index] == '\n' || g_context->fileContent[m_index] == '\r')
    {
        m_index++;
        if (m_index >= g_context->fileContent.size())
            return { .type = TokenType::Eof };
    }

    if (g_context->fileContent[m_index] == '(')
    {
        m_index++;
        return { .type = TokenType::LParen, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == ')')
    {
        m_index++;
        return { .type = TokenType::RParen, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '{')
    {
        m_index++;
        return { .type = TokenType::LCurly, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '}')
    {
        m_index++;
        return { .type = TokenType::RCurly, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '[')
    {
        m_index++;
        return { .type = TokenType::LSquareBracket, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == ']')
    {
        m_index++;
        return { .type = TokenType::RSquareBracket, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == ':')
    {
        m_index++;
        return { .type = TokenType::Colon, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == ';')
    {
        m_index++;
        return { .type = TokenType::Semicolon, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '+')
    {
        m_index++;
        return { .type = TokenType::Plus, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '-')
    {
        m_index++;
        return { .type = TokenType::Minus, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '*')
    {
        m_index++;
        return { .type = TokenType::Asterisk, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '/')
    {
        m_index++;
        return { .type = TokenType::Slash, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == ',')
    {
        m_index++;
        return { .type = TokenType::Comma, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '=')
    {
        m_index++;
        if (g_context->fileContent[m_index] == '=')
        {
            m_index++;
            return { .type = TokenType::DoubleEquals, .length = m_index - beginIndex, .location = m_index - 2 };
        }

        return { .type = TokenType::Equals, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '>')
    {
        m_index++;
        if (g_context->fileContent[m_index] == '=')
        {
            m_index++;
            return { .type = TokenType::GreaterThanOrEqual, .length = m_index - beginIndex, .location = m_index - 2 };
        }
        return { .type = TokenType::GreaterThan, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '<')
    {
        m_index++;
        if (g_context->fileContent[m_index] == '=')
        {
            m_index++;
            return { .type = TokenType::LessThanOrEqual, .length = m_index - beginIndex, .location = m_index - 2 };
        }
        return { .type = TokenType::LessThan, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '!')
    {
        m_index++;
        if (g_context->fileContent[m_index] == '=')
        {
            m_index++;
            return { .type = TokenType::NotEqual, .length = m_index - beginIndex, .location = m_index - 2 };
        }
        return { .type = TokenType::ExclamationMark, .length = m_index - beginIndex, .location = m_index - 1 };
    }
    else if (g_context->fileContent[m_index] == '"')
    {
        uint32_t trueBeginIndex = m_index;
        std::string value;
        m_index++;
        do {
            char c = g_context->fileContent[m_index];
            if (c == '\\')
            {
                m_index++;
                if (g_context->fileContent[m_index] == 'n')
                {
                    value += 10;
                }
                else if (g_context->fileContent[m_index] == '\\')
                {
                    value += '\\';
                }
                else
                {
                    g_context->Error(m_index, "Unknown escape char: %c", g_context->fileContent[m_index]);
                }
            }
            else
            {
                value += c;
            }
            m_index++;
        } while(g_context->fileContent[m_index] != '"');
        m_index++;

        return { 
            .type = TokenType::StringLiteral,
            .stringValue = value,
            .length = m_index - beginIndex,
            .location = trueBeginIndex
        };
    }

    if (isdigit(g_context->fileContent[m_index]))
    {
        uint32_t trueBeginIndex = m_index;
        std::string numberString;
        do {
            numberString += g_context->fileContent[m_index];
            m_index++;
        } while(isdigit(g_context->fileContent[m_index]));

        return {
            .type = TokenType::Number,
            .intValue = std::strtoull(numberString.c_str(), 0, 10),
            .length = m_index - beginIndex,
            .location = trueBeginIndex
        };
    }
    else if (isalpha(g_context->fileContent[m_index]) || g_context->fileContent[m_index] == '_')
    {
        uint32_t trueBeginIndex = m_index;
        std::string value;
        do {
            value += g_context->fileContent[m_index];
            m_index++;
        } while(isalnum(g_context->fileContent[m_index])  || g_context->fileContent[m_index] == '_');
    
        if (value == "function")
            return { .type = TokenType::Function, .length = m_index - beginIndex, .location = trueBeginIndex };
        else if (value == "return")
            return { .type = TokenType::Return, .length = m_index - beginIndex, .location = trueBeginIndex };
        else if (value == "if")
            return { .type = TokenType::If, .length = m_index - beginIndex, .location = trueBeginIndex };
        else if (value == "extern")
            return { .type = TokenType::Extern, .length = m_index - beginIndex, .location = trueBeginIndex };
        else if (value == "while")
            return { .type = TokenType::While, .length = m_index - beginIndex, .location = trueBeginIndex };
        else
            return { 
                .type = TokenType::Identifier,
                .stringValue = value,
                .length = m_index - beginIndex,
                .location = trueBeginIndex
            };
    }

    printf("%X\n", g_context->fileContent[1]);

    g_context->Error(m_index, "Unexpected symbol: %c", g_context->fileContent[m_index]);
}

void Lexer::RollbackToken(Token token) const
{
    m_index -= token.length;
}
