#include <AST.h>

void NumberExpressionAST::AdjustType(Ref<IntegerType> type)
{
    // FIXME: Check if adjustment is correct
    this->type = type;
}
