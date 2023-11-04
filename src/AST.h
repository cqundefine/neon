#pragma once

#include <Enums.h>
#include <llvm/IR/Value.h>
#include <Type.h>
#include <Utils.h>
#include <variant>

// FIXME: Use typedef or using ... = ...
#define ExpressionOrStatement std::variant<Ref<StatementAST>, Ref<ExpressionAST>>

enum class ExpressionType
{
    Number,
    Variable,
    StringLiteral,
    Binary,
    Call,
    Cast,
    ArrayAccess
};

struct AST
{
    Location location;

    inline AST(Location location) : location(location) {}
};

struct ExpressionAST : public AST
{
    ExpressionType type;

    inline ExpressionAST(Location location, ExpressionType type) : AST(location), type(type) {}

    virtual void Dump(uint32_t indentCount) const = 0;
    virtual llvm::Value* Codegen() const = 0;
    virtual void Typecheck() const = 0;
    virtual Ref<Type> GetType() const = 0;
};

struct NumberExpressionAST : public ExpressionAST
{
    uint64_t value;
    Ref<IntegerType> type;

    inline NumberExpressionAST(Location location, uint64_t value, Ref<IntegerType> type) : ExpressionAST(location, ExpressionType::Number), value(value), type(type) {}

    void AdjustType(Ref<IntegerType> type);

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return type; }
};

struct VariableExpressionAST : public ExpressionAST
{
    std::string name;

    // Assigned during typechecking
    mutable Ref<Type> type;

    inline VariableExpressionAST(Location location, const std::string& name) : ExpressionAST(location, ExpressionType::Variable), name(name) {}

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return type; }
};

struct StringLiteralAST : public ExpressionAST
{
    std::string value;

    inline StringLiteralAST(Location location, const std::string& value) : ExpressionAST(location, ExpressionType::StringLiteral), value(value) {}

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return MakeRef<StringType>(); }
};

struct BinaryExpressionAST : public ExpressionAST
{
    Ref<ExpressionAST> lhs;
    BinaryOperation binaryOperation;
    Ref<ExpressionAST> rhs;

    inline BinaryExpressionAST(Location location, Ref<ExpressionAST> lhs, BinaryOperation binaryOperation, Ref<ExpressionAST> rhs) : ExpressionAST(location, ExpressionType::Binary), lhs(lhs), binaryOperation(binaryOperation), rhs(rhs) {}

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return lhs->GetType(); }
};

struct CallExpressionAST : public ExpressionAST
{
    std::string calleeName;
    std::vector<Ref<ExpressionAST>> args;

    // Assigned during typechecking
    mutable Ref<Type> returnedType;
    
    inline CallExpressionAST(Location location, std::string calleeName, std::vector<Ref<ExpressionAST>> args) : ExpressionAST(location, ExpressionType::Call), calleeName(calleeName), args(args) {}
    
    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return returnedType; }
};

struct CastExpressionAST : public ExpressionAST
{
    Ref<Type> castedTo;
    Ref<ExpressionAST> child;

    inline CastExpressionAST(Location location, Ref<Type> castedTo, Ref<ExpressionAST> child) : ExpressionAST(location, ExpressionType::Cast), castedTo(castedTo), child(child) {}
    
    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return castedTo; }
};

struct ArrayAccessExpressionAST : public ExpressionAST
{
    Ref<VariableExpressionAST> array;
    Ref<NumberExpressionAST> index;
    
    inline ArrayAccessExpressionAST(Location location, Ref<VariableExpressionAST> array, Ref<NumberExpressionAST> index) : ExpressionAST(location, ExpressionType::ArrayAccess), array(array), index(index) {}

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen() const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return StaticRefCast<ArrayType>(array->type)->arrayType; }
};

struct StatementAST : public AST
{
    inline StatementAST(Location location) : AST(location) {}

    virtual void Dump(uint32_t indentCount) const = 0;
    virtual void Codegen() const = 0;
    virtual void Typecheck() const = 0;
};

struct ReturnStatementAST : public StatementAST
{
    Ref<ExpressionAST> value;
    
    // Assigned during typechecking
    mutable Ref<Type> returnedType;
    
    inline ReturnStatementAST(Location location, Ref<ExpressionAST> value) : StatementAST(location), value(value) {}
    
    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() const override;
};

struct BlockAST : public AST
{
    std::vector<ExpressionOrStatement> statements;

    inline BlockAST(Location location, const std::vector<ExpressionOrStatement>& statements) : AST(location), statements(statements) {}
    
    void Dump(uint32_t indentCount) const;
    void Codegen() const;
    void Typecheck() const;
};

struct IfStatementAST : public StatementAST
{
    Ref<ExpressionAST> condition;
    Ref<BlockAST> block;
    Ref<BlockAST> elseBlock;

    inline IfStatementAST(Location location, Ref<ExpressionAST> condition, Ref<BlockAST> block, Ref<BlockAST> elseBlock) : StatementAST(location), condition(condition), block(block), elseBlock(elseBlock) {}
    
    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() const override;
};

struct WhileStatementAST : public StatementAST
{
    Ref<ExpressionAST> condition;
    Ref<BlockAST> block;

    inline WhileStatementAST(Location location, Ref<ExpressionAST> condition, Ref<BlockAST> block) : StatementAST(location), condition(condition), block(block) {}
    
    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() const override;
};

struct VariableDefinitionAST : public StatementAST
{
    std::string name;
    Ref<Type> type;
    Ref<ExpressionAST> initialValue;

    inline VariableDefinitionAST(Location location, const std::string& name, Ref<Type> type, Ref<ExpressionAST> initialValue) : StatementAST(location), name(name), type(type), initialValue(initialValue) {}

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() const override;
};

struct FunctionAST : public AST
{
    struct Param
    {
        std::string name;
        Ref<Type> type;
    };

    std::string name;
    std::vector<Param> params;
    Ref<Type> returnType;
    Ref<BlockAST> block;

    inline FunctionAST(Location location, const std::string& name, std::vector<Param> params, Ref<Type> returnType, Ref<BlockAST> block) : AST(location), name(name), params(params), returnType(returnType), block(block) {}
    
    void Dump(uint32_t indentCount) const;
    llvm::Function* Codegen() const;
    void Typecheck() const;
};

struct ParsedFile
{
    std::vector<Ref<FunctionAST>> functions;

    inline ParsedFile(const std::vector<Ref<FunctionAST>>& functions) : functions(functions) {}

    void Dump(uint32_t indentCount = 0) const;
    void Codegen() const;
    void Typecheck() const;
};
