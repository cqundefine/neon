#include "Type.h"
#include <AST.h>

Ref<ParsedFile> g_parsedFile;

void NumberExpressionAST::AdjustType(Ref<IntegerType> type)
{
    // FIXME: Check if adjustment is correct
    this->type = type;
}

Ref<FunctionAST> ParsedFile::FindFunction(const std::string& name) const
{
    for (const auto& function : functions)
    {
        if (function->name == name)
            return function;
    }

    return nullptr;
}

Ref<VariableDefinitionAST> ParsedFile::FindGlobalVariable(const std::string& name) const
{
    for (const auto& variable : globalVariables)
    {
        if (variable->name == name)
            return variable;
    }

    return nullptr;
}
