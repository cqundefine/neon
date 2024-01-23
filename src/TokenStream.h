#pragma once

#include <Context.h>
#include <Token.h>

class TokenStream
{
public:
    explicit inline TokenStream(std::vector<Token> tokens)
        : m_tokens(std::move(tokens))
    {
    }

    Token NextToken();
    void PreviousToken();

    void RemoveLastToken();
    void Reset();
    void InsertStream(TokenStream stream);

    void Dump();

private:
    std::vector<Token> m_tokens;
    uint32_t m_index = 0;
};
