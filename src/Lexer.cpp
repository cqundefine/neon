#include <Error.h>
#include <Lexer.h>

Lexer::Lexer(const std::string& fileContent) : m_fileContent(fileContent) {}

Token Lexer::NextToken()
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 20, "Not all tokens are handled in Lexer::NextToken()");

    uint32_t beginIndex = m_index;

    if (m_index >= m_fileContent.size())
        return {
            .type = TokenType::Eof
        };
    
    while(m_fileContent[m_index] == ' ' || m_fileContent[m_index] == '\t' || m_fileContent[m_index] == '\n' || m_fileContent[m_index] == '\r')
        m_index++;

    if (m_fileContent[m_index] == '(')
    {
        m_index++;
        return { .type = TokenType::LParen, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == ')')
    {
        m_index++;
        return { .type = TokenType::RParen, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == '{')
    {
        m_index++;
        return { .type = TokenType::LCurly, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == '}')
    {
        m_index++;
        return { .type = TokenType::RCurly, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == ':')
    {
        m_index++;
        return { .type = TokenType::Colon, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == ';')
    {
        m_index++;
        return { .type = TokenType::Semicolon, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == '+')
    {
        m_index++;
        return { .type = TokenType::Plus, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == '-')
    {
        m_index++;
        return { .type = TokenType::Minus, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == '*')
    {
        m_index++;
        return { .type = TokenType::Asterisk, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == '/')
    {
        m_index++;
        return { .type = TokenType::Slash, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == ',')
    {
        m_index++;
        return { .type = TokenType::Comma, .length = m_index - beginIndex, .offset = m_index - 1 };
    }
    else if (m_fileContent[m_index] == '=')
    {
        m_index++;
        if (m_fileContent[m_index] == '=')
        {
            m_index++;
            return { .type = TokenType::DoubleEquals, .length = m_index - beginIndex, .offset = m_index - 2 };
        }

        return { .type = TokenType::Equals, .length = m_index - beginIndex, .offset = m_index - 1 };
    }

    if (isdigit(m_fileContent[m_index]))
    {
        uint32_t trueBeginIndex = m_index;
        std::string numberString;
        do {
            numberString += m_fileContent[m_index];
            m_index++;
        } while(isdigit(m_fileContent[m_index]));

        return {
            .type = TokenType::Number,
            .intValue = std::strtoull(numberString.c_str(), 0, 10),
            .length = m_index - beginIndex,
            .offset = trueBeginIndex
        };
    }
    else if (isalpha(m_fileContent[m_index]) || m_fileContent[m_index] == '_')
    {
        uint32_t trueBeginIndex = m_index;
        std::string value;
        do {
            value += m_fileContent[m_index];
            m_index++;
        } while(isalnum(m_fileContent[m_index])  || m_fileContent[m_index] == '_');
    
        if (value == "function")
            return { .type = TokenType::Function, .length = m_index - beginIndex, .offset = trueBeginIndex };
        else if (value == "return")
            return { .type = TokenType::Return, .length = m_index - beginIndex, .offset = trueBeginIndex };
        else if (value == "if")
            return { .type = TokenType::If, .length = m_index - beginIndex, .offset = trueBeginIndex };
        else if (value == "extern")
            return { .type = TokenType::Extern, .length = m_index - beginIndex, .offset = trueBeginIndex };
        else
            return { 
                .type = TokenType::Identifier,
                .stringValue = value,
                .length = m_index - beginIndex,
                .offset = trueBeginIndex
            };
    }

    Error(m_index, "Unexpected symbol: %c", m_fileContent[m_index]);
}

void Lexer::RollbackToken(Token token)
{
    m_index -= token.length;
}

std::pair<uint32_t, uint32_t> Lexer::LineColumnFromOffset(uint32_t offset)
{
    uint32_t line = 1;
    uint32_t column = 1;

    for (uint32_t index = 0; index <= offset; index++)
    {
        if (m_fileContent[index] == '\n')
        {
            line++;
            column = 1;
        }
        else
        {
            column++;
        }
    }

    return { line, column };
}
