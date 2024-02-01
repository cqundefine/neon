#include <Token.h>
#include <assert.h>

std::string TokenTypeToString(TokenType tokenType)
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 33, "Not all tokens are handled in Token::ToString()");

    switch (tokenType)
    {
        case TokenType::Number:
            return "Number";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::StringLiteral:
            return "StringLiteral";
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
        case TokenType::LSquareBracket:
            return "LSquareBracket";
        case TokenType::RSquareBracket:
            return "RSquareBracket";
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
        case TokenType::NotEqual:
            return "NotEqual";
        case TokenType::DoubleEquals:
            return "DoubleEquals";
        case TokenType::GreaterThan:
            return "GreaterThan";
        case TokenType::GreaterThanOrEqual:
            return "GreaterThanOrEqual";
        case TokenType::LessThan:
            return "LessThan";
        case TokenType::LessThanOrEqual:
            return "LessThanOrEqual";
        case TokenType::ExclamationMark:
            return "ExclamationMark";
        case TokenType::Dot:
            return "Dot";
        case TokenType::Function:
            return "Function";
        case TokenType::Return:
            return "Return";
        case TokenType::If:
            return "If";
        case TokenType::Extern:
            return "Extern";
        case TokenType::While:
            return "While";
        case TokenType::Include:
            return "Include";
        case TokenType::Struct:
            return "Struct";
        default:
            assert(false);
    }
}

std::string Token::ToString()
{
    static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 33, "Not all tokens are handled in Token::ToString()");
    switch (type)
    {
        case TokenType::Number:
            return std::string("Number (`") + std::to_string(intValue) + "`)";
        case TokenType::Identifier:
            return std::string("Identifier (`") + stringValue + "`)";
        case TokenType::StringLiteral:
            return std::string("StringLiteral (`") + stringValue + "`)";
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
        case TokenType::LSquareBracket:
            return "LSquareBracket (`[`)";
        case TokenType::RSquareBracket:
            return "RSquareBracket (`]`)";
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
        case TokenType::NotEqual:
            return "Equals (`!=`)";
        case TokenType::DoubleEquals:
            return "DoubleEquals (`==`)";
        case TokenType::GreaterThan:
            return "GreaterThan (`>`)";
        case TokenType::GreaterThanOrEqual:
            return "GreaterThan (`>=`)";
        case TokenType::LessThan:
            return "LessThan (`<`)";
        case TokenType::LessThanOrEqual:
            return "LessThan (`<=`)";
        case TokenType::ExclamationMark:
            return "LessThan (`!`)";
        case TokenType::Dot:
            return "Dot (`.`)";
        case TokenType::Function:
            return "Function (`function`)";
        case TokenType::Return:
            return "Return (`return`)";
        case TokenType::If:
            return "If (`if`)";
        case TokenType::Extern:
            return "Extern (`extern`)";
        case TokenType::While:
            return "While (`while`)";
        case TokenType::Include:
            return "Include (`include`)";
        case TokenType::Struct:
            return "Struct (`struct`)";
        default:
            return "Invalid";
    }
}
