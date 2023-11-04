#include <AST.h>
#include <Context.h>
#include <llvm/IR/Type.h>

std::vector<std::map<std::string, Ref<Type>>> typecheckBlockStack;
[[nodiscard]] Ref<Type> FindVariable(const std::string& name)
{
    for (int i = typecheckBlockStack.size() - 1; i >= 0; i--)
    {
        const auto& block = typecheckBlockStack.at(i);
        if (block.contains(name))
            return block.at(name);
    }

    g_context->Error(0, "Can't find variable: %s", name.c_str());
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
    type = FindVariable(name);
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
    
    // Check if variable exists in the current scope
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

void ReturnStatementAST::Typecheck() const
{
    returnedType = FindVariable(typecheckCurrentFunction);

    // FIXME: Check if correct type is returned
    value->Typecheck();
}

void BlockAST::Typecheck() const
{
    typecheckBlockStack.push_back({});

    for (const auto& statement : statements)
    {
        if (std::holds_alternative<Ref<StatementAST>>(statement))
            std::get<Ref<StatementAST>>(statement)->Typecheck();
        else
            std::get<Ref<ExpressionAST>>(statement)->Typecheck();   
    }

    typecheckBlockStack.pop_back();
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
    typecheckBlockStack.back()[name] = type;
    // Check if default assignment is correct
}

void FunctionAST::Typecheck() const
{
    typecheckCurrentFunction = name;

    typecheckBlockStack.back()[name] = returnType;

    if (block != nullptr)
        block->Typecheck();
    
    std::vector<Ref<Type>> typecheckParams;
    for (const auto& param : params)
        typecheckParams.push_back(param.type);

    typecheckFunctions[name] = {
        .params = typecheckParams,
        .returnType = returnType
    };

    typecheckCurrentFunction = "";
}

void ParsedFile::Typecheck() const
{
    typecheckBlockStack.push_back({});

    auto int64 = MakeRef<IntegerType>(64, false);
    typecheckFunctions["syscall0"] = {{ int64 }};
    typecheckFunctions["syscall1"] = {{ int64, int64 }};
    typecheckFunctions["syscall2"] = {{ int64, int64, int64 }};
    typecheckFunctions["syscall3"] = {{ int64, int64, int64, int64 }};

    for (const auto& function : functions)
        function->Typecheck();

    typecheckBlockStack.pop_back();
}
