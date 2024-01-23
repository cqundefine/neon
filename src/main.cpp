#include <Context.h>
#include <Lexer.h>
#include <Parser.h>
#include <Preprocessor.h>

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

    program.add_argument("-O")
        .help("optimize")
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

    g_context = MakeRef<Context>(program.get("filename"));

    g_context->optimize = program["-O"] == true;

    auto tokenStream = CreateTokenStream(g_context->rootFileID);
    tokenStream = Preprocess(std::move(tokenStream));

    if (program["--dump-tokens"] == true)
    {
        tokenStream.Dump();
        return 0;
    }

    Parser parser(std::move(tokenStream));
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
