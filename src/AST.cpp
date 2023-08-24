#include <AST.h>
#include <Context.h>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>

// -------------------------
// Constructors
// -------------------------

NumberExpressionAST::NumberExpressionAST(uint64_t value) : value(value) {}
VariableExpressionAST::VariableExpressionAST(const std::string& name) : name(name) {}
StringLiteralAST::StringLiteralAST(const std::string& value) : value(value) {}
BinaryExpressionAST::BinaryExpressionAST(std::shared_ptr<ExpressionAST> lhs, BinaryOperation binaryOperation, std::shared_ptr<ExpressionAST> rhs) : lhs(lhs), binaryOperation(binaryOperation), rhs(rhs) {}
CallExpressionAST::CallExpressionAST(std::string calleeName, std::vector<std::shared_ptr<ExpressionAST>> args) : calleeName(calleeName), args(args) {}
ReturnStatementAST::ReturnStatementAST(std::shared_ptr<ExpressionAST> value) : value(value) {}
BlockAST::BlockAST(const std::vector<ExpressionOrStatement>& statements) : statements(statements) {}
IfStatementAST::IfStatementAST(std::shared_ptr<ExpressionAST> condition, std::shared_ptr<BlockAST> block, std::shared_ptr<BlockAST> elseBlock) : condition(condition), block(block), elseBlock(elseBlock) {}
VariableDefinitionAST::VariableDefinitionAST(const std::string& name, llvm::Type* type, std::shared_ptr<ExpressionAST> initialValue) : name(name), type(type), initialValue(initialValue) {}
AssignmentStatementAST::AssignmentStatementAST(std::string name, std::shared_ptr<ExpressionAST> value) : name(name), value(value) {}
FunctionAST::FunctionAST(const std::string& name, std::vector<Param> params, llvm::Type* returnType, std::shared_ptr<BlockAST> block) : name(name), params(params), returnType(returnType), block(block) {}
ParsedFile::ParsedFile(const std::vector<std::shared_ptr<FunctionAST>>& functions) : functions(functions) {}

// -------------------------
// Dump
// -------------------------

static std::map<std::string, llvm::AllocaInst*> allocas;

#define INDENT(count) for (uint32_t i = 0; i < count; i++) { printf("    "); }

void NumberExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Number Expression (`%lu`, %s)\n", value, NumberTypeToString(type).c_str());
}

void VariableExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Variable Expression (`%s`)\n", name.c_str());
}

void StringLiteralAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("String Literal (`%s`)\n", value.c_str());
}

void BinaryExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("Binary Expression");
    lhs->Dump(indentCount + 1);
    INDENT(indentCount + 1);
    puts(BinaryOperationToString(binaryOperation).c_str());
    rhs->Dump(indentCount + 1);
}

void CallExpressionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Call Expression (`%s`)\n", calleeName.c_str());
    for (const auto& arg : args)
        arg->Dump(indentCount + 1);
}

void ReturnStatementAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("ReturnStatement");
    value->Dump(indentCount + 1);
}

void BlockAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("Block");

    for(const auto& statement : statements)
    {
        if (std::holds_alternative<std::shared_ptr<StatementAST>>(statement))
            std::get<std::shared_ptr<StatementAST>>(statement)->Dump(indentCount + 1);
        else
            std::get<std::shared_ptr<ExpressionAST>>(statement)->Dump(indentCount + 1);
    }
}

void IfStatementAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("If Statement");
    condition->Dump(indentCount + 1);
    block->Dump(indentCount + 1);
    if (elseBlock != nullptr)
        elseBlock->Dump(indentCount + 1);
}

void VariableDefinitionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Variable Declaration (`%s`, `%u`)\n", name.c_str(), type->getTypeID());
    initialValue->Dump(indentCount + 1);
}

void AssignmentStatementAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Assignment Statement (`%s`)\n", name.c_str());
    value->Dump(indentCount + 1);
}

void FunctionAST::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    printf("Function (`%s`)\n", name.c_str());
    for (const auto& param : params)
    {
        INDENT(indentCount + 1);
        printf("Param ('%s', '%u')\n", param.name.c_str(), param.type->getTypeID());
    }
    if (block != nullptr)
        block->Dump(indentCount + 1);
}

void ParsedFile::Dump(uint32_t indentCount) const
{
    INDENT(indentCount);
    puts("Parsed File");
    for (const auto& function : functions)
        function->Dump(indentCount + 1);
}

// -------------------------
// Codegen
// -------------------------

llvm::Value* NumberExpressionAST::Codegen() const
{
    static_assert(static_cast<uint32_t>(NumberType::_NumberTypeCount) == 8, "Not all number types are handled in NumberExpressionAST::Codegen()");

    llvm::APInt apInt;

    switch (type)
    {
    case NumberType::Int8:
        apInt = llvm::APInt(8, value, true);
        break;
    case NumberType::UInt8:
        apInt = llvm::APInt(8, value, false);
        break;
    case NumberType::Int16:
        apInt = llvm::APInt(16, value, true);
        break;
    case NumberType::UInt16:
        apInt = llvm::APInt(16, value, false);
        break;
    case NumberType::Int32:
        apInt = llvm::APInt(32, value, true);
        break;
    case NumberType::UInt32:
        apInt = llvm::APInt(32, value, false);
        break;
    case NumberType::Int64:
        apInt = llvm::APInt(64, value, true);
        break;
    case NumberType::UInt64:
        apInt = llvm::APInt(64, value, false);
        break;
    default:
        assert(false);
    }

    return llvm::ConstantInt::get(*g_context->llvmContext, apInt);
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
    static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 5, "Not all binary operations are handled in BinaryExpressionAST::Codegen()");

    switch(binaryOperation)
    {
        case BinaryOperation::Add:
            return g_context->builder->CreateAdd(lhs->Codegen(), rhs->Codegen());
        case BinaryOperation::Subtract:
            return g_context->builder->CreateSub(lhs->Codegen(), rhs->Codegen());
        case BinaryOperation::Multiply:
            return g_context->builder->CreateMul(lhs->Codegen(), rhs->Codegen());
        case BinaryOperation::Divide:
            return g_context->builder->CreateSDiv(lhs->Codegen(), rhs->Codegen());
        case BinaryOperation::Equals: {
            return g_context->builder->CreateICmpEQ(lhs->Codegen(), rhs->Codegen()); }
        default:
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

    return g_context->builder->CreateCall(function, codegennedArgs);
}

void ReturnStatementAST::Codegen() const
{
    g_context->builder->CreateRet(value->Codegen());
}

void BlockAST::Codegen() const
{
    for(const auto& statement : statements)
    {
        if (std::holds_alternative<std::shared_ptr<StatementAST>>(statement))
            std::get<std::shared_ptr<StatementAST>>(statement)->Codegen();
        else
            std::get<std::shared_ptr<ExpressionAST>>(statement)->Codegen();
    }
}

void IfStatementAST::Codegen() const
{  
    auto conditionFinal = g_context->builder->CreateICmpNE(condition->Codegen(), llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(1, 0)));
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

void VariableDefinitionAST::Codegen() const
{
    auto parentFunction = g_context->builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> functionBeginBuilder(&parentFunction->getEntryBlock(), parentFunction->getEntryBlock().begin());
    allocas[name] = functionBeginBuilder.CreateAlloca(type, nullptr, name);
    if (initialValue != nullptr)
        g_context->builder->CreateStore(initialValue->Codegen(), allocas[name]);
}

void AssignmentStatementAST::Codegen() const
{
    g_context->builder->CreateStore(value->Codegen(), allocas[name]);
}

llvm::Function* FunctionAST::Codegen() const
{
    std::vector<llvm::Type*> llvmParams;
    for (const auto& param : params)
        llvmParams.push_back(param.type);

    auto functionType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*g_context->llvmContext), llvmParams, false);
    auto function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, name, g_context->module.get());
    
    if (block != nullptr)
    {
        auto basicBlock = llvm::BasicBlock::Create(*g_context->llvmContext, "entry", function);
        g_context->builder->SetInsertPoint(basicBlock);

        block->Codegen();

        llvm::verifyFunction(*function);

        //g_context->functionPassManager->run(*function);
    }

    return function;
}

void ParsedFile::Codegen() const
{
    for (const auto& function : functions)
        function->Codegen();
}
