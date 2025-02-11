#include <AST.h>
#include <llvm/IR/Type.h>
#include <print>
#include <regex>

template <class... Args>
static void dump(std::string_view msg, uint32_t indentCount, Args&&... args)
{
    std::println("{}{}", std::string(indentCount * 2, ' '), std::vformat(msg, std::make_format_args(args...)));
}

void NumberExpressionAST::Dump(uint32_t indentCount) const
{
    dump("Number Expression (`{}`, {})", indentCount, value, type->Dump());
}

void VariableExpressionAST::Dump(uint32_t indentCount) const
{
    dump("Variable Expression ({}, {})", indentCount, name, type->Dump());
}

void StringLiteralAST::Dump(uint32_t indentCount) const
{
    auto fixedValue = std::regex_replace(value, std::regex("\\\\"), "\\\\");
    fixedValue = std::regex_replace(fixedValue, std::regex("\n"), "\\n");
    dump("String Literal (`{}`)", indentCount, fixedValue);
}

void BinaryExpressionAST::Dump(uint32_t indentCount) const
{
    dump("Binary Expression", indentCount);
    lhs->Dump(indentCount + 1);
    dump("{}", indentCount + 1, binaryOperation);
    rhs->Dump(indentCount + 1);
}

void CallExpressionAST::Dump(uint32_t indentCount) const
{
    dump("Call Expression (`{}`)", indentCount, calleeName);
    for (const auto& arg : args)
        arg->Dump(indentCount + 1);
}

void CastExpressionAST::Dump(uint32_t indentCount) const
{
    dump("Cast Expression ({})", indentCount, castedTo->Dump());
    child->Dump(indentCount + 1);
}

void ArrayAccessExpressionAST::Dump(uint32_t indentCount) const
{
    dump("Array Access Expression ({})", indentCount, GetType()->Dump());
    array->Dump(indentCount + 1);
    index->Dump(indentCount + 1);
}

void DereferenceExpressionAST::Dump(uint32_t indentCount) const
{
    dump("Dereference Expression ({})", indentCount, GetType()->Dump());
    pointer->Dump(indentCount + 1);
}

void MemberAccessExpressionAST::Dump(uint32_t indentCount) const
{
    dump("Member Access Expression (`{}`, {})", indentCount, memberName, GetType()->Dump());
    object->Dump(indentCount + 1);
}

void ReturnStatementAST::Dump(uint32_t indentCount) const
{
    dump("ReturnStatement", indentCount);
    if (value != nullptr)
        value->Dump(indentCount + 1);
}

void BlockAST::Dump(uint32_t indentCount) const
{
    dump("Block", indentCount);

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
    dump("If Statement", indentCount);
    condition->Dump(indentCount + 1);
    block->Dump(indentCount + 1);
    if (elseBlock != nullptr)
        elseBlock->Dump(indentCount + 1);
}

void WhileStatementAST::Dump(uint32_t indentCount) const
{
    puts("While Statement");
    condition->Dump(indentCount + 1);
    block->Dump(indentCount + 1);
}

void VariableDefinitionAST::Dump(uint32_t indentCount) const
{
    dump("Variable Declaration (`{}`, {})", indentCount, name, type->Dump());
    if (initialValue != nullptr)
        initialValue->Dump(indentCount + 1);
}

void FunctionAST::Dump(uint32_t indentCount) const
{
    dump("Function (`{}`)", indentCount, name);
    dump("Return Type: {}", indentCount + 1, returnType->Dump());
    for (const auto& param : params)
        dump("Param ('{}', {})", indentCount + 1, param.name, param.type->Dump());

    if (block != nullptr)
        block->Dump(indentCount + 1);
}

void ParsedFile::Dump(uint32_t indentCount) const
{
    dump("Parsed File", indentCount);
    for (const auto& function : functions)
        function->Dump(indentCount + 1);
    for (const auto& variable : globalVariables)
        variable->Dump(indentCount + 1);
}
