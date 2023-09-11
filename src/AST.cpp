#include <AST.h>
#include <regex>

ExpressionAST::ExpressionAST(ExpressionType type) : type(type) {}
NumberExpressionAST::NumberExpressionAST(uint64_t value, NumberType type) : ExpressionAST(ExpressionType::Number), value(value), type(type) {}
VariableExpressionAST::VariableExpressionAST(const std::string& name) : ExpressionAST(ExpressionType::Variable), name(name) {}
StringLiteralAST::StringLiteralAST(const std::string& value) : ExpressionAST(ExpressionType::StringLiteral), value(value) {}
BinaryExpressionAST::BinaryExpressionAST(std::shared_ptr<ExpressionAST> lhs, BinaryOperation binaryOperation, std::shared_ptr<ExpressionAST> rhs) : ExpressionAST(ExpressionType::Binary), lhs(lhs), binaryOperation(binaryOperation), rhs(rhs) {}
CallExpressionAST::CallExpressionAST(std::string calleeName, std::vector<std::shared_ptr<ExpressionAST>> args) : ExpressionAST(ExpressionType::Call), calleeName(calleeName), args(args) {}
CastExpressionAST::CastExpressionAST(llvm::Type* castedTo, std::shared_ptr<ExpressionAST> child) : ExpressionAST(ExpressionType::Cast), castedTo(castedTo), child(child) {}
ReturnStatementAST::ReturnStatementAST(std::shared_ptr<ExpressionAST> value) : value(value) {}
BlockAST::BlockAST(const std::vector<ExpressionOrStatement>& statements) : statements(statements) {}
IfStatementAST::IfStatementAST(std::shared_ptr<ExpressionAST> condition, std::shared_ptr<BlockAST> block, std::shared_ptr<BlockAST> elseBlock) : condition(condition), block(block), elseBlock(elseBlock) {}
WhileStatementAST::WhileStatementAST(std::shared_ptr<ExpressionAST> condition, std::shared_ptr<BlockAST> block) : condition(condition), block(block) {}
VariableDefinitionAST::VariableDefinitionAST(const std::string& name, llvm::Type* type, std::shared_ptr<ExpressionAST> initialValue) : name(name), type(type), initialValue(initialValue) {}
AssignmentStatementAST::AssignmentStatementAST(std::string name, std::shared_ptr<ExpressionAST> value) : name(name), value(value) {}
FunctionAST::FunctionAST(const std::string& name, std::vector<Param> params, llvm::Type* returnType, std::shared_ptr<BlockAST> block) : name(name), params(params), returnType(returnType), block(block) {}
ParsedFile::ParsedFile(const std::vector<std::shared_ptr<FunctionAST>>& functions) : functions(functions) {}

void NumberExpressionAST::AdjustTypeToBits(uint32_t bits)
{
    // FIXME: Check if adjustment is correct
    // FIXME: Signed/Unsigned types
    if (bits == 8)
        type = NumberType::Int8;
    else if (bits == 16)
        type = NumberType::Int16;
    else if (bits == 32)
        type = NumberType::Int32;
    else if (bits == 64)
        type = NumberType::Int64;
}
