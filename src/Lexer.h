#pragma once

#include <Context.h>
#include <Token.h>

class Lexer
{
public:
    Token NextToken() const;
    void RollbackToken(Token token) const;

private:
    mutable uint32_t m_index = 0;
};
