#include "Type.h"
#include <AST.h>
#include <Context.h>
#include <TypeCasts.h>
#include <llvm/IR/Type.h>

struct VariableInfo
{
    Ref<Type> type;
    bool isConst;
};
static std::vector<std::map<std::string, VariableInfo>> blockStack;
[[nodiscard]] static VariableInfo FindVariable(const std::string& name, Location location)
{
    for (int i = blockStack.size() - 1; i >= 0; i--)
    {
        const auto& block = blockStack.at(i);
        if (block.contains(name))
            return block.at(name);
    }

    g_context->Error(location, "Can't find variable: {}", name);
}

static std::string typecheckCurrentFunction;
static bool foundMain = false;

struct TypecheckFunction
{
    std::vector<Ref<Type>> params;
    Ref<Type> returnType;
};
static std::map<std::string, TypecheckFunction> typecheckFunctions;

void NumberExpressionAST::Typecheck()
{
}

void VariableExpressionAST::Typecheck()
{
    type = FindVariable(name, location).type;
}

void StringLiteralAST::Typecheck()
{
}

void BinaryExpressionAST::Typecheck()
{
    lhs->Typecheck();
    rhs->Typecheck();

    if (is<NumberExpressionAST>(rhs) && is<IntegerType>(lhs->GetType()))
        as<NumberExpressionAST>(rhs)->AdjustType(StaticRefCast<IntegerType>(lhs->GetType()));

    if (*lhs->GetType() != *rhs->GetType())
        g_context->Error(
            location, "Wrong binary operation: {} with type {}", lhs->GetType()->ReadableName(), rhs->GetType()->ReadableName());

    if (binaryOperation == BinaryOperation::Assignment && !is<VariableExpressionAST>(lhs) && !is<ArrayAccessExpressionAST>(lhs) &&
        !is<DereferenceExpressionAST>(lhs) && !is<MemberAccessExpressionAST>(lhs))
        g_context->Error(location, "Can't assign to non-variable expression");

    if (binaryOperation == BinaryOperation::Assignment && is<VariableExpressionAST>(lhs) &&
        FindVariable(as<VariableExpressionAST>(lhs)->name, lhs->location).isConst)
        g_context->Error(location, "Can't assign to const variable");
}

void CallExpressionAST::Typecheck()
{
    if (typecheckFunctions.find(calleeName) == typecheckFunctions.end())
        g_context->Error(location, "Can't find function: {}", calleeName);

    auto function = typecheckFunctions[calleeName];

    returnedType = function.returnType;

    if (function.params.size() != args.size())
        g_context->Error(location, "Wrong number of arguments: {}, expected {}", args.size(), function.params.size());

    for (int i = 0; i < args.size(); i++)
    {
        args[i]->Typecheck();

        if (is<NumberExpressionAST>(args[i]) && is<IntegerType>(function.params[i]))
            as<NumberExpressionAST>(args[i])->AdjustType(StaticRefCast<IntegerType>(function.params[i]));

        if (*args[i]->GetType() != *function.params[i])
            g_context->Error(
                location, "Wrong argument type: {}, expected {}", args[i]->GetType()->ReadableName(), function.params[i]->ReadableName());
    }
}

void CastExpressionAST::Typecheck()
{
    child->Typecheck();

    if (!is<IntegerType>(child->GetType()) && !is<PointerType>(child->GetType()))
        g_context->Error(location, "Can't cast from type {}", child->GetType()->ReadableName());

    if (!is<IntegerType>(castedTo) && !is<PointerType>(castedTo))
        g_context->Error(location, "Can't cast to type {}", castedTo->ReadableName());
}

void ArrayAccessExpressionAST::Typecheck()
{
    array->Typecheck();
    index->Typecheck();

    if (!is<IntegerType>(index->GetType()))
        g_context->Error(location, "Array index must be an integer");
}

void DereferenceExpressionAST::Typecheck()
{
    pointer->Typecheck();

    if (!is<PointerType>(pointer->type))
        g_context->Error(location, "Can't dereference non-pointer type: {}", pointer->type->ReadableName());
}

void MemberAccessExpressionAST::Typecheck()
{
    object->Typecheck();

    if (!is<StructType>(object->GetType()))
        g_context->Error(location, "Can't access member of non-struct type: {}", object->GetType()->ReadableName());

    auto structType = as<StructType>(object->GetType());
    for (const auto& [memberName, memberType] : g_context->structs[structType->name].members)
    {
        if (memberName == this->memberName)
        {
            return;
        }
    }

    g_context->Error(location, "Can't access member {}", memberName);
}

void ReturnStatementAST::Typecheck()
{
    returnedType = FindVariable(typecheckCurrentFunction, location).type;

    if (value != nullptr)
    {
        value->Typecheck();

        if (is<NumberExpressionAST>(value) && is<IntegerType>(returnedType))
            as<NumberExpressionAST>(value)->AdjustType(StaticRefCast<IntegerType>(returnedType));

        if (*value->GetType() != *returnedType)
            g_context->Error(
                location, "Wrong return type: {}, expected {}", value->GetType()->ReadableName(), returnedType->ReadableName());
    }
}

void BlockAST::Typecheck()
{
    blockStack.push_back({});

    for (const auto& statement : statements)
    {
        if (std::holds_alternative<Ref<StatementAST>>(statement))
            std::get<Ref<StatementAST>>(statement)->Typecheck();
        else
            std::get<Ref<ExpressionAST>>(statement)->Typecheck();
    }

    blockStack.pop_back();
}

void IfStatementAST::Typecheck()
{
    condition->Typecheck();
    block->Typecheck();
    if (elseBlock != nullptr)
        elseBlock->Typecheck();
}

void WhileStatementAST::Typecheck()
{
    condition->Typecheck();
    block->Typecheck();
}

void VariableDefinitionAST::Typecheck()
{
    type->Typecheck(location);

    if (isConst && initialValue == nullptr)
        g_context->Error(location, "Const variable must have an initial value");

    if (initialValue != nullptr)
    {
        initialValue->Typecheck();

        if (*initialValue->GetType() != *type)
            g_context->Error(
                location, "Wrong variable type: {}, expected {}", initialValue->GetType()->ReadableName(), type->ReadableName());

        if (auto* number = as_if<NumberExpressionAST>(initialValue))
            number->AdjustType(StaticRefCast<IntegerType>(type));
    }

    blockStack.back()[name] = {type, isConst};
}

void FunctionAST::Typecheck()
{
    assert(name != "");
    assert(returnType);

    typecheckCurrentFunction = name;
    blockStack.push_back({});

    blockStack.back()[name] = {returnType, false};

    std::vector<Ref<Type>> typecheckParams;
    for (const auto& param : params)
    {
        typecheckParams.push_back(param.type);

        param.type->Typecheck(location);

        blockStack.back()[param.name] = {param.type, false};
    }

    returnType->Typecheck(location);

    typecheckFunctions[name] = {.params = typecheckParams, .returnType = returnType};

    if (name == "main")
    {
        foundMain = true;

        if (!is<IntegerType>(returnType))
            g_context->Error(location, "Main function must return an integer");

        if (params.size() == 2 && (!is<IntegerType>(params[0].type) || !is<PointerType>(params[1].type)))
            g_context->Error(location, "Invalid main function parameters");

        if (params.size() != 0 && params.size() != 2)
            g_context->Error(location, "Main function must have no parameters or argc and argv");
    }

    if (block != nullptr)
    {
        block->Typecheck();

        // TODO: This is not an ideal solution, but it works for now
        if (!is<VoidType>(returnType))
        {
            if (block->statements.size() == 0)
                g_context->Error(location, "Function must have a return statement");

            auto lastStatement = block->statements[block->statements.size() - 1];
            if (!std::holds_alternative<Ref<StatementAST>>(lastStatement))
                g_context->Error(location, "Last statement must be a return statement");

            if (!is<ReturnStatementAST>(std::get<Ref<StatementAST>>(lastStatement)))
                g_context->Error(location, "Last statement must be a return statement");
        }
    }

    blockStack.pop_back();
    typecheckCurrentFunction = "";
}

void ParsedFile::Typecheck()
{
    blockStack.push_back({});

    auto int64 = MakeRef<IntegerType>(64, false);
    typecheckFunctions["syscall0"] = {.params = {int64}, .returnType = int64};
    typecheckFunctions["syscall1"] = {.params = {int64, int64}, .returnType = int64};
    typecheckFunctions["syscall2"] = {.params = {int64, int64, int64}, .returnType = int64};
    typecheckFunctions["syscall3"] = {.params = {int64, int64, int64, int64}, .returnType = int64};
    typecheckFunctions["syscall4"] = {.params = {int64, int64, int64, int64, int64}, .returnType = int64};
    typecheckFunctions["syscall5"] = {.params = {int64, int64, int64, int64, int64, int64}, .returnType = int64};
    typecheckFunctions["syscall6"] = {.params = {int64, int64, int64, int64, int64, int64, int64}, .returnType = int64};

    for (const auto& variable : globalVariables)
        variable->Typecheck();

    for (const auto& function : functions)
        function->Typecheck();

    if (!foundMain)
        g_context->Error({}, "No main function found");

    blockStack.pop_back();

    assert(blockStack.size() == 0);
}
