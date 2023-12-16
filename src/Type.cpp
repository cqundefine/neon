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

std::string PointerType::Dump() const
{
    return std::string("PointerType (") + underlayingType->Dump() + ")"; 
}

std::string VoidType::Dump() const
{
    return "VoidType";
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

llvm::Type* PointerType::GetType() const
{
    return llvm::PointerType::get(underlayingType->GetType(), 0);
}

llvm::Type* VoidType::GetType() const
{
    return llvm::Type::getVoidTy(*g_context->llvmContext);
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

bool PointerType::Equals(const Type& other) const
{
    auto otherPointer = reinterpret_cast<const PointerType*>(&other);
    return *underlayingType == *otherPointer->underlayingType;
}

bool VoidType::Equals(const Type& other) const
{
    // FIXME: Figure out if we should ever compare void types
    return true;
}
