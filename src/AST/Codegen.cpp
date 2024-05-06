#include <AST.h>
#include <Context.h>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>

struct VariableInfo
{
    virtual llvm::Value* GetValue() const = 0;
    virtual llvm::Type* GetType() const = 0;
    virtual bool IsConst() const = 0;
};

struct VariableInfoAlloca : public VariableInfo
{
    llvm::AllocaInst* alloc;

    VariableInfoAlloca(llvm::AllocaInst* alloc)
        : alloc(alloc)
    {
    }

    virtual llvm::Value* GetValue() const override
    {
        return alloc;
    }

    virtual llvm::Type* GetType() const override
    {
        return alloc->getAllocatedType();
    }

    virtual bool IsConst() const override
    {
        return false;
    }
};

struct VariableInfoGlobal : public VariableInfo
{
    llvm::GlobalVariable* global;

    VariableInfoGlobal(llvm::GlobalVariable* global)
        : global(global)
    {
    }

    virtual llvm::Value* GetValue() const override
    {
        return global;
    }

    virtual llvm::Type* GetType() const override
    {
        return global->getValueType();
    }

    virtual bool IsConst() const override
    {
        return global->isConstant();
    }
};

static std::vector<std::map<std::string, Ref<VariableInfo>>> blockStack;
[[nodiscard]] static Ref<VariableInfo> FindVariable(const std::string& name, Location location)
{
    for (int i = blockStack.size() - 1; i >= 0; i--)
    {
        const auto& block = blockStack.at(i);
        if (block.contains(name))
            return block.at(name);
    }

    fprintf(stderr, "COMPILER INTERNAL ERROR: Variable not found at codegen stage, this is a typechecker or codegen bug\n");
    exit(1);
}

static bool isInsideFunction = false;

llvm::Value* NumberExpressionAST::Codegen(bool) const
{
    return EvaluateAsConstant();
}

llvm::Value* VariableExpressionAST::Codegen(bool) const
{
    auto var = FindVariable(name, location);
    return g_context->builder->CreateLoad(var->GetType(), var->GetValue(), name);
}

llvm::Value* VariableExpressionAST::RawCodegen() const
{
    if (type->type == TypeEnum::Struct)
        return FindVariable(name, location)->GetValue();
    return Codegen();
}

llvm::Value* StringLiteralAST::Codegen(bool) const
{
    auto value = EvaluateAsConstant();
    return new llvm::GlobalVariable(*g_context->module, value->getType(), true, llvm::GlobalVariable::ExternalLinkage, value);
}

llvm::Value* BinaryExpressionAST::Codegen(bool usedAsStatement) const
{
    static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11, "Not all binary operations are handled in BinaryExpressionAST::Codegen()");

    if (binaryOperation == BinaryOperation::Assignment)
    {
        // FIXME: This needs a rafactor
        if (lhs->type == ExpressionType::Variable)
        {
            auto variableLHS = StaticRefCast<VariableExpressionAST>(lhs);
            g_context->builder->CreateStore(rhs->Codegen(), FindVariable(variableLHS->name, variableLHS->location)->GetValue());
        }
        else if (lhs->type == ExpressionType::ArrayAccess)
        {
            auto arrayLHS = StaticRefCast<ArrayAccessExpressionAST>(lhs);
            auto var = FindVariable(arrayLHS->array->name, arrayLHS->array->location);
            auto gep = g_context->builder->CreateGEP(var->GetType(), var->GetValue(), arrayLHS->index->Codegen(), "gep");
            g_context->builder->CreateStore(rhs->Codegen(), gep);
        }
        else if (lhs->type == ExpressionType::Dereference)
        {
            auto dereferenceLHS = StaticRefCast<DereferenceExpressionAST>(lhs);
            g_context->builder->CreateStore(rhs->Codegen(), dereferenceLHS->pointer->Codegen());
        }
        else if (lhs->type == ExpressionType::MemberAccess)
        {
            auto memberAccessLHS = StaticRefCast<MemberAccessExpressionAST>(lhs);
            assert(memberAccessLHS->object->GetType()->type == TypeEnum::Struct);
            auto structType = StaticRefCast<StructType>(memberAccessLHS->object->GetType());

            uint32_t elementIndex = 0;
            for (const auto& [name, type] : g_context->structs[structType->name].members)
            {
                if (name == memberAccessLHS->memberName)
                    break;
                elementIndex++;
            }
            llvm::Type* elementType = g_context->structs[structType->name].members[memberAccessLHS->memberName]->GetType();

            if (memberAccessLHS->object->type == ExpressionType::Variable)
            {
                auto var = FindVariable(StaticRefCast<VariableExpressionAST>(memberAccessLHS->object)->name, memberAccessLHS->object->location);
                if (var->GetType()->getTypeID() == llvm::Type::TypeID::StructTyID)
                {
                    auto gep = g_context->builder->CreateStructGEP(structType->GetUnderlayingType(), var->GetValue(), elementIndex, "gep");
                    g_context->builder->CreateStore(rhs->Codegen(), gep);
                }
                else if (var->GetType()->getTypeID() == llvm::Type::TypeID::PointerTyID)
                {
                    auto load = g_context->builder->CreateLoad(var->GetType(), var->GetValue(), "load");
                    auto gep = g_context->builder->CreateStructGEP(structType->GetUnderlayingType(), load, elementIndex, "gep");
                    g_context->builder->CreateStore(rhs->Codegen(), gep);
                }
                else
                {
                    assert(false);
                }
            }
            else
            {
                assert(false);
            }
        }
        else
        {
            assert(false);
        }
        if (usedAsStatement)
            return nullptr;
        return lhs->Codegen();
    }
    else if (lhs->GetType()->type == TypeEnum::Integer && rhs->GetType()->type == TypeEnum::Integer)
    {
        assert(*lhs->GetType() == *rhs->GetType());

        auto isLHSSigned = StaticRefCast<IntegerType>(lhs->GetType())->isSigned;

        auto lhsCodegenned = lhs->Codegen();
        auto rhsCodegenned = g_context->builder->CreateIntCast(rhs->Codegen(), lhsCodegenned->getType(), isLHSSigned, "intcast");

        switch (binaryOperation)
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

llvm::Value* CallExpressionAST::Codegen(bool) const
{
    auto function = g_context->module->getFunction(calleeName);
    assert(function);

    std::vector<llvm::Value*> codegennedArgs;
    for (const auto& arg : function->args())
    {
        auto codegennedArg = args[arg.getArgNo()]->RawCodegen();
        if (codegennedArg->getType()->getTypeID() == llvm::Type::TypeID::IntegerTyID && arg.getType()->getTypeID() == llvm::Type::TypeID::IntegerTyID)
            codegennedArg = g_context->builder->CreateIntCast(codegennedArg, arg.getType(), StaticRefCast<IntegerType>(args[arg.getArgNo()]->GetType())->isSigned, "intcast");
        codegennedArgs.push_back(codegennedArg);
    }

    if (returnedType->type == TypeEnum::Void)
    {
        g_context->builder->CreateCall(function, codegennedArgs);
        return nullptr;
    }

    return g_context->builder->CreateCall(function, codegennedArgs, "call");
}

llvm::Value* CastExpressionAST::Codegen(bool) const
{
    auto numberType = reinterpret_cast<IntegerType*>(castedTo.get());
    if (child->GetType()->type == TypeEnum::Integer && castedTo->type == TypeEnum::Integer)
        return g_context->builder->CreateIntCast(child->Codegen(), numberType->GetType(), StaticRefCast<IntegerType>(child->GetType())->isSigned, "intcast");
    else if (child->GetType()->type == TypeEnum::Integer && castedTo->type == TypeEnum::Pointer)
        return g_context->builder->CreateIntToPtr(child->Codegen(), numberType->GetType(), "inttoptr");
    else if (child->GetType()->type == TypeEnum::Pointer && castedTo->type == TypeEnum::Integer)
        return g_context->builder->CreatePtrToInt(child->Codegen(), numberType->GetType(), "ptrtoint");
    else if (child->GetType()->type == TypeEnum::Pointer && castedTo->type == TypeEnum::Pointer)
        return child->Codegen();
    else
        assert(false);
}

llvm::Value* ArrayAccessExpressionAST::Codegen(bool) const
{
    auto var = FindVariable(array->name, location);
    auto gep = g_context->builder->CreateGEP(var->GetType(), var->GetValue(), index->Codegen(), "gep");
    return g_context->builder->CreateLoad(StaticRefCast<ArrayType>(array->GetType())->arrayType->GetType(), gep, array->name);
}

llvm::Value* DereferenceExpressionAST::Codegen(bool) const
{
    return g_context->builder->CreateLoad(StaticRefCast<PointerType>(pointer->GetType())->underlayingType->GetType(), pointer->Codegen(), pointer->name);
}

llvm::Value* MemberAccessExpressionAST::Codegen(bool) const
{
    assert(object->GetType()->type == TypeEnum::Struct);
    auto structType = StaticRefCast<StructType>(object->GetType());

    uint32_t elementIndex = 0;
    for (const auto& [name, type] : g_context->structs[structType->name].members)
    {
        if (name == memberName)
            break;
        elementIndex++;
    }
    llvm::Type* elementType = g_context->structs[structType->name].members[memberName]->GetType();

    if (object->type == ExpressionType::StringLiteral)
    {
        auto gep = g_context->builder->CreateStructGEP(structType->GetUnderlayingType(), StaticRefCast<StringLiteralAST>(object)->Codegen(), elementIndex, "gep");
        return g_context->builder->CreateLoad(elementType, gep, memberName);
    }
    else if (object->type == ExpressionType::Variable)
    {
        auto var = FindVariable(StaticRefCast<VariableExpressionAST>(object)->name, location);
        if (var->GetType()->getTypeID() == llvm::Type::TypeID::StructTyID)
        {
            auto gep = g_context->builder->CreateStructGEP(structType->GetUnderlayingType(), var->GetValue(), elementIndex, "gep");
            return g_context->builder->CreateLoad(elementType, gep, memberName);
        }
        else if (var->GetType()->getTypeID() == llvm::Type::TypeID::PointerTyID)
        {
            auto load = g_context->builder->CreateLoad(var->GetType(), var->GetValue(), "load");
            auto gep = g_context->builder->CreateStructGEP(structType->GetUnderlayingType(), load, elementIndex, "gep");
            return g_context->builder->CreateLoad(elementType, gep, memberName);
        }
        else
        {
            assert(false);
        }
    }
    else
    {
        assert(false);
    }
}

void ReturnStatementAST::Codegen() const
{
    if (!value)
        g_context->builder->CreateRetVoid();
    else if (value->GetType()->type == TypeEnum::Integer)
        g_context->builder->CreateRet(g_context->builder->CreateIntCast(value->Codegen(), returnedType->GetType(), StaticRefCast<IntegerType>(returnedType)->isSigned, "bincast"));
    else
        g_context->builder->CreateRet(value->Codegen());
}

void BlockAST::Codegen() const
{
    blockStack.push_back({});

    for (const auto& statement : statements)
    {
        if (std::holds_alternative<Ref<StatementAST>>(statement))
            std::get<Ref<StatementAST>>(statement)->Codegen();
        else
            std::get<Ref<ExpressionAST>>(statement)->Codegen(true);
    }

    blockStack.pop_back();
}

void IfStatementAST::Codegen() const
{
    auto conditionCodegenned = condition->Codegen();
    if (conditionCodegenned->getType()->getTypeID() != llvm::Type::TypeID::IntegerTyID)
        g_context->Error(condition->location, "Condition must be an integer, this is probably a typechecker bug");
    auto bits = conditionCodegenned->getType()->getIntegerBitWidth();
    auto conditionFinal = g_context->builder->CreateICmpNE(conditionCodegenned, llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(bits, 0)), "ifcmpne");
    
    auto parentFunction = g_context->builder->GetInsertBlock()->getParent();

    auto thenBlock = llvm::BasicBlock::Create(*g_context->llvmContext, "if.then", parentFunction);
    llvm::BasicBlock* elseBlockB;
    if (elseBlock != nullptr)
        elseBlockB = llvm::BasicBlock::Create(*g_context->llvmContext, "if.else");
    auto mergeBlock = llvm::BasicBlock::Create(*g_context->llvmContext, "if.end");

    if (elseBlock != nullptr)
        g_context->builder->CreateCondBr(conditionFinal, thenBlock, elseBlockB);
    else
        g_context->builder->CreateCondBr(conditionFinal, thenBlock, mergeBlock);

    g_context->builder->SetInsertPoint(thenBlock);
    block->Codegen();

    if (!thenBlock->getTerminator())
        g_context->builder->CreateBr(mergeBlock);

    if (elseBlock != nullptr)
    {
        parentFunction->insert(parentFunction->end(), elseBlockB);
        g_context->builder->SetInsertPoint(elseBlockB);
        elseBlock->Codegen();
        if (!elseBlockB->getTerminator())
            g_context->builder->CreateBr(mergeBlock);
    }

    parentFunction->insert(parentFunction->end(), mergeBlock);
    g_context->builder->SetInsertPoint(mergeBlock);
}

void WhileStatementAST::Codegen() const
{
    auto parentFunction = g_context->builder->GetInsertBlock()->getParent();

    auto loopCond = llvm::BasicBlock::Create(*g_context->llvmContext, "while.cond", parentFunction);
    auto loopBody = llvm::BasicBlock::Create(*g_context->llvmContext, "while.body");
    auto loopEnd = llvm::BasicBlock::Create(*g_context->llvmContext, "while.end");

    g_context->builder->CreateBr(loopCond);

    g_context->builder->SetInsertPoint(loopCond);

    auto conditionCodegenned = condition->Codegen();
    if (conditionCodegenned->getType()->getTypeID() != llvm::Type::TypeID::IntegerTyID)
        g_context->Error(condition->location, "Condition must be an integer, this is probably a typechecker bug");
    auto bits = conditionCodegenned->getType()->getIntegerBitWidth();
    auto conditionFinal = g_context->builder->CreateICmpNE(conditionCodegenned, llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(bits, 0)), "whilecmpne");
    g_context->builder->CreateCondBr(conditionFinal, loopBody, loopEnd);

    parentFunction->insert(parentFunction->end(), loopBody);
    g_context->builder->SetInsertPoint(loopBody);

    block->Codegen();
    g_context->builder->CreateBr(loopCond);

    parentFunction->insert(parentFunction->end(), loopEnd);
    g_context->builder->SetInsertPoint(loopEnd);
}

void VariableDefinitionAST::Codegen() const
{
    if (isInsideFunction)
    {
        // FIXME: Do something about local constants
        auto parentFunction = g_context->builder->GetInsertBlock()->getParent();
        llvm::IRBuilder<> functionBeginBuilder(&parentFunction->getEntryBlock(), parentFunction->getEntryBlock().begin());
        auto size = type->type == TypeEnum::Array ? llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(64, StaticRefCast<ArrayType>(type)->size)) : nullptr;

        if (type->type == TypeEnum::Struct)
            blockStack.back()[name] = MakeRef<VariableInfoAlloca>(functionBeginBuilder.CreateAlloca(StaticRefCast<StructType>(type)->GetUnderlayingType(), size, name));
        else
            blockStack.back()[name] = MakeRef<VariableInfoAlloca>(functionBeginBuilder.CreateAlloca(type->GetType(), size, name));

        if (initialValue != nullptr)
            g_context->builder->CreateStore(initialValue->Codegen(), FindVariable(name, location)->GetValue());
    }
    else
    {
        assert(initialValue);
        auto global = new llvm::GlobalVariable(*g_context->module, type->GetType(), isConst, llvm::GlobalValue::ExternalLinkage, initialValue->EvaluateAsConstant(), name);
        blockStack.back()[name] = MakeRef<VariableInfoGlobal>(global);
    }
}

llvm::Function* FunctionAST::Codegen() const
{
    assert(!isInsideFunction);

    isInsideFunction = true;

    std::vector<llvm::Type*> llvmParams;
    for (const auto& param : params)
        llvmParams.push_back(param.type->GetType());

    auto functionType = llvm::FunctionType::get(returnType->GetType(), llvmParams, false);
    auto function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, name, g_context->module.get());

    for (auto& param : function->args())
        param.setName(params[param.getArgNo()].name);

    if (block != nullptr)
    {
        blockStack.push_back({});

        auto basicBlock = llvm::BasicBlock::Create(*g_context->llvmContext, "entry", function);
        g_context->builder->SetInsertPoint(basicBlock);

        for (auto& arg : function->args())
        {
            // FIXME: Array support
            auto alloca = g_context->builder->CreateAlloca(arg.getType(), 0, arg.getName());
            blockStack.back()[arg.getName().str()] = MakeRef<VariableInfoAlloca>(alloca);
            g_context->builder->CreateStore(&arg, alloca);
        }

        block->Codegen();

        if (returnType->type == TypeEnum::Void)
            g_context->builder->CreateRetVoid();

        llvm::verifyFunction(*function);

        if (g_context->optimize)
            g_context->functionPassManager->run(*function, *g_context->functionAnalysisManager);

        blockStack.pop_back();
    }

    isInsideFunction = false;

    return function;
}

void ParsedFile::Codegen() const
{
    blockStack.push_back({});

    for (const auto& variable : globalVariables)
        variable->Codegen();

    for (const auto& function : functions)
        function->Codegen();

    llvm::verifyModule(*g_context->module);

    blockStack.pop_back();

    assert(blockStack.size() == 0);
    assert(!isInsideFunction);
}
