#include "Type.h"
#include <AST.h>
#include <Context.h>
#include <TypeCasts.h>
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

    virtual llvm::Value* GetValue() const override { return alloc; }

    virtual llvm::Type* GetType() const override { return alloc->getAllocatedType(); }

    virtual bool IsConst() const override { return false; }
};

struct VariableInfoGlobal : public VariableInfo
{
    llvm::GlobalVariable* global;

    VariableInfoGlobal(llvm::GlobalVariable* global)
        : global(global)
    {
    }

    virtual llvm::Value* GetValue() const override { return global; }

    virtual llvm::Type* GetType() const override { return global->getValueType(); }

    virtual bool IsConst() const override { return global->isConstant(); }
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

    std::println(std::cerr, "COMPILER ERROR: Variable not found at codegen stage, this is a typechecker or codegen bug");
    exit(1);
}

static bool isInsideFunction = false;
static std::vector<llvm::DIScope*> debugScopes;

llvm::DIScope* GetCurrentScope()
{
    return debugScopes.empty() ? g_context->debugCompileUnit : debugScopes.back();
}

void AST::EmitLocation() const
{
    if (g_context->debug)
    {
        auto scope = GetCurrentScope();
        g_context->builder->SetCurrentDebugLocation(llvm::DILocation::get(scope->getContext(), location.line, location.column, scope));
    }
}

llvm::Value* NumberExpressionAST::Codegen(bool) const
{
    EmitLocation();
    return EvaluateAsConstant();
}

llvm::Value* VariableExpressionAST::Codegen(bool) const
{
    EmitLocation();
    auto var = FindVariable(name, location);
    return g_context->builder->CreateLoad(var->GetType(), var->GetValue(), name);
}

llvm::Value* VariableExpressionAST::RawCodegen() const
{
    EmitLocation();
    if (is<StructType>(type))
        return FindVariable(name, location)->GetValue();
    return Codegen();
}

llvm::Value* StringLiteralAST::Codegen(bool) const
{
    EmitLocation();
    auto value = EvaluateAsConstant();
    return new llvm::GlobalVariable(*g_context->module, value->getType(), true, llvm::GlobalVariable::ExternalLinkage, value);
}

llvm::Value* BinaryExpressionAST::Codegen(bool usedAsStatement) const
{
    EmitLocation();
    static_assert(
        static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11,
        "Not all binary operations are handled in BinaryExpressionAST::Codegen()");

    if (binaryOperation == BinaryOperation::Assignment)
    {
        // FIXME: This needs a rafactor
        auto rhsCodegenned = rhs->Codegen();
        if (auto* lhsInt = as_if<IntegerType>(lhs->GetType()))
            rhsCodegenned = g_context->builder->CreateIntCast(rhsCodegenned, lhs->GetType()->GetType(), lhsInt->isSigned, "intcast");

        if (auto* var = as_if<VariableExpressionAST>(lhs))
        {
            g_context->builder->CreateStore(rhsCodegenned, FindVariable(var->name, var->location)->GetValue());
        }
        else if (auto* array = as_if<ArrayAccessExpressionAST>(lhs))
        {
            auto var = FindVariable(array->array->name, array->array->location);
            auto gep = g_context->builder->CreateGEP(var->GetType(), var->GetValue(), array->index->Codegen(), "gep");
            g_context->builder->CreateStore(rhsCodegenned, gep);
        }
        else if (auto* deref = as_if<DereferenceExpressionAST>(lhs))
        {
            g_context->builder->CreateStore(rhsCodegenned, deref->pointer->Codegen());
        }
        else if (auto* access = as_if<MemberAccessExpressionAST>(lhs))
        {
            auto structType = as<StructType>(access->object->GetType());

            uint32_t elementIndex = 0;
            for (const auto& [name, type] : g_context->structs[structType->name].members)
            {
                if (name == access->memberName)
                    break;
                elementIndex++;
            }
            llvm::Type* elementType = g_context->structs[structType->name].members[access->memberName]->GetType();

            auto* varExpr = as<VariableExpressionAST>(access->object);
            auto var = FindVariable(varExpr->name, access->object->location);
            if (var->GetType()->getTypeID() == llvm::Type::TypeID::StructTyID)
            {
                auto gep = g_context->builder->CreateStructGEP(structType->GetUnderlayingType(), var->GetValue(), elementIndex, "gep");
                g_context->builder->CreateStore(rhsCodegenned, gep);
            }
            else if (var->GetType()->getTypeID() == llvm::Type::TypeID::PointerTyID)
            {
                auto load = g_context->builder->CreateLoad(var->GetType(), var->GetValue(), "load");
                auto gep = g_context->builder->CreateStructGEP(structType->GetUnderlayingType(), load, elementIndex, "gep");
                g_context->builder->CreateStore(rhsCodegenned, gep);
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
    else if (is<IntegerType>(lhs->GetType()) && is<IntegerType>(rhs->GetType()))
    {
        assert(*lhs->GetType() == *rhs->GetType());

        auto isLHSSigned = as<IntegerType>(lhs->GetType())->isSigned;

        auto lhsCodegenned = lhs->Codegen();
        auto rhsCodegenned = g_context->builder->CreateIntCast(rhs->Codegen(), lhsCodegenned->getType(), isLHSSigned, "intcast");

        switch (binaryOperation)
        {
            case BinaryOperation::Add:      return g_context->builder->CreateAdd(lhsCodegenned, rhsCodegenned, "add");
            case BinaryOperation::Subtract: return g_context->builder->CreateSub(lhsCodegenned, rhsCodegenned, "sub");
            case BinaryOperation::Multiply: return g_context->builder->CreateMul(lhsCodegenned, rhsCodegenned, "mul");
            case BinaryOperation::Divide:
                if (isLHSSigned)
                    return g_context->builder->CreateSDiv(lhsCodegenned, rhsCodegenned, "div");
                else
                    return g_context->builder->CreateUDiv(lhsCodegenned, rhsCodegenned, "div");
            case BinaryOperation::Equals:   return g_context->builder->CreateICmpEQ(lhsCodegenned, rhsCodegenned, "eq");
            case BinaryOperation::NotEqual: return g_context->builder->CreateICmpNE(lhsCodegenned, rhsCodegenned, "ne");
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
            default: assert(false);
        }
    }
    else
    {
        assert(false);
    }
}

llvm::Value* CallExpressionAST::Codegen(bool) const
{
    EmitLocation();
    auto function = g_context->module->getFunction(calleeName);
    assert(function);

    std::vector<llvm::Value*> codegennedArgs;
    for (const auto& arg : function->args())
    {
        auto codegennedArg = args[arg.getArgNo()]->RawCodegen();
        if (codegennedArg->getType()->getTypeID() == llvm::Type::TypeID::IntegerTyID &&
            arg.getType()->getTypeID() == llvm::Type::TypeID::IntegerTyID)
            codegennedArg = g_context->builder->CreateIntCast(
                codegennedArg, arg.getType(), as<IntegerType>(args[arg.getArgNo()]->GetType())->isSigned, "intcast");
        codegennedArgs.push_back(codegennedArg);
    }

    if (is<VoidType>(returnedType))
    {
        g_context->builder->CreateCall(function, codegennedArgs);
        return nullptr;
    }

    return g_context->builder->CreateCall(function, codegennedArgs, "call");
}

llvm::Value* CastExpressionAST::Codegen(bool) const
{
    EmitLocation();
    auto numberType = reinterpret_cast<IntegerType*>(castedTo.get());
    if (is<IntegerType>(child->GetType()) && is<IntegerType>(castedTo))
        return g_context->builder->CreateIntCast(
            child->Codegen(), numberType->GetType(), as<IntegerType>(child->GetType())->isSigned, "intcast");
    else if (is<IntegerType>(child->GetType()) && is<PointerType>(castedTo))
        return g_context->builder->CreateIntToPtr(child->Codegen(), numberType->GetType(), "inttoptr");
    else if (is<PointerType>(child->GetType()) && is<IntegerType>(castedTo))
        return g_context->builder->CreatePtrToInt(child->Codegen(), numberType->GetType(), "ptrtoint");
    else if (is<PointerType>(child->GetType()) && is<PointerType>(castedTo))
        return child->Codegen();
    else
        assert(false);
}

llvm::Value* ArrayAccessExpressionAST::Codegen(bool) const
{
    EmitLocation();
    auto var = FindVariable(array->name, location);
    if (is<ArrayType>(array->GetType()))
    {
        auto gep = g_context->builder->CreateGEP(var->GetType(), var->GetValue(), index->Codegen(), "gep");
        return g_context->builder->CreateLoad(GetType()->GetType(), gep, array->name);
    }
    else if (is<PointerType>(array->GetType()))
    {
        auto load = g_context->builder->CreateLoad(var->GetType(), var->GetValue(), "load");
        auto gep = g_context->builder->CreateGEP(load->getType(), load, index->Codegen(), "gep");
        return g_context->builder->CreateLoad(GetType()->GetType(), gep, array->name);
    }
    else
    {
        assert(false);
    }
}

llvm::Value* DereferenceExpressionAST::Codegen(bool) const
{
    EmitLocation();
    return g_context->builder->CreateLoad(
        as<PointerType>(pointer->GetType())->underlayingType->GetType(), pointer->Codegen(), pointer->name);
}

llvm::Value* MemberAccessExpressionAST::Codegen(bool) const
{
    EmitLocation();
    auto structType = as<StructType>(object->GetType());

    uint32_t elementIndex = 0;
    for (const auto& [name, type] : g_context->structs[structType->name].members)
    {
        if (name == memberName)
            break;
        elementIndex++;
    }
    llvm::Type* elementType = g_context->structs[structType->name].members[memberName]->GetType();

    if (auto* str = as_if<StringLiteralAST>(object))
    {
        auto gep = g_context->builder->CreateStructGEP(structType->GetUnderlayingType(), str->Codegen(), elementIndex, "gep");
        return g_context->builder->CreateLoad(elementType, gep, memberName);
    }
    else if (auto* varExpr = as_if<VariableExpressionAST>(object))
    {
        auto var = FindVariable(varExpr->name, location);
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
    EmitLocation();
    if (!value)
        g_context->builder->CreateRetVoid();
    else if (auto* valueInt = as_if<IntegerType>(value->GetType()))
        g_context->builder->CreateRet(
            g_context->builder->CreateIntCast(value->Codegen(), returnedType->GetType(), valueInt->isSigned, "bincast"));
    else
        g_context->builder->CreateRet(value->Codegen());
}

void BlockAST::Codegen() const
{
    EmitLocation();
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
    EmitLocation();
    auto conditionCodegenned = condition->Codegen();
    if (conditionCodegenned->getType()->getTypeID() != llvm::Type::TypeID::IntegerTyID)
        g_context->Error(condition->location, "Condition must be an integer, this is probably a typechecker bug");
    auto bits = conditionCodegenned->getType()->getIntegerBitWidth();
    auto conditionFinal = g_context->builder->CreateICmpNE(
        conditionCodegenned, llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(bits, 0)), "ifcmpne");

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
    EmitLocation();
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
    auto conditionFinal = g_context->builder->CreateICmpNE(
        conditionCodegenned, llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(bits, 0)), "whilecmpne");
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
    EmitLocation();
    if (isInsideFunction)
    {
        // FIXME: Do something about local constants
        auto parentFunction = g_context->builder->GetInsertBlock()->getParent();
        llvm::IRBuilder<> functionBeginBuilder(&parentFunction->getEntryBlock(), parentFunction->getEntryBlock().begin());
        auto size =
            is<ArrayType>(type) ? llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(64, as<ArrayType>(type)->size)) : nullptr;

        if (auto* structType = as_if<StructType>(type))
            blockStack.back()[name] =
                MakeRef<VariableInfoAlloca>(functionBeginBuilder.CreateAlloca(structType->GetUnderlayingType(), size, name));
        else
            blockStack.back()[name] = MakeRef<VariableInfoAlloca>(functionBeginBuilder.CreateAlloca(type->GetType(), size, name));

        if (g_context->debug)
        {
            auto file = location.GetFile().debugFile;
            auto debugLocalVariable =
                g_context->debugBuilder->createAutoVariable(GetCurrentScope(), name, file, location.line, type->GetDebugType(), true);
            g_context->debugBuilder->insertDeclare(
                FindVariable(name, location)->GetValue(),
                debugLocalVariable,
                g_context->debugBuilder->createExpression(),
                llvm::DILocation::get(parentFunction->getContext(), location.line, 0, GetCurrentScope()),
                functionBeginBuilder.GetInsertBlock()); // FIXME: Column
        }

        if (initialValue != nullptr)
        {
            auto initialValueCodegenned = initialValue->Codegen();
            if (auto* intType = as_if<IntegerType>(type))
                initialValueCodegenned =
                    g_context->builder->CreateIntCast(initialValueCodegenned, type->GetType(), intType->isSigned, "intcast");

            g_context->builder->CreateStore(initialValueCodegenned, FindVariable(name, location)->GetValue());
        }
    }
    else
    {
        auto global = new llvm::GlobalVariable(
            *g_context->module,
            type->GetType(),
            isConst,
            llvm::GlobalValue::ExternalLinkage,
            initialValue ? initialValue->EvaluateAsConstant() : type->GetDefaultValue(),
            name);
        blockStack.back()[name] = MakeRef<VariableInfoGlobal>(global);

        if (g_context->debug)
        {
            auto file = location.GetFile().debugFile;
            auto debug = g_context->debugBuilder->createGlobalVariableExpression(
                GetCurrentScope(), name, "", file, location.line, type->GetDebugType(), false);
        }
    }
}

llvm::Function* FunctionAST::Codegen() const
{
    EmitLocation();
    assert(!isInsideFunction);

    isInsideFunction = true;

    std::vector<llvm::Type*> llvmParams;
    std::vector<llvm::Metadata*> debugTypes;

    if (g_context->debug)
        debugTypes.push_back(returnType->GetDebugType());

    for (const auto& param : params)
    {
        llvmParams.push_back(param.type->GetType());
        if (g_context->debug)
            debugTypes.push_back(param.type->GetDebugType());
    }

    auto functionType = llvm::FunctionType::get(returnType->GetType(), llvmParams, false);
    auto function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, name, g_context->module.get());
    for (auto& arg : function->args())
    {
        auto param = params[arg.getArgNo()];
        if (is<StructType>(param.type) && !param.type->isRef)
            arg.addAttr(llvm::Attribute::getWithByValType(*g_context->llvmContext, as<StructType>(param.type)->GetUnderlayingType()));
    }

    for (auto& param : function->args())
        param.setName(params[param.getArgNo()].name);

    llvm::DISubprogram* debugFunction;
    if (g_context->debug)
    {
        auto file = location.GetFile().debugFile;
        debugFunction = g_context->debugBuilder->createFunction(
            file,
            name,
            "",
            file,
            location.line,
            g_context->debugBuilder->createSubroutineType(g_context->debugBuilder->getOrCreateTypeArray(debugTypes)),
            0,
            llvm::DINode::FlagPrototyped,
            llvm::DISubprogram::SPFlagDefinition);
        function->setSubprogram(debugFunction);

        debugScopes.push_back(debugFunction);
        g_context->builder->SetCurrentDebugLocation(llvm::DebugLoc());
    }

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

            if (g_context->debug)
            {
                auto debugLocalVariable = g_context->debugBuilder->createParameterVariable(
                    debugFunction,
                    arg.getName(),
                    arg.getArgNo() + 1,
                    location.GetFile().debugFile,
                    location.line,
                    params.at(arg.getArgNo()).type->GetDebugType(),
                    true);
                g_context->debugBuilder->insertDeclare(
                    alloca,
                    debugLocalVariable,
                    g_context->debugBuilder->createExpression(),
                    llvm::DILocation::get(debugFunction->getContext(), location.line, 0, debugFunction),
                    g_context->builder->GetInsertBlock()); // FIXME: Column
            }

            g_context->builder->CreateStore(&arg, alloca);
        }

        block->Codegen();

        if (is<VoidType>(returnType))
            g_context->builder->CreateRetVoid();

        llvm::verifyFunction(*function);

        if (g_context->optimize)
            g_context->functionPassManager->run(*function, *g_context->functionAnalysisManager);

        blockStack.pop_back();
    }

    if (g_context->debug)
        debugScopes.pop_back();

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

    if (g_context->debug)
        g_context->debugBuilder->finalize();

    llvm::verifyModule(*g_context->module);

    blockStack.pop_back();

    assert(blockStack.size() == 0);
    assert(!isInsideFunction);
}
