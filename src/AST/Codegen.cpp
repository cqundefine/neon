#include <AST.h>
#include <Context.h>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>

static std::map<std::string, llvm::AllocaInst*> allocas;

llvm::Value* NumberExpressionAST::Codegen() const
{
    return llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(type->bits, value, type->isSigned));
}

llvm::Value* VariableExpressionAST::Codegen() const
{
    return g_context->builder->CreateLoad(allocas[name]->getAllocatedType(), allocas[name], name);
}

llvm::Value* StringLiteralAST::Codegen() const
{
    std::vector<llvm::Constant *> chars(value.size());
    for(unsigned int i = 0; i < value.size(); i++)
        chars[i] = llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(8, value[i], true));
    auto init = llvm::ConstantArray::get(llvm::ArrayType::get(llvm::Type::getInt8Ty(*g_context->llvmContext), chars.size()), chars);
    llvm::GlobalVariable * v = new llvm::GlobalVariable(*g_context->module, init->getType(), true, llvm::GlobalVariable::ExternalLinkage, init, value);
    return llvm::ConstantExpr::getBitCast(v, llvm::Type::getInt8PtrTy(*g_context->llvmContext));
}

llvm::Value* BinaryExpressionAST::Codegen() const
{
    static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11, "Not all binary operations are handled in BinaryExpressionAST::Codegen()");

    if (binaryOperation == BinaryOperation::Assignment)
    {
        if(lhs->type == ExpressionType::Variable)
        {
            auto variableLHS = StaticRefCast<VariableExpressionAST>(lhs);
            g_context->builder->CreateStore(rhs->Codegen(), allocas[variableLHS->name]);
        }
        else if(lhs->type == ExpressionType::ArrayAccess)
        {
            auto arrayLHS = StaticRefCast<ArrayAccessExpressionAST>(lhs);
            auto gep = g_context->builder->CreateGEP(allocas[arrayLHS->array->name]->getAllocatedType(), allocas[arrayLHS->array->name], arrayLHS->index->Codegen(), "gep");
            g_context->builder->CreateStore(rhs->Codegen(), gep);
        }
        else
        {
            assert(false);
        }
        return nullptr;
        //return lhs->Codegen();
    }
    else if(lhs->GetType()->type == TypeEnum::Integer && rhs->GetType()->type == TypeEnum::Integer)
    {
        assert(*lhs->GetType() == *rhs->GetType());
        
        auto isLHSSigned = StaticRefCast<IntegerType>(lhs->GetType())->isSigned;

        auto lhsCodegenned = lhs->Codegen();
        auto rhsCodegenned = g_context->builder->CreateIntCast(rhs->Codegen(), lhsCodegenned->getType(), isLHSSigned, "intcast");

        switch(binaryOperation)
        {
            case BinaryOperation::Add:
                return g_context->builder->CreateAdd(lhsCodegenned, rhsCodegenned, "add");
            case BinaryOperation::Subtract:
                return g_context->builder->CreateSub(lhsCodegenned, rhsCodegenned, "sub");
            case BinaryOperation::Multiply:
                return g_context->builder->CreateMul(lhsCodegenned, rhsCodegenned, "mul");
            case BinaryOperation::Divide:
                if (isLHSSigned)
                    return g_context->builder->CreateSDiv(lhsCodegenned, rhsCodegenned, "div");
                else
                    return g_context->builder->CreateUDiv(lhsCodegenned, rhsCodegenned, "div");
            case BinaryOperation::Equals:
                return g_context->builder->CreateICmpEQ(lhsCodegenned, rhsCodegenned, "eq");
            case BinaryOperation::NotEqual:
                return g_context->builder->CreateICmpNE(lhsCodegenned, rhsCodegenned, "ne");
            case BinaryOperation::GreaterThan:
                if (isLHSSigned)
                    return g_context->builder->CreateICmpSGT(lhsCodegenned, rhsCodegenned, "gt");
                else
                    return g_context->builder->CreateICmpUGT(lhsCodegenned, rhsCodegenned, "gt");
            case BinaryOperation::GreaterThanOrEqual:
                if (isLHSSigned)
                    return g_context->builder->CreateICmpSGE(lhsCodegenned, rhsCodegenned, "ge");
                else
                    return g_context->builder->CreateICmpUGE(lhsCodegenned, rhsCodegenned, "ge");
            case BinaryOperation::LessThan:
                if (isLHSSigned)
                    return g_context->builder->CreateICmpSLT(lhsCodegenned, rhsCodegenned, "lt");
                else
                    return g_context->builder->CreateICmpULT(lhsCodegenned, rhsCodegenned, "lt");
            case BinaryOperation::LessThanOrEqual:
                if (isLHSSigned)
                    return g_context->builder->CreateICmpSLE(lhsCodegenned, rhsCodegenned, "le");
                else
                    return g_context->builder->CreateICmpULE(lhsCodegenned, rhsCodegenned, "le");
            default:
                assert(false);
        }
    }
    else
    {
        assert(false);
    }
}

llvm::Value* CallExpressionAST::Codegen() const
{
    auto function = g_context->module->getFunction(calleeName);
    assert(function);

    std::vector<llvm::Value*> codegennedArgs;
    for (const auto& arg : args)
        codegennedArgs.push_back(arg->Codegen());

    return g_context->builder->CreateCall(function, codegennedArgs, "call");
}

llvm::Value* CastExpressionAST::Codegen() const
{
    assert(castedTo->type == TypeEnum::Integer);
    auto numberType = reinterpret_cast<IntegerType*>(castedTo.get());
    return g_context->builder->CreateIntCast(child->Codegen(), numberType->GetType(), numberType->isSigned, "cast");
}

llvm::Value* ArrayAccessExpressionAST::Codegen() const
{
    auto gep = g_context->builder->CreateGEP(allocas[array->name]->getAllocatedType(), allocas[array->name], index->Codegen(), "gep");
    return g_context->builder->CreateLoad(StaticRefCast<ArrayType>(array->GetType())->arrayType->GetType(), gep, array->name);
}

void ReturnStatementAST::Codegen() const
{
    if (value->GetType()->type == TypeEnum::Integer)
        g_context->builder->CreateRet(g_context->builder->CreateIntCast(value->Codegen(), returnedType->GetType(), StaticRefCast<IntegerType>(returnedType)->isSigned, "bincast"));
    else
        g_context->builder->CreateRet(value->Codegen());
}

void BlockAST::Codegen() const
{
    for(const auto& statement : statements)
    {
        if (std::holds_alternative<Ref<StatementAST>>(statement))
            std::get<Ref<StatementAST>>(statement)->Codegen();
        else
            std::get<Ref<ExpressionAST>>(statement)->Codegen();
    }
}

void IfStatementAST::Codegen() const
{  
    auto conditionFinal = g_context->builder->CreateICmpNE(condition->Codegen(), llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(1, 0)), "ifcmpne");
    auto parentFunction = g_context->builder->GetInsertBlock()->getParent();
    
    auto thenBlock = llvm::BasicBlock::Create(*g_context->llvmContext, "then", parentFunction);
    llvm::BasicBlock* elseBlockB;
    if (elseBlock != nullptr)
        elseBlockB = llvm::BasicBlock::Create(*g_context->llvmContext, "else");
    auto mergeBlock = llvm::BasicBlock::Create(*g_context->llvmContext, "merge");

    if (elseBlock != nullptr)
        g_context->builder->CreateCondBr(conditionFinal, thenBlock, elseBlockB);
    else
        g_context->builder->CreateCondBr(conditionFinal, thenBlock, mergeBlock);

    g_context->builder->SetInsertPoint(thenBlock);
    block->Codegen();
    g_context->builder->CreateBr(mergeBlock);

    if (elseBlock != nullptr)
    {
        parentFunction->getBasicBlockList().push_back(elseBlockB);
        g_context->builder->SetInsertPoint(elseBlockB);
        elseBlock->Codegen();
        g_context->builder->CreateBr(mergeBlock);
    }

    parentFunction->getBasicBlockList().push_back(mergeBlock);
    g_context->builder->SetInsertPoint(mergeBlock);
}

void WhileStatementAST::Codegen() const
{  
    auto parentFunction = g_context->builder->GetInsertBlock()->getParent();
    
    auto loopCond = llvm::BasicBlock::Create(*g_context->llvmContext, "loopCond", parentFunction);
    auto loopBody = llvm::BasicBlock::Create(*g_context->llvmContext, "loopBody");
    auto loopEnd = llvm::BasicBlock::Create(*g_context->llvmContext, "loopEnd");

    g_context->builder->CreateBr(loopCond);

    g_context->builder->SetInsertPoint(loopCond);
    
    auto conditionFinal = g_context->builder->CreateICmpNE(condition->Codegen(), llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(1, 0)), "whilecmpne");
    g_context->builder->CreateCondBr(conditionFinal, loopBody, loopEnd);
    
    parentFunction->getBasicBlockList().push_back(loopBody);
    g_context->builder->SetInsertPoint(loopBody);

    block->Codegen();
    g_context->builder->CreateBr(loopCond);

    parentFunction->getBasicBlockList().push_back(loopEnd);
    g_context->builder->SetInsertPoint(loopEnd);
}

void VariableDefinitionAST::Codegen() const
{
    auto parentFunction = g_context->builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> functionBeginBuilder(&parentFunction->getEntryBlock(), parentFunction->getEntryBlock().begin());
    auto size = type->type == TypeEnum::Array ? llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(64, StaticRefCast<ArrayType>(type)->size)) : nullptr;
    allocas[name] = functionBeginBuilder.CreateAlloca(type->GetType(), size, name);
    if (initialValue != nullptr)
        g_context->builder->CreateStore(initialValue->Codegen(), allocas[name]);
}

llvm::Function* FunctionAST::Codegen() const
{
    std::vector<llvm::Type*> llvmParams;
    for (const auto& param : params)
        llvmParams.push_back(param.type->GetType());

    auto functionType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*g_context->llvmContext), llvmParams, false);
    auto function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, name, g_context->module.get());
    
    if (block != nullptr)
    {
        auto basicBlock = llvm::BasicBlock::Create(*g_context->llvmContext, "entry", function);
        g_context->builder->SetInsertPoint(basicBlock);

        block->Codegen();

        llvm::verifyFunction(*function);

        // g_context->functionPassManager->run(*function);
    }

    return function;
}

void ParsedFile::Codegen() const
{
    for (const auto& function : functions)
        function->Codegen();
}
