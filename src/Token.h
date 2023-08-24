#pragma once

#include <string>

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
    Colon,
    Semicolon,
    Plus,
    Minus,
    Asterisk,
    Slash,
    Comma,
    Equals,
    DoubleEquals,

    // Keywords
    Function,
    Return,
    If,
    Extern,

    _TokenTypeCount
};

std::string TokenTypeToString(TokenType tokenType);

struct Token
{
    TokenType type;

    std::string stringValue;
    uint64_t intValue;

    size_t length;
    uint32_t offset;

    std::string ToString();
};