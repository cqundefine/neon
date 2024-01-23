#include <Parser.h>
#include <Type.h>
#include <stack>

Ref<ParsedFile> Parser::Parse()
{
    std::vector<Ref<FunctionAST>> functions;
    while (true)
    {
        auto token = m_stream.NextToken();
        if (token.type == TokenType::Extern || token.type == TokenType::Function)
        {
            if (token.type == TokenType::Extern)
                ExpectToken(TokenType::Function);
            auto nameToken = m_stream.NextToken();
            assert(nameToken.type == TokenType::Identifier);
            ExpectToken(TokenType::LParen);

            std::vector<FunctionAST::Param> params;

            Token maybeName = m_stream.NextToken();
            while (maybeName.type == TokenType::Identifier)
            {
                ExpectToken(TokenType::Colon);
                auto type = ParseType();

                params.push_back({ maybeName.stringValue, type });

                Token maybeComma = m_stream.NextToken();
                if (maybeComma.type != TokenType::Comma)
                    m_stream.PreviousToken();

                maybeName = m_stream.NextToken();
            }

            m_stream.PreviousToken();

            ExpectToken(TokenType::RParen);
            ExpectToken(TokenType::Colon);

            auto returnType = ParseType(true);

            Ref<BlockAST> block;
            if (token.type == TokenType::Extern)
                ExpectToken(TokenType::Semicolon);
            else
                block = ParseBlock();
            functions.push_back(MakeRef<FunctionAST>(token.location, nameToken.stringValue, params, returnType, block));
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
    auto lcurly = m_stream.NextToken();
    assert(lcurly.type == TokenType::LCurly);
    Token token = m_stream.NextToken();

    std::vector<ExpressionOrStatement> statements;

    while (token.type != TokenType::RCurly)
    {
        m_stream.PreviousToken();
        statements.push_back(ParseStatement());
        token = m_stream.NextToken();
    }

    return MakeRef<BlockAST>(lcurly.location, statements);
}

ExpressionOrStatement Parser::ParseStatement()
{
    auto token = m_stream.NextToken();
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
        auto else_ = m_stream.NextToken();
        if (else_.type == TokenType::Identifier && else_.stringValue == "else")
        {
            auto elseBlock = ParseBlock();
            return MakeRef<IfStatementAST>(token.location, condition, block, elseBlock);
        }
        else
        {
            m_stream.PreviousToken();
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
        Token colon = m_stream.NextToken();
        if (colon.type == TokenType::Colon)
        {
            auto type = ParseType();
            auto equalsOrSemicolon = m_stream.NextToken();
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
            m_stream.PreviousToken();
            m_stream.PreviousToken();
            auto expression = ParseExpression();
            ExpectToken(TokenType::Semicolon);
            return expression;
        }
    }
    else
    {
        m_stream.PreviousToken();
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
        Token token = m_stream.NextToken();
        while (token.type == TokenType::LParen)
        {
            parenCount++;
            token = m_stream.NextToken();
        }
        m_stream.PreviousToken();
    }

    auto leftSide = ParsePrimary();
    expressionStack.push(leftSide);

    while (true)
    {
        auto operation = ParseOperation();
        if (operation.first == BinaryOperation::_BinaryOperationCount)
            break;

        int precedence = BinaryOperationPrecedence[operation.first] + parenCount * 1000;

        {
            Token token = m_stream.NextToken();
            while (token.type == TokenType::LParen)
            {
                parenCount++;
                token = m_stream.NextToken();
            }
            m_stream.PreviousToken();
        }

        auto right_side = ParsePrimary();

        if (parenCount > 0)
        {
            Token token = m_stream.NextToken();
            while (token.type == TokenType::RParen)
            {
                parenCount--;
                token = m_stream.NextToken();
            }
            m_stream.PreviousToken();
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
        Token token = m_stream.NextToken();
        while (token.type == TokenType::RParen)
        {
            parenCount--;
            token = m_stream.NextToken();
        }
        m_stream.PreviousToken();
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
    auto primary = ParseBarePrimary();
    Token token = m_stream.NextToken();
    while (token.type == TokenType::Dot)
    {
        auto member = m_stream.NextToken();
        assert(member.type == TokenType::Identifier);
        primary = MakeRef<MemberAccessExpressionAST>(token.location, primary, member.stringValue);
        token = m_stream.NextToken();
    }

    m_stream.PreviousToken();
    return primary;
}

Ref<ExpressionAST> Parser::ParseBarePrimary()
{
    Token first = m_stream.NextToken();
    if (first.type == TokenType::Number)
    {
        return MakeRef<NumberExpressionAST>(first.location, first.intValue, MakeRef<IntegerType>(32, true));
    }
    else if (first.type == TokenType::Identifier)
    {
        Token second = m_stream.NextToken();
        if (second.type == TokenType::LParen)
        {
            std::vector<Ref<ExpressionAST>> args;
            Token arg = m_stream.NextToken();
            while (arg.type != TokenType::RParen)
            {
                if (arg.type != TokenType::Comma)
                {
                    m_stream.PreviousToken();
                    args.push_back(ParseExpression());
                }
                arg = m_stream.NextToken();
            }
            return MakeRef<CallExpressionAST>(first.location, first.stringValue, args);
        }
        else if (second.type == TokenType::LessThan)
        {
            auto type = ParseType();
            ExpectToken(TokenType::GreaterThan);
            ExpectToken(TokenType::LParen);
            auto child = ParseExpression();
            ExpectToken(TokenType::RParen);
            return MakeRef<CastExpressionAST>(first.location, type, child);
        }
        else
        {
            auto var = MakeRef<VariableExpressionAST>(first.location, first.stringValue);
            if (second.type == TokenType::LSquareBracket)
            {
                auto index = ParsePrimary();
                assert(index->type == ExpressionType::Number);
                ExpectToken(TokenType::RSquareBracket);
                return MakeRef<ArrayAccessExpressionAST>(second.location, var, std::static_pointer_cast<NumberExpressionAST>(index));
            }
            m_stream.PreviousToken();
            return var;
        }
    }
    else if (first.type == TokenType::StringLiteral)
    {
        return MakeRef<StringLiteralAST>(first.location, first.stringValue);
    }
    else if (first.type == TokenType::Asterisk)
    {
        auto child = ParsePrimary();
        if (child->type != ExpressionType::Variable)
            g_context->Error(first.location, "Can't dereference this expression");
        return MakeRef<DereferenceExpressionAST>(first.location, StaticRefCast<VariableExpressionAST>(child));
    }
    else
    {
        g_context->Error(first.location, "Unexpected token: %s", first.ToString().c_str());
    }
}

std::pair<BinaryOperation, Location> Parser::ParseOperation()
{
    static_assert(static_cast<uint32_t>(BinaryOperation::_BinaryOperationCount) == 11, "Not all binary operations are handled in Parser::ParseOperation()");

    Token token = m_stream.NextToken();
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
            m_stream.PreviousToken();
            return { BinaryOperation::_BinaryOperationCount, token.location };
    }
}

Ref<Type> Parser::ParseType(bool allowVoid)
{
    Token typeToken = m_stream.NextToken();
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
    else if (typeToken.stringValue == "string")
        type = MakeRef<StringType>();
    else if (typeToken.stringValue == "void")
    {
        if (!allowVoid)
            g_context->Error(typeToken.location, "void is not allowed here");
        type = MakeRef<VoidType>();
    }
    else
        g_context->Error(typeToken.location, "Unexpected token: %s", typeToken.ToString().c_str());

    Token modifier = m_stream.NextToken();
    if (modifier.type == TokenType::Asterisk)
    {
        while (modifier.type == TokenType::Asterisk)
        {
            type = MakeRef<PointerType>(type);
            modifier = m_stream.NextToken();
        }
        m_stream.PreviousToken();
    }
    else if (modifier.type == TokenType::LSquareBracket)
    {
        auto size = m_stream.NextToken();
        assert(size.type == TokenType::Number);
        ExpectToken(TokenType::RSquareBracket);
        return MakeRef<ArrayType>(type, size.intValue);
    }
    else
    {
        m_stream.PreviousToken();
    }

    return type;
}

void Parser::ExpectToken(TokenType tokenType)
{
    Token token = m_stream.NextToken();
    if (token.type != tokenType)
    {
        g_context->Error(token.location, "Unexpected token %s, expected %s", token.ToString().c_str(), TokenTypeToString(tokenType).c_str());
    }
}
