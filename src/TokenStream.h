#pragma once

#include <Context.h>
#include <Token.h>
#include <span>

class TokenStream
{
public:
    explicit inline TokenStream(std::span<const Token> tokens)
        : m_tokens(tokens.begin(), tokens.end())
    {
    }

    Token NextToken();
    Token PeekToken();
    void PreviousToken();

    void RemoveLastToken();
    void Reset();
    void InsertStream(TokenStream stream);

    void Dump();

private:
    std::vector<Token> m_tokens;
    uint32_t m_index = 0;
};
