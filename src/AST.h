#pragma once

#include <Enums.h>
#include <llvm/IR/Value.h>
#include <variant>

// FIXME: Use typedef or using ... = ...
#define ExpressionOrStatement std::variant<std::shared_ptr<StatementAST>, std::shared_ptr<ExpressionAST>>

struct ExpressionAST
{
    virtual void Dump(uint32_t indentCount) const = 0;
    virtual llvm::Value* Codegen() const = 0;
};

struct NumberExpressionAST : public ExpressionAST
{
    uint64_t value;
    NumberType type = NumberType::Int32;

    NumberExpressionAST(uint64_t value);

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
};

struct VariableExpressionAST : public ExpressionAST
{
    std::string name;

    VariableExpressionAST(const std::string& name);

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
};

struct BinaryExpressionAST : public ExpressionAST
{
    std::shared_ptr<ExpressionAST> lhs;
    BinaryOperation binaryOperation;
    std::shared_ptr<ExpressionAST> rhs;

    BinaryExpressionAST(std::shared_ptr<ExpressionAST> lhs, BinaryOperation binaryOperation, std::shared_ptr<ExpressionAST> rhs);

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
};

struct CallExpressionAST : public ExpressionAST
{
    std::string calleeName;
    std::vector<std::shared_ptr<ExpressionAST>> args;
    
    CallExpressionAST(std::string calleeName, std::vector<std::shared_ptr<ExpressionAST>> args);
    
    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
};

struct StatementAST
{
    virtual void Dump(uint32_t indentCount) const = 0;
    virtual void Codegen() const = 0;
};

struct ReturnStatementAST : public StatementAST
{
    std::shared_ptr<ExpressionAST> value;
    
    ReturnStatementAST(std::shared_ptr<ExpressionAST> value);
    
    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
};

struct BlockAST
{
    std::vector<ExpressionOrStatement> statements;

    BlockAST(const std::vector<ExpressionOrStatement>& statements);
    
    void Dump(uint32_t indentCount) const;
    void Codegen() const;
};

struct IfStatementAST : public StatementAST
{
    std::shared_ptr<ExpressionAST> condition;
    std::shared_ptr<BlockAST> block;
    std::shared_ptr<BlockAST> elseBlock;

    IfStatementAST(std::shared_ptr<ExpressionAST> condition, std::shared_ptr<BlockAST> block, std::shared_ptr<BlockAST> elseBlock);
    
    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
};

struct VariableDefinitionAST : public StatementAST
{
    std::string name;
    llvm::Type* type;
    std::shared_ptr<ExpressionAST> initialValue;

    VariableDefinitionAST(const std::string& name, llvm::Type* type, std::shared_ptr<ExpressionAST> initialValue);

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
};

struct AssignmentStatementAST : public StatementAST
{
    std::string name;
    std::shared_ptr<ExpressionAST> value;

    AssignmentStatementAST(std::string name, std::shared_ptr<ExpressionAST> value);

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
};

struct FunctionAST
{
    std::string name;
    std::shared_ptr<BlockAST> block;

    FunctionAST(const std::string& name, std::shared_ptr<BlockAST> block);
    
    void Dump(uint32_t indentCount) const;
    llvm::Function* Codegen() const;
};

struct ParsedFile
{
    std::vector<std::shared_ptr<FunctionAST>> functions;

    ParsedFile(const std::vector<std::shared_ptr<FunctionAST>>& functions);

    void Dump(uint32_t indentCount = 0) const;
    void Codegen() const;
};
