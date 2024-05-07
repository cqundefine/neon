#pragma once

#include <Utils.h>
#include <llvm/IR/Type.h>

enum class TypeEnum
{
    Integer,
    Array,
    Pointer,
    Struct,
    Void
};

struct Type
{
    TypeEnum type;

    inline Type(TypeEnum type)
        : type(type)
    {
    }

    virtual llvm::Type* GetType() const = 0;
    virtual std::string Dump() const = 0;
    virtual bool Equals(const Type& other) const = 0;
    virtual llvm::DIType* GetDebugType() const = 0;
    virtual void Typecheck(Location) const {};
    virtual llvm::Constant* GetDefaultValue() const = 0;

    virtual std::string ReadableName() const = 0;

    bool operator==(const Type& other) const
    {
        if (other.type != type)
            return false;
        return Equals(other);
    }

    bool operator!=(const Type& other) const
    {
        return !(*this == other);
    }
};

struct IntegerType : public Type
{
    uint16_t bits;
    bool isSigned;

    inline IntegerType(uint16_t bits, bool isSigned)
        : Type(TypeEnum::Integer)
        , bits(bits)
        , isSigned(isSigned)
    {
    }

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
    virtual llvm::DIType* GetDebugType() const override;
    virtual llvm::Constant* GetDefaultValue() const override;

    virtual std::string ReadableName() const override;
};

struct ArrayType : public Type
{
    Ref<Type> arrayType;
    uint64_t size;

    inline ArrayType(Ref<Type> arrayType, uint64_t size)
        : Type(TypeEnum::Array)
        , arrayType(arrayType)
        , size(size)
    {
    }

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
    virtual llvm::DIType* GetDebugType() const override;
    virtual llvm::Constant* GetDefaultValue() const override;

    virtual std::string ReadableName() const override;
};

struct PointerType : public Type
{
    Ref<Type> underlayingType;

    inline PointerType(Ref<Type> underlayingType)
        : Type(TypeEnum::Pointer)
        , underlayingType(underlayingType)
    {
    }

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
    virtual llvm::DIType* GetDebugType() const override;
    virtual void Typecheck(Location location) const override { underlayingType->Typecheck(location); };
    virtual llvm::Constant* GetDefaultValue() const override;

    virtual std::string ReadableName() const override;
};

struct StructType : public Type
{
    std::string name;

    inline StructType(const std::string& name)
        : Type(TypeEnum::Struct)
        , name(name)
    {
    }

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
    virtual llvm::DIType* GetDebugType() const override;
    virtual void Typecheck(Location) const override;
    virtual llvm::Constant* GetDefaultValue() const override;

    virtual std::string ReadableName() const override;

    llvm::Type* GetUnderlayingType() const;
};

struct VoidType : public Type
{
    inline VoidType()
        : Type(TypeEnum::Void)
    {
    }

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
    virtual llvm::DIType* GetDebugType() const override;
    virtual llvm::Constant* GetDefaultValue() const override;

    virtual std::string ReadableName() const override;
};
