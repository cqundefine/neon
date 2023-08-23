#pragma once

#include <AST.h>
#include <Enums.h>
#include <Lexer.h>
#include <llvm/IR/Type.h>

class Parser
{
public:
    Parser(const Lexer& lexer);

    std::shared_ptr<ParsedFile> Parse();

private:
    std::shared_ptr<BlockAST> ParseBlock();
    ExpressionOrStatement ParseStatement();
    std::shared_ptr<ExpressionAST> ParseExpression();
    std::shared_ptr<ExpressionAST> ParsePrimary();
    BinaryOperation ParseOperation();
    llvm::Type* ParseType();

    void ExpectToken(TokenType tokenType);

    const Lexer& m_lexer;
};