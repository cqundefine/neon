#include <AST.h>
#include <Context.h>
#include <llvm/IR/Type.h>

struct TypecheckAlloca
{
    llvm::Type* type;
};
static std::map<std::string, TypecheckAlloca> typecheckAllocas;

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
    if (!typecheckAllocas.count(name))
        g_context->Error(0, "Unknown variable: %s", name.c_str());
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
    assert(castedTo->getTypeID() == llvm::Type::IntegerTyID && castedTo->getPrimitiveSizeInBits() == 64);
}

void ReturnStatementAST::Typecheck() const
{
    // FIXME: Check if correct type is returned
    value->Typecheck();
}

void BlockAST::Typecheck() const
{
    for (const auto& statement : statements)
    {
        if (std::holds_alternative<std::shared_ptr<StatementAST>>(statement))
            std::get<std::shared_ptr<StatementAST>>(statement)->Typecheck();
        else
            std::get<std::shared_ptr<ExpressionAST>>(statement)->Typecheck();   
    }
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
    if (initialValue->type == ExpressionType::Number)
    {
        assert(type->getTypeID() == llvm::Type::IntegerTyID);
        reinterpret_cast<NumberExpressionAST*>(initialValue.get())->AdjustTypeToBits(reinterpret_cast<llvm::IntegerType*>(type)->getPrimitiveSizeInBits());
    }
    initialValue->Typecheck();
    typecheckAllocas[name] = {
        .type = type
    };
    // Check if default assignment is correct
}

void AssignmentStatementAST::Typecheck() const
{
    value->Typecheck();
    // Try adjust type of number value
    // Check if variable exists in the current scope
    // Check if default assignment is correct
}

void FunctionAST::Typecheck() const
{
    if (block != nullptr)
        block->Typecheck();
    
    std::vector<llvm::Type*> typecheckParams;
    for (const auto& param : params)
        typecheckParams.push_back(param.type);

    typecheckFunctions[name] = {
        .params = typecheckParams
    };
}

void ParsedFile::Typecheck() const
{
    typecheckFunctions["syscall0"] = {{ llvm::Type::getInt64Ty(*g_context->llvmContext) }};
    typecheckFunctions["syscall1"] = {{ llvm::Type::getInt64Ty(*g_context->llvmContext), llvm::Type::getInt64Ty(*g_context->llvmContext) }};
    typecheckFunctions["syscall2"] = {{ llvm::Type::getInt64Ty(*g_context->llvmContext), llvm::Type::getInt64Ty(*g_context->llvmContext), llvm::Type::getInt64Ty(*g_context->llvmContext) }};
    typecheckFunctions["syscall3"] = {{ llvm::Type::getInt64Ty(*g_context->llvmContext), llvm::Type::getInt64Ty(*g_context->llvmContext), llvm::Type::getInt64Ty(*g_context->llvmContext), llvm::Type::getInt64Ty(*g_context->llvmContext) }};

    for (const auto& function : functions)
        function->Typecheck();
}
