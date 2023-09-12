#include <assert.h>
#include <Enums.h>

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
