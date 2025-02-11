#include <Enums.h>
#include <cstdint>

static_assert(
    static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11,
    "Not all binary operations are in BinaryOperationPrecedence");
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
    {BinaryOperation::Assignment, 1}};
