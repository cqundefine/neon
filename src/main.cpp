#include <Context.h>
#include <Lexer.h>
#include <Parser.h>

#include <llvm/CodeGen/MachineInstr.h>

int main(int argc, char** argv)
{
    assert(argc == 3);

    g_context = MakeRef<Context>();
    g_context->filename = argv[1];
    g_context->fileContent = ReadFile(g_context->filename);

    std::string operation = argv[2];

    Lexer lexer;
    
    if (operation == "token")
    {
        Token token;
        do {
            token = lexer.NextToken();
            printf("%s\n", token.ToString().c_str());
        } while (token.type != TokenType::Eof);
        return 0;
    }

    Parser parser(lexer);
    auto parsedFile = parser.Parse();
    parsedFile->Typecheck();
    
    if (operation == "ast")
    {
        parsedFile->Dump();
    }
    else if (operation == "ir")
    {
        parsedFile->Codegen();
        g_context->module->print(llvm::outs(), nullptr);
    }
    else if (operation == "asm")
    {
        parsedFile->Codegen();
        g_context->Write(Context::OutputFileType::Assembly);
    }
    else if (operation == "obj")
    {
        parsedFile->Codegen();
        g_context->Write(Context::OutputFileType::Object);
    }
    else if (operation == "exe")
    {
        parsedFile->Codegen();
        g_context->Write(Context::OutputFileType::Executable);
    }
    else if (operation == "exe-r")
    {
        parsedFile->Codegen();
        g_context->Write(Context::OutputFileType::ExecutableRun);
    }
    else
    {
        assert(false);
    }
}
