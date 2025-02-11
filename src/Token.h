#pragma once

#include <Utils.h>

enum class TokenType
{
    // Generic
    Number,
    Identifier,
    StringLiteral,
    Eof,

    // Symbols
    LParen,
    RParen,
    LCurly,
    RCurly,
    LSquareBracket,
    RSquareBracket,
    Colon,
    Semicolon,
    Plus,
    Minus,
    Asterisk,
    Slash,
    Comma,
    Equals,
    NotEqual,
    DoubleEquals,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,
    ExclamationMark,
    Dot,
    Hash,
    Ampersand,

    // Keywords
    Function,
    Return,
    If,
    Extern,
    While,
    Include,
    Struct,
    Var,
    Const,
    Endif,
    To,

    _TokenTypeCount
};

struct Token
{
    TokenType type;

    std::string stringValue;
    uint64_t intValue;

    Location location;
};

template <>
struct std::formatter<TokenType>
{
    constexpr auto parse(std::format_parse_context& ctx) { return std::cbegin(ctx); }

    auto format(const TokenType& obj, std::format_context& ctx) const
    {
        static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 39, "Not all tokens are handled in TokenType formatter");
        std::string str = "???";

        switch (obj)
        {
            using enum TokenType;
            case Number:             str = "Number"; break;
            case Identifier:         str = "Identifier"; break;
            case StringLiteral:      str = "StringLiteral"; break;
            case Eof:                str = "Eof"; break;
            case LParen:             str = "LParen"; break;
            case RParen:             str = "RParen"; break;
            case LCurly:             str = "LCurly"; break;
            case RCurly:             str = "RCurly"; break;
            case LSquareBracket:     str = "LSquareBracket"; break;
            case RSquareBracket:     str = "RSquareBracket"; break;
            case Colon:              str = "Colon"; break;
            case Semicolon:          str = "Semicolon"; break;
            case Plus:               str = "Plus"; break;
            case Minus:              str = "Minus"; break;
            case Asterisk:           str = "Asterisk"; break;
            case Slash:              str = "Slash"; break;
            case Comma:              str = "Comma"; break;
            case Equals:             str = "Equals"; break;
            case NotEqual:           str = "NotEqual"; break;
            case DoubleEquals:       str = "DoubleEquals"; break;
            case GreaterThan:        str = "GreaterThan"; break;
            case GreaterThanOrEqual: str = "GreaterThanOrEqual"; break;
            case LessThan:           str = "LessThan"; break;
            case LessThanOrEqual:    str = "LessThanOrEqual"; break;
            case ExclamationMark:    str = "ExclamationMark"; break;
            case Dot:                str = "Dot"; break;
            case Hash:               str = "Hash"; break;
            case Ampersand:          str = "Ampersand"; break;
            case Function:           str = "Function"; break;
            case Return:             str = "Return"; break;
            case If:                 str = "If"; break;
            case Extern:             str = "Extern"; break;
            case While:              str = "While"; break;
            case Include:            str = "Include"; break;
            case Struct:             str = "Struct"; break;
            case Var:                str = "Var"; break;
            case Const:              str = "Const"; break;
            case Endif:              str = "Endif"; break;
            case To:                 str = "To"; break;
            default:                 str = "???"; break;
        }

        return std::format_to(ctx.out(), "{}", str);
    }
};

template <>
struct std::formatter<Token>
{
    constexpr auto parse(std::format_parse_context& ctx) { return std::cbegin(ctx); }

    auto format(const Token& obj, std::format_context& ctx) const
    {
        static_assert(static_cast<uint32_t>(TokenType::_TokenTypeCount) == 39, "Not all tokens are handled in Token formatter");
        std::string str = "???";

        switch (obj.type)
        {
            using enum TokenType;
            case Number:             str = std::format("Number (`{}`)", obj.intValue); break;
            case Identifier:         str = std::format("Identifier (`{}`)", obj.stringValue); break;
            case StringLiteral:      str = std::format("StringLiteral (`{}`)", obj.stringValue); break;
            case Eof:                str = "EOF"; break;
            case LParen:             str = "LParen (`(`)"; break;
            case RParen:             str = "RParen (`)`)"; break;
            case LCurly:             str = "LCurly (`{`)"; break;
            case RCurly:             str = "RCurly (`}`)"; break;
            case LSquareBracket:     str = "LSquareBracket (`[`)"; break;
            case RSquareBracket:     str = "RSquareBracket (`]`)"; break;
            case Colon:              str = "Colon (`:`)"; break;
            case Semicolon:          str = "Semicolon (`;`)"; break;
            case Plus:               str = "Plus (`+`)"; break;
            case Minus:              str = "Minus (`-`)"; break;
            case Asterisk:           str = "Asterisk (`*`)"; break;
            case Slash:              str = "Slash (`/`)"; break;
            case Comma:              str = "Comma (`,`)"; break;
            case Equals:             str = "Equals (`=`)"; break;
            case NotEqual:           str = "Equals (`!=`)"; break;
            case DoubleEquals:       str = "DoubleEquals (`==`)"; break;
            case GreaterThan:        str = "GreaterThan (`>`)"; break;
            case GreaterThanOrEqual: str = "GreaterThan (`>=`)"; break;
            case LessThan:           str = "LessThan (`<`)"; break;
            case LessThanOrEqual:    str = "LessThan (`<=`)"; break;
            case ExclamationMark:    str = "LessThan (`!`)"; break;
            case Dot:                str = "Dot (`.`)"; break;
            case Hash:               str = "Hash (`#`)"; break;
            case Ampersand:          str = "Ampersand (`&`)"; break;
            case Function:           str = "Function (`function`)"; break;
            case Return:             str = "Return (`return`)"; break;
            case If:                 str = "If (`if`)"; break;
            case Extern:             str = "Extern (`extern`)"; break;
            case While:              str = "While (`while`)"; break;
            case Include:            str = "Include (`include`)"; break;
            case Struct:             str = "Struct (`struct`)"; break;
            case Var:                str = "Var (`var`)"; break;
            case Const:              str = "Const (`const`)"; break;
            case Endif:              str = "Endif (`endif`)"; break;
            case To:                 str = "To (`to`)"; break;
            default:                 str = "Invalid"; break;
        }

        return std::format_to(ctx.out(), "{}", str);
    }
};
