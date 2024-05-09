#pragma once

#include <Context.h>
#include <Enums.h>
#include <Type.h>
#include <Utils.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Value.h>
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
    ArrayAccess,
    Dereference,
    MemberAccess
};

enum class StatementType
{
    Return,
    Block,
    If,
    While,
    VariableDefinition
};

struct AST
{
    Location location;

    inline AST(Location location)
        : location(location)
    {
    }

    void EmitLocation() const;
};

struct ExpressionAST : public AST
{
    ExpressionType type;

    inline ExpressionAST(Location location, ExpressionType type)
        : AST(location)
        , type(type)
    {
    }

    virtual void Dump(uint32_t indentCount) const = 0;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const = 0;
    virtual llvm::Value* RawCodegen() const { return Codegen(); }
    virtual void Typecheck() const = 0;
    virtual Ref<Type> GetType() const = 0;
    virtual llvm::Constant* EvaluateAsConstant() const { g_context->Error(location, "Expression is not constant"); }
};

struct NumberExpressionAST : public ExpressionAST
{
    uint64_t value;
    Ref<IntegerType> type;

    inline NumberExpressionAST(Location location, uint64_t value, Ref<IntegerType> type)
        : ExpressionAST(location, ExpressionType::Number)
        , value(value)
        , type(type)
    {
    }

    void AdjustType(Ref<IntegerType> type);

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return type; }
    virtual llvm::Constant* EvaluateAsConstant() const override;
};

struct VariableExpressionAST : public ExpressionAST
{
    std::string name;

    // Assigned during typechecking
    mutable Ref<Type> type;

    inline VariableExpressionAST(Location location, const std::string& name)
        : ExpressionAST(location, ExpressionType::Variable)
        , name(name)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual llvm::Value* RawCodegen() const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return type; }
};

struct StringLiteralAST : public ExpressionAST
{
    std::string value;

    inline StringLiteralAST(Location location, const std::string& value)
        : ExpressionAST(location, ExpressionType::StringLiteral)
        , value(value)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return MakeRef<StructType>("string"); }
    virtual llvm::Constant* EvaluateAsConstant() const override;
};

struct BinaryExpressionAST : public ExpressionAST
{
    Ref<ExpressionAST> lhs;
    BinaryOperation binaryOperation;
    Ref<ExpressionAST> rhs;

    inline BinaryExpressionAST(Location location, Ref<ExpressionAST> lhs, BinaryOperation binaryOperation, Ref<ExpressionAST> rhs)
        : ExpressionAST(location, ExpressionType::Binary)
        , lhs(lhs)
        , binaryOperation(binaryOperation)
        , rhs(rhs)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return lhs->GetType(); }
};

struct CallExpressionAST : public ExpressionAST
{
    std::string calleeName;
    std::vector<Ref<ExpressionAST>> args;

    // Assigned during typechecking
    mutable Ref<Type> returnedType;

    inline CallExpressionAST(Location location, std::string calleeName, std::vector<Ref<ExpressionAST>> args)
        : ExpressionAST(location, ExpressionType::Call)
        , calleeName(calleeName)
        , args(args)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return returnedType; }
};

struct CastExpressionAST : public ExpressionAST
{
    Ref<Type> castedTo;
    Ref<ExpressionAST> child;

    inline CastExpressionAST(Location location, Ref<Type> castedTo, Ref<ExpressionAST> child)
        : ExpressionAST(location, ExpressionType::Cast)
        , castedTo(castedTo)
        , child(child)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override { return castedTo; }
};

struct ArrayAccessExpressionAST : public ExpressionAST
{
    Ref<VariableExpressionAST> array;
    Ref<ExpressionAST> index;

    inline ArrayAccessExpressionAST(Location location, Ref<VariableExpressionAST> array, Ref<ExpressionAST> index)
        : ExpressionAST(location, ExpressionType::ArrayAccess)
        , array(array)
        , index(index)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override
    {
        if (array->type->type == TypeEnum::Array)
            return StaticRefCast<ArrayType>(array->type)->arrayType;
        else if (array->type->type == TypeEnum::Pointer)
            return StaticRefCast<PointerType>(array->type)->underlayingType;
        else
            assert(false);
    }
};

struct DereferenceExpressionAST : public ExpressionAST
{
    Ref<VariableExpressionAST> pointer;

    inline DereferenceExpressionAST(Location location, Ref<VariableExpressionAST> pointer)
        : ExpressionAST(location, ExpressionType::Dereference)
        , pointer(pointer)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override
    {
        // This should get cought by the typechecker
        assert(pointer->type->type == TypeEnum::Pointer);
        return StaticRefCast<PointerType>(pointer->type)->underlayingType;
    }
};

struct MemberAccessExpressionAST : public ExpressionAST
{
    Ref<ExpressionAST> object;
    std::string memberName;

    inline MemberAccessExpressionAST(Location location, Ref<ExpressionAST> object, const std::string& memberName)
        : ExpressionAST(location, ExpressionType::MemberAccess)
        , object(object)
        , memberName(memberName)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() const override;
    virtual inline Ref<Type> GetType() const override
    {
        assert(object->GetType()->type == TypeEnum::Struct);
        return g_context->structs.at(StaticRefCast<StructType>(object->GetType())->name).members[memberName];
    }
};

struct StatementAST : public AST
{
    StatementType type;

    inline StatementAST(Location location, StatementType type)
        : AST(location)
        , type(type)
    {
    }

    virtual void Dump(uint32_t indentCount) const = 0;
    virtual void Codegen() const = 0;
    virtual void Typecheck() const = 0;
};

struct ReturnStatementAST : public StatementAST
{
    Ref<ExpressionAST> value;

    // Assigned during typechecking
    mutable Ref<Type> returnedType;

    inline ReturnStatementAST(Location location, Ref<ExpressionAST> value)
        : StatementAST(location, StatementType::Return)
        , value(value)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() const override;
};

struct BlockAST : public AST
{
    std::vector<ExpressionOrStatement> statements;

    inline BlockAST(Location location, const std::vector<ExpressionOrStatement>& statements)
        : AST(location)
        , statements(statements)
    {
    }

    void Dump(uint32_t indentCount) const;
    void Codegen() const;
    void Typecheck() const;
};

struct IfStatementAST : public StatementAST
{
    Ref<ExpressionAST> condition;
    Ref<BlockAST> block;
    Ref<BlockAST> elseBlock;

    inline IfStatementAST(Location location, Ref<ExpressionAST> condition, Ref<BlockAST> block, Ref<BlockAST> elseBlock)
        : StatementAST(location, StatementType::If)
        , condition(condition)
        , block(block)
        , elseBlock(elseBlock)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() const override;
};

struct WhileStatementAST : public StatementAST
{
    Ref<ExpressionAST> condition;
    Ref<BlockAST> block;

    inline WhileStatementAST(Location location, Ref<ExpressionAST> condition, Ref<BlockAST> block)
        : StatementAST(location, StatementType::While)
        , condition(condition)
        , block(block)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() const override;
};

struct VariableDefinitionAST : public StatementAST
{
    std::string name;
    Ref<Type> type;
    bool isConst;
    Ref<ExpressionAST> initialValue;

    inline VariableDefinitionAST(Location location, const std::string& name, Ref<Type> type, bool isConst, Ref<ExpressionAST> initialValue)
        : StatementAST(location, StatementType::VariableDefinition)
        , name(name)
        , type(type)
        , isConst(isConst)
        , initialValue(initialValue)
    {
    }

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

    inline FunctionAST(Location location, const std::string& name, std::vector<Param> params, Ref<Type> returnType, Ref<BlockAST> block)
        : AST(location)
        , name(name)
        , params(params)
        , returnType(returnType)
        , block(block)
    {
    }

    void Dump(uint32_t indentCount) const;
    llvm::Function* Codegen() const;
    void Typecheck() const;
};

struct ParsedFile
{
    std::vector<Ref<FunctionAST>> functions;
    std::vector<Ref<VariableDefinitionAST>> globalVariables;

    inline ParsedFile(const std::vector<Ref<FunctionAST>>& functions, const std::vector<Ref<VariableDefinitionAST>>& globalVariables)
        : functions(functions)
        , globalVariables(globalVariables)
    {
    }

    void Dump(uint32_t indentCount = 0) const;
    void Codegen() const;
    void Typecheck() const;
};
