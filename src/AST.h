#pragma once

#include <Context.h>
#include <Enums.h>
#include <Type.h>
#include <TypeCasts.h>
#include <Utils.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Value.h>
#include <variant>

// FIXME: Use typedef or using ... = ...
#define ExpressionOrStatement std::variant<Ref<StatementAST>, Ref<ExpressionAST>>

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
    inline ExpressionAST(Location location)
        : AST(location)
    {
    }

    virtual void Dump(uint32_t indentCount) const = 0;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const = 0;
    virtual llvm::Value* RawCodegen() const { return Codegen(); }
    virtual void Typecheck() = 0;
    virtual Ref<Type> GetType() const = 0;
    virtual void DCE() const = 0;
    virtual llvm::Constant* EvaluateAsConstant() const { g_context->Error(location, "Expression is not constant"); }
};

struct NumberExpressionAST : public ExpressionAST
{
    uint64_t value;
    Ref<IntegerType> type;

    inline NumberExpressionAST(Location location, uint64_t value, Ref<IntegerType> type)
        : ExpressionAST(location)
        , value(value)
        , type(type)
    {
    }

    void AdjustType(Ref<IntegerType> type);

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
    virtual inline Ref<Type> GetType() const override { return type; }
    virtual llvm::Constant* EvaluateAsConstant() const override;
};

struct VariableExpressionAST : public ExpressionAST
{
    std::string name;

    // Assigned during typechecking
    mutable Ref<Type> type;

    inline VariableExpressionAST(Location location, const std::string& name)
        : ExpressionAST(location)
        , name(name)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual llvm::Value* RawCodegen() const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
    virtual inline Ref<Type> GetType() const override { return type; }
};

struct StringLiteralAST : public ExpressionAST
{
    std::string value;
    Ref<StructType> type;

    inline StringLiteralAST(Location location, const std::string& value)
        : ExpressionAST(location)
        , value(value)
        , type(MakeRef<StructType>("string"))
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() override;
    virtual inline Ref<Type> GetType() const override { return type; }
    virtual void DCE() const override;
    virtual llvm::Constant* EvaluateAsConstant() const override;
};

struct BinaryExpressionAST : public ExpressionAST
{
    Ref<ExpressionAST> lhs;
    BinaryOperation binaryOperation;
    Ref<ExpressionAST> rhs;

    inline BinaryExpressionAST(Location location, Ref<ExpressionAST> lhs, BinaryOperation binaryOperation, Ref<ExpressionAST> rhs)
        : ExpressionAST(location)
        , lhs(lhs)
        , binaryOperation(binaryOperation)
        , rhs(rhs)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
    virtual inline Ref<Type> GetType() const override { return lhs->GetType(); }
};

struct CallExpressionAST : public ExpressionAST
{
    std::string calleeName;
    std::vector<Ref<ExpressionAST>> args;

    // Assigned during typechecking
    mutable Ref<Type> returnedType;

    inline CallExpressionAST(Location location, std::string calleeName, std::vector<Ref<ExpressionAST>> args)
        : ExpressionAST(location)
        , calleeName(calleeName)
        , args(args)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
    virtual inline Ref<Type> GetType() const override { return returnedType; }
};

struct CastExpressionAST : public ExpressionAST
{
    Ref<Type> castedTo;
    Ref<ExpressionAST> child;

    inline CastExpressionAST(Location location, Ref<Type> castedTo, Ref<ExpressionAST> child)
        : ExpressionAST(location)
        , castedTo(castedTo)
        , child(child)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
    virtual inline Ref<Type> GetType() const override { return castedTo; }
};

struct ArrayAccessExpressionAST : public ExpressionAST
{
    Ref<VariableExpressionAST> array;
    Ref<ExpressionAST> index;

    inline ArrayAccessExpressionAST(Location location, Ref<VariableExpressionAST> array, Ref<ExpressionAST> index)
        : ExpressionAST(location)
        , array(array)
        , index(index)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;

    virtual inline Ref<Type> GetType() const override
    {
        if (auto* arr = as_if<ArrayType>(array->type))
            return arr->arrayType;
        else if (auto* ptr = as_if<PointerType>(array->type))
            return ptr->underlayingType;
        else
            assert(false);
    }
};

struct DereferenceExpressionAST : public ExpressionAST
{
    Ref<VariableExpressionAST> pointer;

    inline DereferenceExpressionAST(Location location, Ref<VariableExpressionAST> pointer)
        : ExpressionAST(location)
        , pointer(pointer)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;

    virtual inline Ref<Type> GetType() const override { return as<PointerType>(pointer->type.get())->underlayingType; }
};

struct MemberAccessExpressionAST : public ExpressionAST
{
    Ref<ExpressionAST> object;
    std::string memberName;

    inline MemberAccessExpressionAST(Location location, Ref<ExpressionAST> object, const std::string& memberName)
        : ExpressionAST(location)
        , object(object)
        , memberName(memberName)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual llvm::Value* Codegen(bool usedAsStatement = false) const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
    virtual inline Ref<Type> GetType() const override
    {
        return g_context->structs.at(as<StructType>(object->GetType())->name).members[memberName];
    }
};

struct StatementAST : public AST
{
    inline StatementAST(Location location)
        : AST(location)
    {
    }

    virtual void Dump(uint32_t indentCount) const = 0;
    virtual void Codegen() const = 0;
    virtual void Typecheck() = 0;
    virtual void DCE() const = 0;
};

struct ReturnStatementAST : public StatementAST
{
    Ref<ExpressionAST> value;

    // Assigned during typechecking
    mutable Ref<Type> returnedType;

    inline ReturnStatementAST(Location location, Ref<ExpressionAST> value)
        : StatementAST(location)
        , value(value)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
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
    void Typecheck();
    void DCE() const;
};

struct IfStatementAST : public StatementAST
{
    Ref<ExpressionAST> condition;
    Ref<BlockAST> block;
    Ref<BlockAST> elseBlock;

    inline IfStatementAST(Location location, Ref<ExpressionAST> condition, Ref<BlockAST> block, Ref<BlockAST> elseBlock)
        : StatementAST(location)
        , condition(condition)
        , block(block)
        , elseBlock(elseBlock)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
};

struct WhileStatementAST : public StatementAST
{
    Ref<ExpressionAST> condition;
    Ref<BlockAST> block;

    inline WhileStatementAST(Location location, Ref<ExpressionAST> condition, Ref<BlockAST> block)
        : StatementAST(location)
        , condition(condition)
        , block(block)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
};

struct VariableDefinitionAST : public StatementAST
{
    std::string name;
    Ref<Type> type;
    bool isConst;
    Ref<ExpressionAST> initialValue;

    bool used = false;

    inline VariableDefinitionAST(Location location, const std::string& name, Ref<Type> type, bool isConst, Ref<ExpressionAST> initialValue)
        : StatementAST(location)
        , name(name)
        , type(type)
        , isConst(isConst)
        , initialValue(initialValue)
    {
    }

    virtual void Dump(uint32_t indentCount) const override;
    virtual void Codegen() const override;
    virtual void Typecheck() override;
    virtual void DCE() const override;
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

    bool used = false;

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
    void Typecheck();
    void DCE() const;
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

    Ref<FunctionAST> FindFunction(const std::string& name) const;
    Ref<VariableDefinitionAST> FindGlobalVariable(const std::string& name) const;

    void Dump(uint32_t indentCount = 0) const;
    void Codegen() const;
    void Typecheck();
    void DCE();
};

extern Ref<ParsedFile> g_parsedFile;
