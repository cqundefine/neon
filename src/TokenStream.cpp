#include <TokenStream.h>
#include <print>

Token TokenStream::NextToken()
{
    if (m_index >= m_tokens.size())
        assert(false && "TokenStream::NextToken() called after EOF");

    return m_tokens[m_index++];
}

Token TokenStream::PeekToken()
{
    if (m_index >= m_tokens.size())
        assert(false && "TokenStream::PeekToken() called after EOF");

    return m_tokens[m_index];
}

void TokenStream::PreviousToken()
{
    if (m_index == 0)
        assert(false && "TokenStream::PreviousToken() called at the beginning of the stream");

    m_index--;
}

void TokenStream::RemoveLastToken()
{
    m_tokens.erase(m_tokens.begin() + m_index - 1);
    m_index--;
}

void TokenStream::Reset()
{
    m_index = 0;
}

void TokenStream::InsertStream(TokenStream stream)
{
    m_tokens.insert(m_tokens.begin() + m_index, stream.m_tokens.begin(), stream.m_tokens.end() - 1);
}

void TokenStream::Dump()
{
    auto currentIndex = m_index;

    Token token;
    std::println("Dumping token stream:");
    do
    {
        token = NextToken();
        std::println("{}", token);
    } while (token.type != TokenType::Eof);

    m_index = currentIndex;
}
