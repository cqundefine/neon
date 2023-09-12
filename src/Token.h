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

    // Keywords
    Function,
    Return,
    If,
    Extern,
    While,

    _TokenTypeCount
};

std::string TokenTypeToString(TokenType tokenType);

struct Token
{
    TokenType type;

    std::string stringValue;
    uint64_t intValue;

    size_t length;
    Location location;

    std::string ToString();
};