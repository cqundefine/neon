#include <Context.h>
#include <Type.h>

// --------------------------
// Dump
// --------------------------

std::string IntegerType::Dump() const
{
    return std::string("IntegerType (") + std::to_string(bits) + ", " + (isSigned ? "true" : "false") + ")";
}

std::string ArrayType::Dump() const
{
    return std::string("ArrayType (") + arrayType->Dump() + ", " + std::to_string(size) + ")";
}

std::string PointerType::Dump() const
{
    return std::string("PointerType (") + underlayingType->Dump() + ")";
}

std::string StructType::Dump() const
{
    return std::string("StructType (") + name + ")";
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

llvm::Type* ArrayType::GetType() const
{
    return llvm::ArrayType::get(arrayType->GetType(), size);
}

llvm::Type* PointerType::GetType() const
{
    return llvm::PointerType::get(underlayingType->GetType(), 0);
}

llvm::Type* StructType::GetType() const
{
    return llvm::PointerType::get(g_context->structs.at(name).llvmType, 0);
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
    // FIXME: Figure out if we should ever compare integer types
    // for now, just return true
    return true;
    // auto otherInteger = reinterpret_cast<const IntegerType*>(&other);
    // return bits == otherInteger->bits && isSigned == otherInteger->isSigned;
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

bool StructType::Equals(const Type& other) const
{
    auto otherStruct = reinterpret_cast<const StructType*>(&other);
    return name == otherStruct->name;
}

bool VoidType::Equals(const Type& other) const
{
    // FIXME: Figure out if we should ever compare void types
    return true;
}

// --------------------------
// ReadableName
// --------------------------

std::string IntegerType::ReadableName() const
{
    return (isSigned ? "int" : "uint") + std::to_string(bits);
}

std::string ArrayType::ReadableName() const
{
    return arrayType->ReadableName() + "[" + std::to_string(size) + "]";
}

std::string PointerType::ReadableName() const
{
    return underlayingType->ReadableName() + "*";
}

std::string StructType::ReadableName() const
{
    return name;
}

std::string VoidType::ReadableName() const
{
    return "void";
}

// --------------------------
// GetDebugType
// --------------------------

llvm::DIType* IntegerType::GetDebugType() const
{
    return g_context->debugBuilder->createBasicType(ReadableName(), bits, isSigned ? llvm::dwarf::DW_ATE_signed : llvm::dwarf::DW_ATE_unsigned);
}

llvm::DIType* ArrayType::GetDebugType() const
{
    return g_context->debugBuilder->createArrayType(size, 0, arrayType->GetDebugType(), g_context->debugBuilder->getOrCreateArray(g_context->debugBuilder->getOrCreateSubrange(0, size)));
}

llvm::DIType* PointerType::GetDebugType() const
{
    return g_context->debugBuilder->createPointerType(underlayingType->GetDebugType(), 64);
}

llvm::DIType* StructType::GetDebugType() const
{
    return g_context->structs.at(name).debugType;
}

llvm::DIType* VoidType::GetDebugType() const
{
    return g_context->debugBuilder->createUnspecifiedType("void");
}

// --------------------------
// GetDefaultValue
// --------------------------

llvm::Constant* IntegerType::GetDefaultValue() const
{
    return llvm::ConstantInt::get(GetType(), llvm::APInt(bits, 0, isSigned));
}

llvm::Constant* ArrayType::GetDefaultValue() const
{
    return llvm::ConstantAggregateZero::get(GetType());
}

llvm::Constant* PointerType::GetDefaultValue() const
{
    return llvm::ConstantPointerNull::get(llvm::PointerType::get(underlayingType->GetType(), 0));
}

llvm::Constant* StructType::GetDefaultValue() const
{
    return llvm::ConstantPointerNull::get(llvm::PointerType::get(g_context->structs.at(name).llvmType, 0));
}

llvm::Constant* VoidType::GetDefaultValue() const
{
    fprintf(stderr, "COMPILER ERROR: Can't get default value for void type\n");
    exit(1);
}

// --------------------------
// Other
// --------------------------

void StructType::Typecheck(Location location) const
{
    if (g_context->structs.find(name) == g_context->structs.end())
        g_context->Error(location, "Can't find struct: %s", name.c_str());
}

llvm::Type* StructType::GetUnderlayingType() const
{
    return g_context->structs.at(name).llvmType;
}
