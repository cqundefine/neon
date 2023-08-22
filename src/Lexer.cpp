#include <Lexer.h>

Lexer::Lexer(const std::string& fileContent) : m_fileContent(fileContent) {}

Token Lexer::NextToken()
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 19, "Not all tokens are handled in Lexer::NextToken()");

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
        return { .type = TokenType::LParen, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == ')')
    {
        m_index++;
        return { .type = TokenType::RParen, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == '{')
    {
        m_index++;
        return { .type = TokenType::LCurly, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == '}')
    {
        m_index++;
        return { .type = TokenType::RCurly, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == ':')
    {
        m_index++;
        return { .type = TokenType::Colon, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == ';')
    {
        m_index++;
        return { .type = TokenType::Semicolon, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == '+')
    {
        m_index++;
        return { .type = TokenType::Plus, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == '-')
    {
        m_index++;
        return { .type = TokenType::Minus, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == '*')
    {
        m_index++;
        return { .type = TokenType::Asterisk, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == '/')
    {
        m_index++;
        return { .type = TokenType::Slash, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == ',')
    {
        m_index++;
        return { .type = TokenType::Comma, .length = m_index - beginIndex };
    }
    else if (m_fileContent[m_index] == '=')
    {
        m_index++;
        if (m_fileContent[m_index] == '=')
        {
            m_index++;
            return { .type = TokenType::DoubleEquals, .length = m_index - beginIndex };
        }

        return { .type = TokenType::Equals, .length = m_index - beginIndex };
    }

    if (isdigit(m_fileContent[m_index]))
    {
        std::string numberString;
        do {
            numberString += m_fileContent[m_index];
            m_index++;
        } while(isdigit(m_fileContent[m_index]));

        return {
            .type = TokenType::Number,
            .intValue = std::strtoull(numberString.c_str(), 0, 10),
            .length = m_index - beginIndex
        };
    }
    else if (isalpha(m_fileContent[m_index]) || m_fileContent[m_index] == '_')
    {
        std::string value;
        do {
            value += m_fileContent[m_index];
            m_index++;
        } while(isalnum(m_fileContent[m_index])  || m_fileContent[m_index] == '_');
    
        if (value == "function")
            return { .type = TokenType::Function, .length = m_index - beginIndex };
        else if (value == "return")
            return { .type = TokenType::Return, .length = m_index - beginIndex };
        else if (value == "if")
            return { .type = TokenType::If, .length = m_index - beginIndex };
        else
            return { 
                .type = TokenType::Identifier,
                .stringValue = value,
                .length = m_index - beginIndex
            };
    }

    fprintf(stderr, "Unexpected symbol: %c\n", m_fileContent[m_index]);
    exit(1);
}

void Lexer::RollbackToken(Token token)
{
    m_index -= token.length;
}
    