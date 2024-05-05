#pragma once

#include <AST.h>
#include <Enums.h>
#include <TokenStream.h>
#include <llvm/IR/Type.h>

class Parser
{
public:
    explicit inline Parser(TokenStream stream)
        : m_stream(std::move(stream))
    {
    }

    Ref<ParsedFile> Parse();

private:
    Ref<BlockAST> ParseBlock();
    ExpressionOrStatement ParseStatement();
    Ref<VariableDefinitionAST> ParseVariableDefinition();
    Ref<ExpressionAST> ParseExpression();
    Ref<ExpressionAST> ParsePrimary();
    Ref<ExpressionAST> ParseBarePrimary();
    std::pair<BinaryOperation, Location> ParseOperation();
    Ref<Type> ParseType(bool allowVoid = false);

    void ExpectToken(TokenType tokenType);
    void ExpectToBe(Token token, TokenType tokenType);

    TokenStream m_stream;
};
