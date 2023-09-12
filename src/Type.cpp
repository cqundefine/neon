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
