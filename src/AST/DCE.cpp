#include <AST.h>

void NumberExpressionAST::DCE() const
{
}

void VariableExpressionAST::DCE() const
{
    if (auto variable = g_parsedFile->FindGlobalVariable(name))
        variable->used = true;
}

void StringLiteralAST::DCE() const
{
}

void BinaryExpressionAST::DCE() const
{
    lhs->DCE();
    rhs->DCE();
}

void CallExpressionAST::DCE() const
{
    g_parsedFile->FindFunction(calleeName)->used = true;
    for (const auto& arg : args)
        arg->DCE();
}

void CastExpressionAST::DCE() const
{
    child->DCE();
}

void ArrayAccessExpressionAST::DCE() const
{
    array->DCE();
    index->DCE();
}

void DereferenceExpressionAST::DCE() const
{
    pointer->DCE();
}

void MemberAccessExpressionAST::DCE() const
{
    object->DCE();
}

void ReturnStatementAST::DCE() const
{
    if (value)
        value->DCE();
}

void BlockAST::DCE() const
{
    for (const auto& statement : statements)
    {
        if (std::holds_alternative<Ref<StatementAST>>(statement))
            std::get<Ref<StatementAST>>(statement)->DCE();
        else
            std::get<Ref<ExpressionAST>>(statement)->DCE();
    }
}

void IfStatementAST::DCE() const
{
    condition->DCE();
    block->DCE();
    if (elseBlock)
        elseBlock->DCE();
}

void WhileStatementAST::DCE() const
{
    condition->DCE();
    block->DCE();
}

void VariableDefinitionAST::DCE() const
{
    if (initialValue)
        initialValue->DCE();
}

void FunctionAST::DCE() const
{
    if (block)
        block->DCE();
}

void ParsedFile::DCE()
{
    bool changed;
    while (true)
    {
        for (const auto& function : functions)
            function->used = false;

        for (const auto& variable : globalVariables)
            variable->used = false;

        for (const auto& function : functions)
            function->DCE();

        for (const auto& variable : globalVariables)
            variable->DCE();

        FindFunction("main")->used = true;

        std::vector<Ref<FunctionAST>> functionsCopy = functions;
        functions.clear();
        for (const auto& function : functionsCopy)
        {
            if (function->used)
                functions.push_back(function);
        }

        std::vector<Ref<VariableDefinitionAST>> globalVariablesCopy = globalVariables;
        globalVariables.clear();
        for (const auto& variable : globalVariablesCopy)
        {
            if (variable->used)
                globalVariables.push_back(variable);
        }

        if (functions.size() == functionsCopy.size() && globalVariables.size() == globalVariablesCopy.size())
            break;
    }
}
