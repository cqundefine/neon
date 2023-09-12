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

    assert(false);
}

struct TypecheckFunction
{
    std::vector<llvm::Type*> params;
};
static std::map<std::string, TypecheckFunction> typecheckFunctions;

void NumberExpressionAST::Typecheck() const
{

}

void VariableExpressionAST::Typecheck() const
{
    if (!FindVariable(name))
        g_context->Error(location, "Unknown variable: %s", name.c_str());
}

void StringLiteralAST::Typecheck() const
{

}

void BinaryExpressionAST::Typecheck() const
{
    // FIXME: This is just a very barebones check
    // if (lhs->type != rhs->type)
    //     g_context->Error(0, "Invalid binary operation operand");
}

void CallExpressionAST::Typecheck() const
{
    assert(typecheckFunctions.count(calleeName));
    auto function = typecheckFunctions[calleeName];
    assert(function.params.size() == args.size());
    for (int i = 0; i < args.size(); i++)
    {
        args[i]->Typecheck();
        if (args[i]->type == ExpressionType::Number)
        {
            assert(function.params[i]->getTypeID() == llvm::Type::IntegerTyID);
            reinterpret_cast<NumberExpressionAST*>(args[i].get())->AdjustTypeToBits(reinterpret_cast<llvm::IntegerType*>(function.params[i])->getPrimitiveSizeInBits());
        }
    }
    // FIXME: Check if args are correct
}

void CastExpressionAST::Typecheck() const
{
    assert(child->type == ExpressionType::StringLiteral);
    assert(castedTo->type == TypeEnum::Integer && reinterpret_cast<IntegerType*>(castedTo.get())->bits == 64);
}

void ReturnStatementAST::Typecheck() const
{
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
            reinterpret_cast<NumberExpressionAST*>(initialValue.get())->AdjustTypeToBits(reinterpret_cast<IntegerType*>(type.get())->bits);
        }
        initialValue->Typecheck();
    }
    typecheckBlockStack.back()[name] = type;
    // Check if default assignment is correct
}

void AssignmentStatementAST::Typecheck() const
{
    value->Typecheck();
    
    if(FindVariable(name) != value->GetType())
        g_context->Error(location, "Wrong assignment: tried to assign %s to variable of type %s", value->GetType()->Dump().c_str(), FindVariable(name)->Dump().c_str());

    // Try adjust type of number value
    // Check if variable exists in the current scope
}

void FunctionAST::Typecheck() const
{
    typecheckBlockStack.back()[name] = returnType;

    if (block != nullptr)
        block->Typecheck();
    
    std::vector<llvm::Type*> typecheckParams;
    for (const auto& param : params)
        typecheckParams.push_back(param.type->GetType());

    typecheckFunctions[name] = {
        .params = typecheckParams
    };
}

void ParsedFile::Typecheck() const
{
    typecheckBlockStack.push_back({});

    auto int64 = llvm::Type::getInt64Ty(*g_context->llvmContext);
    typecheckFunctions["syscall0"] = {{ int64 }};
    typecheckFunctions["syscall1"] = {{ int64, int64 }};
    typecheckFunctions["syscall2"] = {{ int64, int64, int64 }};
    typecheckFunctions["syscall3"] = {{ int64, int64, int64, int64 }};

    for (const auto& function : functions)
        function->Typecheck();

    typecheckBlockStack.pop_back();
}
