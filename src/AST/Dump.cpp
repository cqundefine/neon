#include <AST.h>
#include <llvm/IR/Type.h>
#include <regex>

#define INDENT(count)                    \
    for (uint32_t i = 0; i < count; i++) \
    {                                    \
        printf("    ");                  \
    }

void NumberExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Number Expression (`%lu`, %s)\n", value, type->Dump().c_str());
}

void VariableExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Variable Expression (%s, %s)\n", name.c_str(), type->Dump().c_str());
}

void StringLiteralAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    auto fixedValue = std::regex_replace(value, std::regex("\\\\"), "\\\\");
    fixedValue = std::regex_replace(fixedValue, std::regex("\n"), "\\n");
    printf("String Literal (`%s`)\n", fixedValue.c_str());
}

void BinaryExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("Binary Expression");
    lhs->Dump(indentCount + 1);
    INDENT(indentCount + 1);
    puts(BinaryOperationToString(binaryOperation).c_str());
    rhs->Dump(indentCount + 1);
}

void CallExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Call Expression (`%s`)\n", calleeName.c_str());
    for (const auto& arg : args)
        arg->Dump(indentCount + 1);
}

void CastExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Cast Expression (%s)\n", castedTo->Dump().c_str());
    child->Dump(indentCount + 1);
}

void ArrayAccessExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Array Access Expression (%s)\n", GetType()->Dump().c_str());
    array->Dump(indentCount + 1);
    index->Dump(indentCount + 1);
}

void DereferenceExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Dereference Expression (%s)\n", GetType()->Dump().c_str());
    pointer->Dump(indentCount + 1);
}

void MemberAccessExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Member Access Expression (`%s`, %s)\n", memberName.c_str(), GetType()->Dump().c_str());
    object->Dump(indentCount + 1);
}

void ReturnStatementAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("ReturnStatement");
    if (value != nullptr)
        value->Dump(indentCount + 1);
}

void BlockAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("Block");

    for (const auto& statement : statements)
    {
        if (std::holds_alternative<Ref<StatementAST>>(statement))
            std::get<Ref<StatementAST>>(statement)->Dump(indentCount + 1);
        else
            std::get<Ref<ExpressionAST>>(statement)->Dump(indentCount + 1);
    }
}

void IfStatementAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("If Statement");
    condition->Dump(indentCount + 1);
    block->Dump(indentCount + 1);
    if (elseBlock != nullptr)
        elseBlock->Dump(indentCount + 1);
}

void WhileStatementAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("While Statement");
    condition->Dump(indentCount + 1);
    block->Dump(indentCount + 1);
}

void VariableDefinitionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Variable Declaration (`%s`, %s)\n", name.c_str(), type->Dump().c_str());
    if (initialValue != nullptr)
        initialValue->Dump(indentCount + 1);
}

void FunctionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Function (`%s`)\n", name.c_str());
    INDENT(indentCount + 1);
    printf("Return Type: %s\n", returnType->Dump().c_str());
    for (const auto& param : params)
    {
        INDENT(indentCount + 1);
        printf("Param ('%s', %s)\n", param.name.c_str(), param.type->Dump().c_str());
    }
    if (block != nullptr)
        block->Dump(indentCount + 1);
}

void ParsedFile::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("Parsed File");
    for (const auto& function : functions)
        function->Dump(indentCount + 1);
}
