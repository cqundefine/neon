#include <AST.h>

void NumberExpressionAST::AdjustTypeToBits(uint32_t bits)
{
    // FIXME: Check if adjustment is correct
    type->bits = bits;
}
