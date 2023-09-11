#pragma once

#include <map>
#include <string>

enum class NumberType
{
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,

    _NumberTypeCount
};

std::string NumberTypeToString(NumberType numberType);

enum class BinaryOperation
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Equals,
    GreaterThan,

    _BinaryOperationCount
};

extern std::map<BinaryOperation, int> BinaryOperationPrecedence;

std::string BinaryOperationToString(BinaryOperation binaryOperation);
