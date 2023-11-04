#include <assert.h>
#include <Enums.h>
#include <cstdint>

static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11, "Not all binary operations are in BinaryOperationPrecedence");
std::map<BinaryOperation, int> BinaryOperationPrecedence = {
    {BinaryOperation::Add, 20},
    {BinaryOperation::Subtract, 20},
    {BinaryOperation::Multiply, 30},
    {BinaryOperation::Divide, 30},
    {BinaryOperation::Equals, 10},
    {BinaryOperation::NotEqual, 10},
    {BinaryOperation::GreaterThan, 10},
    {BinaryOperation::GreaterThanOrEqual, 10},
    {BinaryOperation::LessThan, 10},
    {BinaryOperation::LessThanOrEqual, 10},
    {BinaryOperation::Assignment, 1}
};

std::string BinaryOperationToString(BinaryOperation binaryOperation)
{
    static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11, "Not all binary operations are handled in BinaryOperationToString()");

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
        case BinaryOperation::NotEqual:
            return "NotEqual (`!=`)";
        case BinaryOperation::GreaterThan:
            return "GreaterThan (`>`)";
        case BinaryOperation::GreaterThanOrEqual:
            return "GreaterThanOrEqual (`>=`)";
        case BinaryOperation::LessThan:
            return "LessThan (`<`)";
        case BinaryOperation::LessThanOrEqual:
            return "LessThanOrEqual (`<=`)";
        case BinaryOperation::Assignment:
            return "Assignment (`=`)";
        default:
            assert(false);
    }
}
