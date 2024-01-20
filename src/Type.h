#pragma once

#include <llvm/IR/Type.h>
#include <Utils.h>

enum class TypeEnum
{
    Integer,
    String,
    Array,
    Pointer,
    Void
};

struct Type
{
    TypeEnum type;

    inline Type(TypeEnum type) : type(type) {}

    virtual llvm::Type* GetType() const = 0;
    virtual std::string Dump() const = 0;
    virtual bool Equals(const Type& other) const = 0;

    bool operator==(const Type& other)
    {
        if (other.type != type)
            return false;
        return Equals(other);
    }
};

struct IntegerType : public Type
{
    uint16_t bits;
    bool isSigned;

    inline IntegerType(uint16_t bits, bool isSigned) : Type(TypeEnum::Integer), bits(bits), isSigned(isSigned) {}

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
};

struct StringType : public Type
{
    inline StringType() : Type(TypeEnum::String) {}

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;

    llvm::Type* GetUnderlayingType() const;
};

struct ArrayType : public Type
{
    Ref<Type> arrayType;
    uint64_t size;

    inline ArrayType(Ref<Type> arrayType, uint64_t size) : Type(TypeEnum::Array), arrayType(arrayType), size(size) {}

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
};

struct PointerType : public Type
{
    Ref<Type> underlayingType;

    inline PointerType(Ref<Type> underlayingType) : Type(TypeEnum::Pointer), underlayingType(underlayingType) {}

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
};

struct VoidType : public Type
{
    inline VoidType() : Type(TypeEnum::Void) {}

    virtual llvm::Type* GetType() const override;
    virtual std::string Dump() const override;
    virtual bool Equals(const Type& other) const override;
};
