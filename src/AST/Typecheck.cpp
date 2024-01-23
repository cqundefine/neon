#include <AST.h>
#include <Context.h>
#include <llvm/IR/Type.h>

static std::vector<std::map<std::string, Ref<Type>>> blockStack;
[[nodiscard]] static Ref<Type> FindVariable(const std::string& name, Location location)
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
    type = FindVariable(name, location);
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

    if(*lhs->GetType() != *rhs->GetType())
        g_context->Error(location, "Wrong binary operation: %s with type %s", lhs->GetType()->Dump().c_str(), rhs->GetType()->Dump().c_str());
    
    if(binaryOperation == BinaryOperation::Assignment && lhs->type != ExpressionType::Variable && lhs->type != ExpressionType::ArrayAccess && lhs->type != ExpressionType::Dereference)
        g_context->Error(location, "Can't assign to non-variable expression");

    // FIXME: Check if variable exists in the current scope
}

void CallExpressionAST::Typecheck() const
{
    assert(typecheckFunctions.count(calleeName));
    auto function = typecheckFunctions[calleeName];
    
    returnedType = function.returnType;
    
    assert(function.params.size() == args.size());
    for (int i = 0; i < args.size(); i++)
    {
        args[i]->Typecheck();
        if (args[i]->type == ExpressionType::Number)
        {
            assert(function.params[i]->type == TypeEnum::Integer);
            StaticRefCast<NumberExpressionAST>(args[i])->AdjustType(StaticRefCast<IntegerType>(function.params[i]));
        }
    }
    // FIXME: Check if args are correct
}

void CastExpressionAST::Typecheck() const
{
    assert(child->type == ExpressionType::StringLiteral);
    assert(castedTo->type == TypeEnum::Integer && reinterpret_cast<IntegerType*>(castedTo.get())->bits == 64);
}

void ArrayAccessExpressionAST::Typecheck() const
{
    array->Typecheck();
    index->Typecheck();
}

void DereferenceExpressionAST::Typecheck() const
{
    pointer->Typecheck();

    if (pointer->type->type != TypeEnum::Pointer)
        g_context->Error(location, "Can't dereference non-pointer type: %s", pointer->type->Dump().c_str());
}

void MemberAccessExpressionAST::Typecheck() const
{
    object->Typecheck();
    // FIXME: Check if struct has that field once we actually support structs
    if (memberName != "data" && memberName != "size")
        g_context->Error(location, "Can't access member %s", memberName.c_str());
}

void ReturnStatementAST::Typecheck() const
{
    returnedType = FindVariable(typecheckCurrentFunction, location);

    // FIXME: Check if correct type is returned
    value->Typecheck();
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
    if (initialValue != nullptr)
    {
        if (initialValue->type == ExpressionType::Number)
        {
            assert(type->type == TypeEnum::Integer);
            StaticRefCast<NumberExpressionAST>(initialValue)->AdjustType(StaticRefCast<IntegerType>(type));
        }
        initialValue->Typecheck();
    }
    blockStack.back()[name] = type;
    // Check if default assignment is correct
}

void FunctionAST::Typecheck() const
{
    typecheckCurrentFunction = name;
    blockStack.push_back({});

    blockStack.back()[name] = returnType;
    
    std::vector<Ref<Type>> typecheckParams;
    for (const auto& param : params)
    {
        typecheckParams.push_back(param.type);
        blockStack.back()[param.name] = param.type;
    }

    typecheckFunctions[name] = {
        .params = typecheckParams,
        .returnType = returnType
    };

    if (block != nullptr)
        block->Typecheck();

    blockStack.pop_back();
    typecheckCurrentFunction = "";
}

void ParsedFile::Typecheck() const
{
    blockStack.push_back({});

    auto int64 = MakeRef<IntegerType>(64, false);
    typecheckFunctions["syscall0"] = {{ int64 }};
    typecheckFunctions["syscall1"] = {{ int64, int64 }};
    typecheckFunctions["syscall2"] = {{ int64, int64, int64 }};
    typecheckFunctions["syscall3"] = {{ int64, int64, int64, int64 }};
    typecheckFunctions["syscall4"] = {{ int64, int64, int64, int64, int64 }};
    typecheckFunctions["syscall5"] = {{ int64, int64, int64, int64, int64, int64 }};
    typecheckFunctions["syscall6"] = {{ int64, int64, int64, int64, int64, int64, int64 }};

    for (const auto& function : functions)
        function->Typecheck();

    blockStack.pop_back();

    assert(blockStack.size() == 0);
}
