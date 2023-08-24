#include <Parser.h>
#include <stack>

Parser::Parser(const Lexer& lexer) : m_lexer(lexer) {}

std::shared_ptr<ParsedFile> Parser::Parse()
{
    std::vector<std::shared_ptr<FunctionAST>> functions;
    while (true)
    {
        auto token = m_lexer.NextToken();
        if (token.type == TokenType::Extern)
        {
            ExpectToken(TokenType::Function);
            auto nameToken = m_lexer.NextToken();
            assert(nameToken.type == TokenType::Identifier);
            ExpectToken(TokenType::LParen);

            std::vector<FunctionAST::Param> params;

            Token maybeName = m_lexer.NextToken();
            while (maybeName.type == TokenType::Identifier)
            {
                ExpectToken(TokenType::Colon);
                auto type = ParseType();

                params.push_back({maybeName.stringValue, type});

                Token maybeComma = m_lexer.NextToken();
                if (maybeComma.type != TokenType::Comma)
                    m_lexer.RollbackToken(maybeComma);

                maybeName = m_lexer.NextToken();
            }

            m_lexer.RollbackToken(maybeName);

            ExpectToken(TokenType::RParen);
            ExpectToken(TokenType::Colon);
            ExpectToken(TokenType::Identifier);
            ExpectToken(TokenType::Semicolon);
            functions.push_back(std::make_shared<FunctionAST>(nameToken.stringValue, params, llvm::Type::getInt32Ty(*g_context->llvmContext), nullptr));
        }
        else if (token.type == TokenType::Function)
        {
            auto nameToken = m_lexer.NextToken();
            assert(nameToken.type == TokenType::Identifier);
            ExpectToken(TokenType::LParen);
            ExpectToken(TokenType::RParen);
            ExpectToken(TokenType::Colon);
            ExpectToken(TokenType::Identifier);
            std::vector<FunctionAST::Param> params;
            functions.push_back(std::make_shared<FunctionAST>(nameToken.stringValue, params, llvm::Type::getInt32Ty(*g_context->llvmContext), ParseBlock()));
        }
        else if (token.type == TokenType::Eof)
        {
            break;
        }
        else
        {
            g_context->Error(token.offset, "Unexpected token: %s", token.ToString().c_str());
        }
    }
    return std::make_shared<ParsedFile>(functions);
}

std::shared_ptr<BlockAST> Parser::ParseBlock()
{
    ExpectToken(TokenType::LCurly);
    Token token = m_lexer.NextToken();
    
    std::vector<ExpressionOrStatement> statements;

    while(token.type != TokenType::RCurly)
    {
        m_lexer.RollbackToken(token);
        statements.push_back(ParseStatement());
        token = m_lexer.NextToken();
    }

    return std::make_shared<BlockAST>(statements);
}

ExpressionOrStatement Parser::ParseStatement()
{
    auto token = m_lexer.NextToken();
    if (token.type == TokenType::Return)
    {
        auto value = ParseExpression();
        ExpectToken(TokenType::Semicolon);
        return std::make_shared<ReturnStatementAST>(value);
    }
    else if (token.type == TokenType::If)
    {
        auto condition = ParseExpression();
        auto block = ParseBlock();
        auto else_ = m_lexer.NextToken();
        if (else_.type == TokenType::Identifier && else_.stringValue == "else")
        {
            auto elseBlock = ParseBlock();
            return std::make_shared<IfStatementAST>(condition, block, elseBlock);
        }
        else
        {
            m_lexer.RollbackToken(else_);
        }
        return std::make_shared<IfStatementAST>(condition, block, nullptr);
    }
    else if (token.type == TokenType::Identifier)
    {
        Token colonOrEquals = m_lexer.NextToken();
        if (colonOrEquals.type == TokenType::Colon)
        {
            auto type = ParseType();
            auto equalsOrSemicolon = m_lexer.NextToken();
            if (equalsOrSemicolon.type == TokenType::Semicolon)
            {
                return std::make_shared<VariableDefinitionAST>(token.stringValue, type, nullptr);
            }
            else
            {
                auto initialValue = ParseExpression();
                ExpectToken(TokenType::Semicolon);
                return std::make_shared<VariableDefinitionAST>(token.stringValue, type, initialValue);
            }
        }
        else if (colonOrEquals.type == TokenType::Equals)
        {
            auto value = ParseExpression();
            ExpectToken(TokenType::Semicolon);
            return std::make_shared<AssignmentStatementAST>(token.stringValue, value);
        }
        else
        {
            m_lexer.RollbackToken(colonOrEquals);
            m_lexer.RollbackToken(token);
            auto expression = ParsePrimary();
            ExpectToken(TokenType::Semicolon);
            return expression;
        }
    }
    else
    {
        m_lexer.RollbackToken(token);
        auto expression = ParsePrimary();
        ExpectToken(TokenType::Semicolon);
        return expression;
    }
}

std::shared_ptr<ExpressionAST> Parser::ParseExpression()
{
    std::stack<std::shared_ptr<ExpressionAST>> expressionStack;
    std::stack<BinaryOperation> operationStack;
    int lastPrecedence = INT32_MAX;

    auto leftSide = ParsePrimary();
    expressionStack.push(leftSide);

    while(true)
    {
        auto operation = ParseOperation();
        if(operation == BinaryOperation::_BinaryOperationCount)
            break;

        int precedence = BinaryOperationPrecedence[operation];

        auto right_side = ParsePrimary();

        while (precedence <= lastPrecedence && expressionStack.size() > 1)
        {
            auto rightSide2 = expressionStack.top();
            expressionStack.pop();
            auto operator2 = operationStack.top();
            operationStack.pop();

            lastPrecedence = BinaryOperationPrecedence[operator2];

            if (lastPrecedence < precedence)
            {
                operationStack.push(operator2);
                expressionStack.push(rightSide2);
                break;
            }

            auto leftSide2 = expressionStack.top();
            expressionStack.pop();

            expressionStack.push(std::make_shared<BinaryExpressionAST>(leftSide2, operator2, rightSide2));
        }

        operationStack.push(operation);
        expressionStack.push(right_side);

        lastPrecedence = precedence;
    }

    while (expressionStack.size() != 1)
    {
        auto rightSide2 = expressionStack.top();
        expressionStack.pop();
        auto operator2 = operationStack.top();
        operationStack.pop();
        auto leftSide2 = expressionStack.top();
        expressionStack.pop();

        expressionStack.push(std::make_shared<BinaryExpressionAST>(leftSide2, operator2, rightSide2));
    }

    return expressionStack.top();
}

std::shared_ptr<ExpressionAST> Parser::ParsePrimary()
{
    Token token = m_lexer.NextToken();
    if (token.type == TokenType::Number)
    {
        return std::make_shared<NumberExpressionAST>(token.intValue);
    }
    else if(token.type == TokenType::Identifier)
    {
        Token paren = m_lexer.NextToken();
        if (paren.type == TokenType::LParen)
        {
            std::vector<std::shared_ptr<ExpressionAST>> args;
            Token arg = m_lexer.NextToken();
            while(arg.type != TokenType::RParen)
            {
                if (arg.type != TokenType::Comma)
                {
                    m_lexer.RollbackToken(arg);
                    args.push_back(ParseExpression());
                }
                arg = m_lexer.NextToken();
            }
            return std::make_shared<CallExpressionAST>(token.stringValue, args);
        }
        else
        {
            m_lexer.RollbackToken(paren);
            return std::make_shared<VariableExpressionAST>(token.stringValue);
        }
    }
    else if (token.type == TokenType::StringLiteral)
    {
        return std::make_shared<StringLiteralAST>(token.stringValue);
    }
    else
    {
        g_context->Error(token.offset, "Unexpected token: %s", token.ToString().c_str());
    }
}

BinaryOperation Parser::ParseOperation()
{
    static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 5, "Not all binary operations are handled in Parser::ParseOperation()");
    
    Token token = m_lexer.NextToken();
    switch (token.type)
    {
    case TokenType::Plus:
        return BinaryOperation::Add;
    case TokenType::Minus:
        return BinaryOperation::Subtract;
    case TokenType::Asterisk:
        return BinaryOperation::Multiply;
    case TokenType::Slash:
        return BinaryOperation::Divide;
    case TokenType::DoubleEquals:
        return BinaryOperation::Equals;
    default:
        m_lexer.RollbackToken(token);
        return BinaryOperation::_BinaryOperationCount;
    }
}

llvm::Type* Parser::ParseType()
{
    Token typeToken = m_lexer.NextToken();
    assert(typeToken.type == TokenType::Identifier);
    llvm::Type* type;
    if (typeToken.stringValue == "int8" || typeToken.stringValue == "uint8")
        type = reinterpret_cast<llvm::Type*>(llvm::Type::getInt8Ty(*g_context->llvmContext));
    else if (typeToken.stringValue == "int16" || typeToken.stringValue == "uint16")
        type = reinterpret_cast<llvm::Type*>(llvm::Type::getInt16Ty(*g_context->llvmContext));
    else if (typeToken.stringValue == "int32" || typeToken.stringValue == "uint32")
        type = reinterpret_cast<llvm::Type*>(llvm::Type::getInt32Ty(*g_context->llvmContext));
    else if (typeToken.stringValue == "int64" || typeToken.stringValue == "uint64")
        type = reinterpret_cast<llvm::Type*>(llvm::Type::getInt64Ty(*g_context->llvmContext));
    else
        g_context->Error(typeToken.offset, "Unexpected token: %s", typeToken.ToString().c_str());

    Token maybeStar = m_lexer.NextToken();
    while (maybeStar.type == TokenType::Asterisk)
    {
        type = type->getPointerTo();
        maybeStar = m_lexer.NextToken();
    }
    m_lexer.RollbackToken(maybeStar);

    return type;
}

void Parser::ExpectToken(TokenType tokenType)
{
    Token token = m_lexer.NextToken();
    if (token.type != tokenType)
    {
        g_context->Error(token.offset, "Unexpected token %s, expected %s", token.ToString().c_str(), TokenTypeToString(tokenType).c_str());
    }
}
