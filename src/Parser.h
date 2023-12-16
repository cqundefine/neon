#pragma once

#include <AST.h>
#include <Enums.h>
#include <Lexer.h>
#include <llvm/IR/Type.h>

class Parser
{
public:
    inline explicit Parser(const Lexer& lexer) : m_lexer(lexer) {}

    Ref<ParsedFile> Parse();

private:
    Ref<BlockAST> ParseBlock();
    ExpressionOrStatement ParseStatement();
    Ref<ExpressionAST> ParseExpression();
    Ref<ExpressionAST> ParsePrimary();
    std::pair<BinaryOperation, Location> ParseOperation();
    Ref<Type> ParseType(bool allowVoid = false);

    void ExpectToken(TokenType tokenType);

    const Lexer& m_lexer;
};