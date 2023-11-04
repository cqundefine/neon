#include <Parser.h>
#include <Type.h>
#include <stack>

Ref<ParsedFile> Parser::Parse()
{
    std::vector<Ref<FunctionAST>> functions;
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
            functions.push_back(MakeRef<FunctionAST>(token.location, nameToken.stringValue, params, MakeRef<IntegerType>(32, true), nullptr));
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
            functions.push_back(MakeRef<FunctionAST>(token.location, nameToken.stringValue, params, MakeRef<IntegerType>(32, true), ParseBlock()));
        }
        else if (token.type == TokenType::Eof)
        {
            break;
        }
        else
        {
            g_context->Error(token.location, "Unexpected token: %s", token.ToString().c_str());
        }
    }
    return MakeRef<ParsedFile>(functions);
}

Ref<BlockAST> Parser::ParseBlock()
{
    auto lcurly = m_lexer.NextToken();
    assert(lcurly.type == TokenType::LCurly);
    Token token = m_lexer.NextToken();
    
    std::vector<ExpressionOrStatement> statements;

    while(token.type != TokenType::RCurly)
    {
        m_lexer.RollbackToken(token);
        statements.push_back(ParseStatement());
        token = m_lexer.NextToken();
    }

    return MakeRef<BlockAST>(lcurly.location, statements);
}

ExpressionOrStatement Parser::ParseStatement()
{
    auto token = m_lexer.NextToken();
    if (token.type == TokenType::Return)
    {
        auto value = ParseExpression();
        ExpectToken(TokenType::Semicolon);
        return MakeRef<ReturnStatementAST>(token.location, value);
    }
    else if (token.type == TokenType::If)
    {
        auto condition = ParseExpression();
        auto block = ParseBlock();
        auto else_ = m_lexer.NextToken();
        if (else_.type == TokenType::Identifier && else_.stringValue == "else")
        {
            auto elseBlock = ParseBlock();
            return MakeRef<IfStatementAST>(token.location, condition, block, elseBlock);
        }
        else
        {
            m_lexer.RollbackToken(else_);
        }
        return MakeRef<IfStatementAST>(token.location, condition, block, nullptr);
    }
    else if (token.type == TokenType::While)
    {
        auto condition = ParseExpression();
        auto block = ParseBlock();
        return MakeRef<WhileStatementAST>(token.location, condition, block);
    }
    else if (token.type == TokenType::Identifier)
    {
        Token colon = m_lexer.NextToken();
        if (colon.type == TokenType::Colon)
        {
            auto type = ParseType();
            auto equalsOrSemicolon = m_lexer.NextToken();
            if (equalsOrSemicolon.type == TokenType::Semicolon)
            {
                return MakeRef<VariableDefinitionAST>(token.location, token.stringValue, type, nullptr);
            }
            else
            {
                auto initialValue = ParseExpression();
                ExpectToken(TokenType::Semicolon);
                return MakeRef<VariableDefinitionAST>(token.location, token.stringValue, type, initialValue);
            }
        }
        else
        {
            m_lexer.RollbackToken(colon);
            m_lexer.RollbackToken(token);
            auto expression = ParseExpression();
            ExpectToken(TokenType::Semicolon);
            return expression;
        }
    }
    else
    {
        m_lexer.RollbackToken(token);
        auto expression = ParseExpression();
        ExpectToken(TokenType::Semicolon);
        return expression;
    }
}

struct OperationInfo
{
    BinaryOperation operation;
    int precedence;
    Location location;
};

Ref<ExpressionAST> Parser::ParseExpression()
{
    std::stack<Ref<ExpressionAST>> expressionStack;
    std::stack<OperationInfo> operationStack;
    int lastPrecedence = INT32_MAX;

    int parenCount = 0;

    {
        Token token = m_lexer.NextToken();
        while(token.type == TokenType::LParen)
        {
            parenCount++;
            token = m_lexer.NextToken();
        }
        m_lexer.RollbackToken(token);
    }

    auto leftSide = ParsePrimary();
    expressionStack.push(leftSide);

    while(true)
    {
        auto operation = ParseOperation();
        if(operation.first == BinaryOperation::_BinaryOperationCount)
            break;

        int precedence = BinaryOperationPrecedence[operation.first] + parenCount * 1000;

        {
            Token token = m_lexer.NextToken();
            while(token.type == TokenType::LParen)
            {
                parenCount++;
                token = m_lexer.NextToken();
            }
            m_lexer.RollbackToken(token);
        }

        auto right_side = ParsePrimary();

        if (parenCount > 0)
        {
            Token token = m_lexer.NextToken();
            while(token.type == TokenType::RParen)
            {
                parenCount--;
                token = m_lexer.NextToken();
            }
            m_lexer.RollbackToken(token);
        }

        while (precedence <= lastPrecedence && expressionStack.size() > 1)
        {
            auto rightSide2 = expressionStack.top();
            expressionStack.pop();
            auto operator2 = operationStack.top();
            operationStack.pop();

            lastPrecedence = operator2.precedence;

            if (lastPrecedence < precedence)
            {
                operationStack.push(operator2);
                expressionStack.push(rightSide2);
                break;
            }

            auto leftSide2 = expressionStack.top();
            expressionStack.pop();

            expressionStack.push(MakeRef<BinaryExpressionAST>(operator2.location, leftSide2, operator2.operation, rightSide2));
        }

        operationStack.push({ operation.first, precedence, operation.second });
        expressionStack.push(right_side);

        lastPrecedence = precedence;
    }

    if (parenCount > 0)
    {
        Token token = m_lexer.NextToken();
        while(token.type == TokenType::RParen)
        {
            parenCount--;
            token = m_lexer.NextToken();
        }
        m_lexer.RollbackToken(token);
    }

    while (expressionStack.size() != 1)
    {
        auto rightSide2 = expressionStack.top();
        expressionStack.pop();
        auto operator2 = operationStack.top();
        operationStack.pop();
        auto leftSide2 = expressionStack.top();
        expressionStack.pop();

        expressionStack.push(MakeRef<BinaryExpressionAST>(operator2.location, leftSide2, operator2.operation, rightSide2));
    }

    return expressionStack.top();
}

Ref<ExpressionAST> Parser::ParsePrimary()
{
    Token token = m_lexer.NextToken();
    if (token.type == TokenType::Number)
    {
        return MakeRef<NumberExpressionAST>(token.location, token.intValue, MakeRef<IntegerType>(32, true));
    }
    else if(token.type == TokenType::Identifier)
    {
        Token paren = m_lexer.NextToken();
        if (paren.type == TokenType::LParen)
        {
            std::vector<Ref<ExpressionAST>> args;
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
            return MakeRef<CallExpressionAST>(token.location, token.stringValue, args);
        }
        else if (paren.type == TokenType::LessThan)
        {
            auto type = ParseType();
            ExpectToken(TokenType::GreaterThan);
            ExpectToken(TokenType::LParen);
            auto child = ParseExpression();
            ExpectToken(TokenType::RParen);
            return MakeRef<CastExpressionAST>(token.location, type, child);
        }
        else
        {
            auto var = MakeRef<VariableExpressionAST>(token.location, token.stringValue);
            if (paren.type == TokenType::LSquareBracket)
            {
                auto index = ParsePrimary();
                assert(index->type == ExpressionType::Number);
                ExpectToken(TokenType::RSquareBracket);
                return MakeRef<ArrayAccessExpressionAST>(paren.location, var, std::static_pointer_cast<NumberExpressionAST>(index));
            }
            m_lexer.RollbackToken(paren);
            return var;
        }
    }
    else if (token.type == TokenType::StringLiteral)
    {
        return MakeRef<StringLiteralAST>(token.location, token.stringValue);
    }
    else
    {
        g_context->Error(token.location, "Unexpected token: %s", token.ToString().c_str());
    }
}

std::pair<BinaryOperation, Location> Parser::ParseOperation()
{
    static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11, "Not all binary operations are handled in Parser::ParseOperation()");
    
    Token token = m_lexer.NextToken();
    switch (token.type)
    {
        case TokenType::Plus:
            return { BinaryOperation::Add, token.location };
        case TokenType::Minus:
            return { BinaryOperation::Subtract, token.location };
        case TokenType::Asterisk:
            return { BinaryOperation::Multiply, token.location };
        case TokenType::Slash:
            return { BinaryOperation::Divide, token.location };
        case TokenType::DoubleEquals:
            return { BinaryOperation::Equals, token.location };
        case TokenType::NotEqual:
            return { BinaryOperation::NotEqual, token.location };
        case TokenType::GreaterThan:
            return { BinaryOperation::GreaterThan, token.location };
        case TokenType::GreaterThanOrEqual:
            return { BinaryOperation::GreaterThanOrEqual, token.location };
        case TokenType::LessThan:
            return { BinaryOperation::LessThan, token.location };
        case TokenType::LessThanOrEqual:
            return { BinaryOperation::LessThanOrEqual, token.location };
        case TokenType::Equals:
            return { BinaryOperation::Assignment, token.location };
        default:
            m_lexer.RollbackToken(token);
            return { BinaryOperation::_BinaryOperationCount, token.location };
    }
}

Ref<Type> Parser::ParseType()
{
    Token typeToken = m_lexer.NextToken();
    assert(typeToken.type == TokenType::Identifier);
    Ref<Type> type;
    if (typeToken.stringValue == "int8" || typeToken.stringValue == "uint8")
        type = MakeRef<IntegerType>(8, typeToken.stringValue[0] != 'u');
    else if (typeToken.stringValue == "int16" || typeToken.stringValue == "uint16")
        type = MakeRef<IntegerType>(16, typeToken.stringValue[0] != 'u');
    else if (typeToken.stringValue == "int32" || typeToken.stringValue == "uint32")
        type = MakeRef<IntegerType>(32, typeToken.stringValue[0] != 'u');
    else if (typeToken.stringValue == "int64" || typeToken.stringValue == "uint64")
        type = MakeRef<IntegerType>(64, typeToken.stringValue[0] != 'u');
    else
        g_context->Error(typeToken.location, "Unexpected token: %s", typeToken.ToString().c_str());

    Token modifier = m_lexer.NextToken();
    if (modifier.type == TokenType::Asterisk)
    {
        assert(false);
        while (modifier.type == TokenType::Asterisk)
        {
            // type = type->getPointerTo();
            // modifier = m_lexer.NextToken();
        }
        m_lexer.RollbackToken(modifier);
    }
    else if (modifier.type == TokenType::LSquareBracket)
    {
        auto size = m_lexer.NextToken();
        assert(size.type == TokenType::Number);
        ExpectToken(TokenType::RSquareBracket);
        return MakeRef<ArrayType>(type, size.intValue);
    }
    else
    {
        m_lexer.RollbackToken(modifier);
    }

    return type;
}

void Parser::ExpectToken(TokenType tokenType)
{
    Token token = m_lexer.NextToken();
    if (token.type != tokenType)
    {
        g_context->Error(token.location, "Unexpected token %s, expected %s", token.ToString().c_str(), TokenTypeToString(tokenType).c_str());
    }
}
