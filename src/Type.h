#pragma once

#include <llvm/IR/Type.h>

enum TypeEnum
{
    Integer,
    String
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
};
