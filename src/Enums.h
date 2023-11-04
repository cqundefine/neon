#pragma once

#include <map>
#include <string>

enum class BinaryOperation
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Equals,
    NotEqual,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,

    Assignment,

    _BinaryOperationCount
};

extern std::map<BinaryOperation, int> BinaryOperationPrecedence;

std::string BinaryOperationToString(BinaryOperation binaryOperation);
