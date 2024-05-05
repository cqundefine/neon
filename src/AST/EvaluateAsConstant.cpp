#include <AST.h>

llvm::Constant* NumberExpressionAST::EvaluateAsConstant() const
{
    return llvm::ConstantInt::get(type->GetType(), llvm::APInt(type->bits, value, type->isSigned));
}

llvm::Constant* StringLiteralAST::EvaluateAsConstant() const
{
    // FIXME: Use createGlobalStringPtr
    std::vector<llvm::Constant*> chars(value.size());
    for (uint8_t i = 0; i < value.size(); i++)
        chars[i] = llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(8, value[i], true));

    auto charArray = llvm::ConstantArray::get(llvm::ArrayType::get(llvm::Type::getInt8Ty(*g_context->llvmContext), chars.size()), chars);
    auto rawString = new llvm::GlobalVariable(*g_context->module, charArray->getType(), true, llvm::GlobalValue::PrivateLinkage, charArray);
    auto stringStruct = llvm::ConstantStruct::get(g_context->structs.at("string").llvmType, { llvm::ConstantExpr::getBitCast(rawString, llvm::Type::getInt8Ty(*g_context->llvmContext)->getPointerTo()), llvm::ConstantInt::get(*g_context->llvmContext, llvm::APInt(64, value.size())) });

    return stringStruct;
}
