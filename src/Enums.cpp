#include <assert.h>
#include <Enums.h>

std::string NumberTypeToString(NumberType numberType)
{
    static_assert(static_cast<uint32_t>(NumberType::_NumberTypeCount) == 8, "Not all number types are handled in NumberExpressionAST::Codegen()");

    switch(numberType)
    {
        case NumberType::Int8:
            return "Int8";
        case NumberType::UInt8:
            return "UInt8";
        case NumberType::Int16:
            return "Int16";
        case NumberType::UInt16:
            return "UInt16";
        case NumberType::Int32:
            return "Int32";
        case NumberType::UInt32:
            return "UInt32";
        case NumberType::Int64:
            return "Int64";
        case NumberType::UInt64:
            return "UInt64";
        default:
            assert(false);
    }
}

static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 6, "Not all binary operations are in BinaryOperationPrecedence");
std::map<BinaryOperation, int> BinaryOperationPrecedence = {
    {BinaryOperation::Add, 20},
    {BinaryOperation::Subtract, 20},
    {BinaryOperation::Multiply, 30},
    {BinaryOperation::Divide, 30},
    {BinaryOperation::Equals, 10},
    {BinaryOperation::GreaterThan, 10}
};

std::string BinaryOperationToString(BinaryOperation binaryOperation)
{
    static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 6, "Not all binary operations are handled in BinaryOperationToString()");

    switch(binaryOperation)
    {
        case BinaryOperation::Add:
            return "Add (`+`)";
        case BinaryOperation::Subtract:
            return "Subtract (`-`)";
        case BinaryOperation::Multiply:
            return "Multiply (`*`)";
        case BinaryOperation::Divide:
            return "Divide (`/`)";
        case BinaryOperation::Equals:
            return "Equals (`==`)";
        case BinaryOperation::GreaterThan:
            return "GreaterThan (`>`)";
        default:
            assert(false);
    }
}
