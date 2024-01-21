#include <Context.h>
#include <Lexer.h>
#include <Parser.h>

#include <argparse/argparse.hpp>

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("neon");

    program.add_argument("filename")
        .help("input file");

    program.add_argument("-c")
        .help("compile to object file")
        .flag();

    program.add_argument("-r", "--run")
        .help("run executable")
        .flag();

    auto& dumpGroup = program.add_mutually_exclusive_group();

    dumpGroup.add_argument("--dump-tokens")
        .help("dump tokens")
        .flag();

    dumpGroup.add_argument("--dump-ast")
        .help("dump AST")
        .flag();

    dumpGroup.add_argument("--dump-ir")
        .help("dump IR")
        .flag();

    dumpGroup.add_argument("--dump-asm")
        .help("dump assembly")
        .flag();

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(1);
    }

    g_context = MakeRef<Context>();
    g_context->filename = program.get("filename");
    g_context->fileContent = ReadFile(g_context->filename);

    Lexer lexer;
    
    if (program["--dump-tokens"] == true)
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
    
    if (program["--dump-ast"] == true)
    {
        parsedFile->Dump();
        return 0;
    }

    parsedFile->Codegen();
    if (program["--dump-ir"] == true)
        g_context->module->print(llvm::outs(), nullptr);
    else if (program["--dump-asm"] == true)
        g_context->Write(Context::OutputFileType::Assembly);
    else if (program["-c"] == true)
        g_context->Write(Context::OutputFileType::Object);
    else
        g_context->Write(Context::OutputFileType::Executable, program["--run"] == true);

    return 0;
}
