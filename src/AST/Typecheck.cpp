#include <AST.h>
#include <Context.h>
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

    g_context->Error(location, "Can't find variable: %s", name.c_str());
}

static std::string typecheckCurrentFunction;
static bool foundMain = false;

struct TypecheckFunction
{
    std::vector<Ref<Type>> params;
    Ref<Type> returnType;
};
static std::map<std::string, TypecheckFunction> typecheckFunctions;

void NumberExpressionAST::Typecheck() const
{
}

void VariableExpressionAST::Typecheck() const
{
    type = FindVariable(name, location).type;
}

void StringLiteralAST::Typecheck() const
{
}

void BinaryExpressionAST::Typecheck() const
{
    lhs->Typecheck();
    rhs->Typecheck();

    if (rhs->type == ExpressionType::Number && lhs->GetType()->type == TypeEnum::Integer)
        StaticRefCast<NumberExpressionAST>(rhs)->AdjustType(StaticRefCast<IntegerType>(lhs->GetType()));

    if (*lhs->GetType() != *rhs->GetType())
        g_context->Error(location, "Wrong binary operation: %s with type %s", lhs->GetType()->ReadableName().c_str(), rhs->GetType()->ReadableName().c_str());

    if (binaryOperation == BinaryOperation::Assignment && lhs->type != ExpressionType::Variable && lhs->type != ExpressionType::ArrayAccess && lhs->type != ExpressionType::Dereference && lhs->type != ExpressionType::MemberAccess)
        g_context->Error(location, "Can't assign to non-variable expression");

    if (binaryOperation == BinaryOperation::Assignment && lhs->type == ExpressionType::Variable && FindVariable(StaticRefCast<VariableExpressionAST>(lhs)->name, lhs->location).isConst)
        g_context->Error(location, "Can't assign to const variable");
}

void CallExpressionAST::Typecheck() const
{
    if (typecheckFunctions.find(calleeName) == typecheckFunctions.end())
        g_context->Error(location, "Can't find function: %s", calleeName.c_str());

    auto function = typecheckFunctions[calleeName];

    returnedType = function.returnType;

    if (function.params.size() != args.size())
        g_context->Error(location, "Wrong number of arguments: %d, expected %d", args.size(), function.params.size());

    for (int i = 0; i < args.size(); i++)
    {
        args[i]->Typecheck();

        if (args[i]->type == ExpressionType::Number && function.params[i]->type == TypeEnum::Integer)
            StaticRefCast<NumberExpressionAST>(args[i])->AdjustType(StaticRefCast<IntegerType>(function.params[i]));

        if (*args[i]->GetType() != *function.params[i])
            g_context->Error(location, "Wrong argument type: %s, expected %s", args[i]->GetType()->ReadableName().c_str(), function.params[i]->ReadableName().c_str());
    }
}

void CastExpressionAST::Typecheck() const
{
    child->Typecheck();

    if (child->GetType()->type != TypeEnum::Integer && child->GetType()->type != TypeEnum::Pointer)
        g_context->Error(location, "Can't cast from type %s", child->GetType()->ReadableName().c_str());

    if (castedTo->type != TypeEnum::Integer && castedTo->type != TypeEnum::Pointer)
        g_context->Error(location, "Can't cast to type %s", castedTo->ReadableName().c_str());
}

void ArrayAccessExpressionAST::Typecheck() const
{
    array->Typecheck();
    index->Typecheck();

    if (index->GetType()->type != TypeEnum::Integer)
        g_context->Error(location, "Array index must be an integer");
}

void DereferenceExpressionAST::Typecheck() const
{
    pointer->Typecheck();

    if (pointer->GetType()->type != TypeEnum::Pointer)
        g_context->Error(location, "Can't dereference non-pointer type: %s", pointer->GetType()->ReadableName().c_str());
}

void MemberAccessExpressionAST::Typecheck() const
{
    object->Typecheck();

    Ref<StructType> structType;

    if (object->GetType()->type == TypeEnum::Struct)
    {
        structType = StaticRefCast<StructType>(object->GetType());
    }
    else if (object->GetType()->type == TypeEnum::Pointer)
    {
        auto pointerType = StaticRefCast<PointerType>(object->GetType());
        if (pointerType->underlayingType->type == TypeEnum::Struct)
            structType = StaticRefCast<StructType>(pointerType->underlayingType);
        else
            g_context->Error(location, "Can't access member of non-struct type: %s", pointerType->underlayingType->ReadableName().c_str());
    }
    else
    {
        g_context->Error(location, "Can't access member of non-struct type: %s", object->GetType()->ReadableName().c_str());
    }

    for (const auto& [memberName, memberType] : g_context->structs[structType->name].members)
    {
        if (memberName == this->memberName)
        {
            return;
        }
    }

    g_context->Error(location, "Can't access member %s", memberName.c_str());

}

void ReturnStatementAST::Typecheck() const
{
    returnedType = FindVariable(typecheckCurrentFunction, location).type;

    if (value != nullptr)
    {
        value->Typecheck();

        if (value->type == ExpressionType::Number && returnedType->type == TypeEnum::Integer)
            StaticRefCast<NumberExpressionAST>(value)->AdjustType(StaticRefCast<IntegerType>(returnedType));

        if (*value->GetType() != *returnedType)
            g_context->Error(location, "Wrong return type: %s, expected %s", value->GetType()->ReadableName().c_str(), returnedType->ReadableName().c_str());
    }
}

void BlockAST::Typecheck() const
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

void IfStatementAST::Typecheck() const
{
    condition->Typecheck();
    block->Typecheck();
    if (elseBlock != nullptr)
        elseBlock->Typecheck();
}

void WhileStatementAST::Typecheck() const
{
    condition->Typecheck();
    block->Typecheck();
}

void VariableDefinitionAST::Typecheck() const
{
    type->Typecheck(location);

    if (isConst && initialValue == nullptr)
        g_context->Error(location, "Const variable must have an initial value");

    if (initialValue != nullptr)
    {
        initialValue->Typecheck();

        if (*initialValue->GetType() != *type)
            g_context->Error(location, "Wrong variable type: %s, expected %s", initialValue->GetType()->ReadableName().c_str(), type->ReadableName().c_str());

        if (initialValue->type == ExpressionType::Number)
        {
            StaticRefCast<NumberExpressionAST>(initialValue)->AdjustType(StaticRefCast<IntegerType>(type));
        }
    }

    blockStack.back()[name] = { type, isConst };
}

void FunctionAST::Typecheck() const
{
    assert(name != "");
    assert(returnType);

    typecheckCurrentFunction = name;
    blockStack.push_back({});

    blockStack.back()[name] = { returnType, false };

    std::vector<Ref<Type>> typecheckParams;
    for (const auto& param : params)
    {
        typecheckParams.push_back(param.type);

        param.type->Typecheck(location);

        blockStack.back()[param.name] = { param.type, false };
    }

    returnType->Typecheck(location);

    if (returnType->type == TypeEnum::Struct)
        g_context->Error(location, "Returning structs is not supported yet");

    typecheckFunctions[name] = {
        .params = typecheckParams,
        .returnType = returnType
    };

    if (name == "main")
    {
        foundMain = true;

        if (returnType->type != TypeEnum::Integer)
            g_context->Error(location, "Main function must return an integer");

        if (params.size() == 2 && (params[0].type->type != TypeEnum::Integer || params[1].type->type != TypeEnum::Pointer))
            g_context->Error(location, "Invalid main function parameters");

        if (params.size() != 0 && params.size() != 2)
            g_context->Error(location, "Main function must have no parameters or argc and argv");
    }

    if (block != nullptr)
    {
        block->Typecheck();

        // TODO: This is not an ideal solution, but it works for now
        if (returnType->type != TypeEnum::Void)
        {
            if (block->statements.size() == 0)
                g_context->Error(location, "Function must have a return statement");

            auto lastStatement = block->statements[block->statements.size() - 1];
            if (!std::holds_alternative<Ref<StatementAST>>(lastStatement))
                g_context->Error(location, "Last statement must be a return statement");

            if (std::get<Ref<StatementAST>>(lastStatement)->type != StatementType::Return)
                g_context->Error(location, "Last statement must be a return statement");
        }
    }

    blockStack.pop_back();
    typecheckCurrentFunction = "";
}

void ParsedFile::Typecheck() const
{
    blockStack.push_back({});

    auto int64 = MakeRef<IntegerType>(64, false);
    typecheckFunctions["syscall0"] = { .params = { int64 }, .returnType = int64 };
    typecheckFunctions["syscall1"] = { .params = { int64, int64 }, .returnType = int64 };
    typecheckFunctions["syscall2"] = { .params = { int64, int64, int64 }, .returnType = int64 };
    typecheckFunctions["syscall3"] = { .params = { int64, int64, int64, int64 }, .returnType = int64 };
    typecheckFunctions["syscall4"] = { .params = { int64, int64, int64, int64, int64 }, .returnType = int64 };
    typecheckFunctions["syscall5"] = { .params = { int64, int64, int64, int64, int64, int64 }, .returnType = int64 };
    typecheckFunctions["syscall6"] = { .params = { int64, int64, int64, int64, int64, int64, int64 }, .returnType = int64 };

    for (const auto& variable : globalVariables)
        variable->Typecheck();

    for (const auto& function : functions)
        function->Typecheck();

    if (!foundMain)
        g_context->Error({}, "No main function found");

    blockStack.pop_back();

    assert(blockStack.size() == 0);
}
