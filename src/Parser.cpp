#include <Parser.h>
#include <Type.h>
#include <stack>

Ref<ParsedFile> Parser::Parse()
{
    std::vector<Ref<FunctionAST>> functions;
    std::vector<Ref<VariableDefinitionAST>> globalVariables;

    while (true)
    {
        auto token = m_stream.NextToken();
        if (token.type == TokenType::Extern || token.type == TokenType::Function)
        {
            if (token.type == TokenType::Extern)
                ExpectToken(TokenType::Function);
            auto nameToken = m_stream.NextToken();
            ExpectToBe(nameToken, TokenType::Identifier);
            ExpectToken(TokenType::LParen);

            std::vector<FunctionAST::Param> params;

            Token maybeName = m_stream.NextToken();
            while (maybeName.type == TokenType::Identifier)
            {
                ExpectToken(TokenType::Colon);
                auto type = ParseType().first;

                params.push_back({ maybeName.stringValue, type });

                Token maybeComma = m_stream.NextToken();
                if (maybeComma.type != TokenType::Comma)
                    m_stream.PreviousToken();

                maybeName = m_stream.NextToken();
            }

            m_stream.PreviousToken();

            ExpectToken(TokenType::RParen);
            ExpectToken(TokenType::Colon);

            auto returnType = ParseType(true).first;

            Ref<BlockAST> block;
            if (token.type == TokenType::Extern)
                ExpectToken(TokenType::Semicolon);
            else
                block = ParseBlock();
            functions.push_back(MakeRef<FunctionAST>(token.location, nameToken.stringValue, params, returnType, block));
        }
        else if (token.type == TokenType::Struct)
        {
            auto nameToken = m_stream.NextToken();
            if (nameToken.type != TokenType::Identifier)
                g_context->Error(nameToken.location, "Expected struct name");
            ExpectToken(TokenType::LCurly);

            std::map<std::string, std::pair<Ref<Type>, Location>> members;

            while (m_stream.PeekToken().type == TokenType::Identifier)
            {
                auto name = m_stream.NextToken();
                ExpectToken(TokenType::Colon);
                auto typeLocation = ParseType();

                members[name.stringValue] = typeLocation;

                ExpectToken(TokenType::Semicolon);
            }

            ExpectToken(TokenType::RCurly);

            std::vector<llvm::Type*> llvmMembers;
            std::map<std::string, Ref<Type>> rawMembers;
            std::vector<llvm::Metadata*> debugTypes;
            for (auto& [name, type] : members)
            {
                type.first->Typecheck(type.second);
                llvmMembers.push_back(type.first->GetType());
                rawMembers[name] = type.first;
                if (g_context->debug)
                {
                    auto file = nameToken.location.GetFile().debugFile;
                    debugTypes.push_back(g_context->debugBuilder->createMemberType(file, name, file, nameToken.location.line, g_context->module->getDataLayout().getTypeAllocSizeInBits(type.first->GetType()), 0, 0, llvm::DINode::FlagZero, type.first->GetDebugType()));
                }
            }

            if (llvmMembers.empty())
            {
                llvmMembers.push_back(llvm::Type::getInt8Ty(*g_context->llvmContext));
                if (g_context->debug)
                    debugTypes.push_back(g_context->debugBuilder->createBasicType("uint8", 8, llvm::dwarf::DW_ATE_unsigned));
            }

            auto llvmType = llvm::StructType::create(llvmMembers, nameToken.stringValue);

            llvm::DICompositeType* debugType = nullptr;
            if (g_context->debug)
            {
                auto file = nameToken.location.GetFile().debugFile;
                debugType = g_context->debugBuilder->createStructType(file, nameToken.stringValue, file, nameToken.location.line, g_context->module->getDataLayout().getTypeAllocSizeInBits(llvmType), 0, llvm::DINode::FlagPrototyped, nullptr, g_context->debugBuilder->getOrCreateArray(debugTypes));
            }

            g_context->structs[nameToken.stringValue] = {
                .name = nameToken.stringValue,
                .members = rawMembers,
                .llvmType = llvmType,
                .debugType = debugType
            };
        }
        else if (token.type == TokenType::Var || token.type == TokenType::Const)
        {
            m_stream.PreviousToken();
            globalVariables.push_back(ParseVariableDefinition());
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
    return MakeRef<ParsedFile>(functions, globalVariables);
}

Ref<BlockAST> Parser::ParseBlock()
{
    auto lcurly = m_stream.NextToken();
    ExpectToBe(lcurly, TokenType::LCurly);
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
        if (m_stream.PeekToken().type == TokenType::Semicolon)
        {
            m_stream.NextToken();
            return MakeRef<ReturnStatementAST>(token.location, nullptr);
        }

        auto value = ParseExpression();
        ExpectToken(TokenType::Semicolon);
        return MakeRef<ReturnStatementAST>(token.location, value);
    }
    else if (token.type == TokenType::If)
    {
        auto condition = ParseExpression();
        auto block = ParseBlock();
        if (m_stream.PeekToken().type == TokenType::Identifier && m_stream.PeekToken().stringValue == "else")
        {
            m_stream.NextToken();
            auto elseBlock = ParseBlock();
            return MakeRef<IfStatementAST>(token.location, condition, block, elseBlock);
        }

        return MakeRef<IfStatementAST>(token.location, condition, block, nullptr);
    }
    else if (token.type == TokenType::While)
    {
        auto condition = ParseExpression();
        auto block = ParseBlock();
        return MakeRef<WhileStatementAST>(token.location, condition, block);
    }
    else if (token.type == TokenType::Var || token.type == TokenType::Const)
    {
        m_stream.PreviousToken();
        return ParseVariableDefinition();
    }
    else
    {
        m_stream.PreviousToken();
        auto expression = ParseExpression();
        ExpectToken(TokenType::Semicolon);
        return expression;
    }
}

Ref<VariableDefinitionAST> Parser::ParseVariableDefinition()
{
    Token declaration = m_stream.NextToken();
    Token name = m_stream.NextToken();
    ExpectToBe(name, TokenType::Identifier);
    ExpectToken(TokenType::Colon);
    auto type = ParseType().first;
    auto equalsOrSemicolon = m_stream.NextToken();
    if (equalsOrSemicolon.type == TokenType::Semicolon)
    {
        return MakeRef<VariableDefinitionAST>(declaration.location, name.stringValue, type, declaration.type == TokenType::Const, nullptr);
    }
    else if (equalsOrSemicolon.type == TokenType::Equals)
    {
        auto initialValue = ParseExpression();
        ExpectToken(TokenType::Semicolon);
        return MakeRef<VariableDefinitionAST>(declaration.location, name.stringValue, type, declaration.type == TokenType::Const, initialValue);
    }
    else
    {
        g_context->Error(equalsOrSemicolon.location, "Unexpected token: %s", equalsOrSemicolon.ToString().c_str());
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
        ExpectToBe(member, TokenType::Identifier);
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
            while (m_stream.PeekToken().type != TokenType::RParen)
            {
                args.push_back(ParseExpression());
                if (m_stream.PeekToken().type == TokenType::Comma)
                {
                    m_stream.NextToken();
                }
                else if (m_stream.PeekToken().type != TokenType::RParen)
                {
                    g_context->Error(m_stream.PeekToken().location, "Expected comma or right parenthesis");
                }
            }
            m_stream.NextToken();
            return MakeRef<CallExpressionAST>(first.location, first.stringValue, args);
        }
        else
        {
            auto var = MakeRef<VariableExpressionAST>(first.location, first.stringValue);
            if (second.type == TokenType::LSquareBracket)
            {
                auto index = ParsePrimary();
                ExpectToken(TokenType::RSquareBracket);
                return MakeRef<ArrayAccessExpressionAST>(second.location, var, index);
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
    else if (first.type == TokenType::To)
    {
        ExpectToken(TokenType::LessThan);
        auto type = ParseType().first;
        ExpectToken(TokenType::GreaterThan);
        ExpectToken(TokenType::LParen);
        auto child = ParseExpression();
        ExpectToken(TokenType::RParen);
        return MakeRef<CastExpressionAST>(first.location, type, child);
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

std::pair<Ref<Type>, Location> Parser::ParseType(bool allowVoid)
{
    Token typeToken = m_stream.NextToken();
    ExpectToBe(typeToken, TokenType::Identifier);

    Ref<Type> type;
    if (typeToken.stringValue == "int8" || typeToken.stringValue == "uint8")
        type = MakeRef<IntegerType>(8, typeToken.stringValue[0] != 'u');
    else if (typeToken.stringValue == "int16" || typeToken.stringValue == "uint16")
        type = MakeRef<IntegerType>(16, typeToken.stringValue[0] != 'u');
    else if (typeToken.stringValue == "int32" || typeToken.stringValue == "uint32")
        type = MakeRef<IntegerType>(32, typeToken.stringValue[0] != 'u');
    else if (typeToken.stringValue == "int64" || typeToken.stringValue == "uint64")
        type = MakeRef<IntegerType>(64, typeToken.stringValue[0] != 'u');
    else if (typeToken.stringValue == "void")
    {
        if (!allowVoid)
            g_context->Error(typeToken.location, "void is not allowed here");
        type = MakeRef<VoidType>();
    }
    else
        type = MakeRef<StructType>(typeToken.stringValue);

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
        ExpectToBe(size, TokenType::Number);
        ExpectToken(TokenType::RSquareBracket);
        return { MakeRef<ArrayType>(type, size.intValue) , typeToken.location };
    }
    else
    {
        m_stream.PreviousToken();
    }

    return { type, typeToken.location };
}

void Parser::ExpectToken(TokenType tokenType)
{
    Token token = m_stream.NextToken();
    if (token.type != tokenType)
        g_context->Error(token.location, "Unexpected token %s, expected %s", token.ToString().c_str(), TokenTypeToString(tokenType).c_str());
}

void Parser::ExpectToBe(Token token, TokenType tokenType)
{
    if (token.type != tokenType)
        g_context->Error(token.location, "Unexpected token %s, expected %s", token.ToString().c_str(), TokenTypeToString(tokenType).c_str());
}
