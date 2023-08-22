#include <assert.h>
#include <Token.h>

std::string TokenTypeToString(TokenType tokenType)
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 20, "Not all tokens are handled in Token::ToString()");

    switch (tokenType)
    {
        case TokenType::Number:
            return "Number";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::Eof:
            return "Eof";
        case TokenType::LParen:
            return "LParen";
        case TokenType::RParen:
            return "RParen";
        case TokenType::LCurly:
            return "LCurly";
        case TokenType::RCurly:
            return "RCurly";
        case TokenType::Colon:
            return "Colon";
        case TokenType::Semicolon:
            return "Semicolon";
        case TokenType::Plus:
            return "Plus";
        case TokenType::Minus:
            return "Minus";
        case TokenType::Asterisk:
            return "Asterisk";
        case TokenType::Slash:
            return "Slash";
        case TokenType::Comma:
            return "Comma";
        case TokenType::Equals:
            return "Equals";
        case TokenType::DoubleEquals:
            return "DoubleEquals";
        case TokenType::Function:
            return "Function";
        case TokenType::Return:
            return "Return";
        case TokenType::If:
            return "If";
        case TokenType::Extern:
            return "Extern";
        default:
            assert(false);
    }
}

std::string Token::ToString()
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 20, "Not all tokens are handled in Token::ToString()");
    switch(type)
    {
        case TokenType::Number:
            return std::string("Number (`") + std::to_string(intValue) + "`)";
        case TokenType::Identifier:
            return std::string("Identifier (`") + stringValue + "`)";
        case TokenType::Eof:
            return "EOF";
        case TokenType::LParen:
            return "LParen (`(`)";
        case TokenType::RParen:
            return "RParen (`)`)";
        case TokenType::LCurly:
            return "LCurly (`{`)";
        case TokenType::RCurly:
            return "RCurly (`}`)";
        case TokenType::Colon:
            return "Colon (`:`)";
        case TokenType::Semicolon:
            return "Semicolon (`;`)";
        case TokenType::Plus:
            return "Plus (`+`)";
        case TokenType::Minus:
            return "Minus (`-`)";
        case TokenType::Asterisk:
            return "Asterisk (`*`)";
        case TokenType::Slash:
            return "Slash (`/`)";
        case TokenType::Comma:
            return "Comma (`,`)";
        case TokenType::Equals:
            return "Equals (`=`)";
        case TokenType::DoubleEquals:
            return "DoubleEquals (`==`)";
        case TokenType::Function:
            return "Function (`function`)";
        case TokenType::Return:
            return "Return (`return`)";
        case TokenType::If:
            return "If (`if`)";
        case TokenType::Extern:
            return "Extern (`extern`)";
        default:
            return "Invalid";
    }
}