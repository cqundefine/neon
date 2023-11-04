#include <Context.h>
#include <Type.h>

// --------------------------
// Dump
// --------------------------

std::string IntegerType::Dump() const
{
    return std::string("IntegerType (") + std::to_string(bits) + ", " + (isSigned ? "true" : "false") + ")"; 
}

std::string StringType::Dump() const
{
    return "StringType";
}

std::string ArrayType::Dump() const
{
    return std::string("ArrayType (") + arrayType->Dump() + ", " + std::to_string(size) + ")"; 
}

// --------------------------
// GetType
// --------------------------

llvm::Type* IntegerType::GetType() const
{
    return llvm::Type::getIntNTy(*g_context->llvmContext, bits);
}

llvm::Type* StringType::GetType() const
{
    return llvm::Type::getInt8PtrTy(*g_context->llvmContext);
}

llvm::Type* ArrayType::GetType() const
{
    return llvm::ArrayType::get(arrayType->GetType(), size);
}

// --------------------------
// Equals
// --------------------------

bool IntegerType::Equals(const Type& other) const
{
    auto otherInteger = reinterpret_cast<const IntegerType*>(&other);
    return bits == otherInteger->bits && isSigned == otherInteger->isSigned;
}

bool StringType::Equals(const Type& other) const
{
    return true;
}

bool ArrayType::Equals(const Type& other) const
{
    auto otherArray = reinterpret_cast<const ArrayType*>(&other);
    return *arrayType == *otherArray->arrayType && size == otherArray->size;
}
