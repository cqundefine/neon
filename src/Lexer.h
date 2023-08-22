#pragma once

#include <Token.h>

class Lexer
{
public:
    Lexer(const std::string& fileContent);

    Token NextToken();
    void RollbackToken(Token token);

    std::pair<uint32_t, uint32_t> LineColumnFromOffset(uint32_t offset);

private:
    std::string m_fileContent;
    uint32_t m_index = 0;
};
