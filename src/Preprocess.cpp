#include <Lexer.h>
#include <Preprocessor.h>

static std::vector<uint32_t> g_includedFiles;

void IncludeFile(TokenStream& stream, const std::string& file)
{
    uint32_t includedID = g_context->LoadFile("lib/" + file + ".ne");
    if (std::find(g_includedFiles.begin(), g_includedFiles.end(), includedID) != g_includedFiles.end())
        return;
    g_includedFiles.push_back(includedID);

    auto includeStream = CreateTokenStream(includedID);
    auto includePreprocessedStream = Preprocess(includeStream);
    stream.InsertStream(includePreprocessedStream);
}

TokenStream PreprocessSubfile(TokenStream stream)
{
    while (true)
    {
        auto token = stream.NextToken();
        if (token.type == TokenType::Eof)
            break;

        if (token.type == TokenType::Include)
        {
            stream.RemoveLastToken();
            auto path = stream.NextToken();
            stream.RemoveLastToken();
            if (path.type != TokenType::StringLiteral)
                g_context->Error(path.location, "Expected string literal after include");

            IncludeFile(stream, path.stringValue);
        }
    }

    stream.Reset();
    return std::move(stream);
}

TokenStream Preprocess(TokenStream stream)
{
    IncludeFile(stream, "Prelude");
    return std::move(PreprocessSubfile(std::move(stream)));
}
