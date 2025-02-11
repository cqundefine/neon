#include <Context.h>
#include <Lexer.h>
#include <Parser.h>
#include <Preprocessor.h>

#include <argparse/argparse.hpp>

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("neon");

    program.add_argument("filename").help("input file");
    program.add_argument("-o").help("output file");
    program.add_argument("-c").help("compile to object file").flag();
    program.add_argument("-r", "--run").help("run executable").flag();
    program.add_argument("--target").help("target triple");
    program.add_argument("--disable-dce").help("disable dead code elimination").flag();

    auto& optimizeDebugGroup = program.add_mutually_exclusive_group();
    optimizeDebugGroup.add_argument("-O").help("optimize").flag();
    optimizeDebugGroup.add_argument("-g").help("add debug information").flag();

    auto& dumpGroup = program.add_mutually_exclusive_group();
    dumpGroup.add_argument("--dump-tokens-before-preprocessor").help("dump tokens before preprocessor").flag();
    dumpGroup.add_argument("--dump-tokens").help("dump tokens").flag();
    dumpGroup.add_argument("--dump-ast").help("dump AST").flag();
    dumpGroup.add_argument("--dump-ir").help("dump IR").flag();
    dumpGroup.add_argument("--dump-asm").help("dump assembly").flag();

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

    g_context = MakeRef<Context>(program.get("filename"), program.present("--target"), program["-O"] == true, program["-g"] == true);

    auto tokenStream = CreateTokenStream(g_context->rootFileID);

    if (program["--dump-tokens-before-preprocessor"] == true)
    {
        tokenStream.Dump();
        return 0;
    }

    tokenStream = Preprocess(std::move(tokenStream));

    if (program["--dump-tokens"] == true)
    {
        tokenStream.Dump();
        return 0;
    }

    Parser parser(std::move(tokenStream));
    g_parsedFile = parser.Parse();
    g_parsedFile->Typecheck();

    if (program["--disable-dce"] == false)
        g_parsedFile->DCE();

    if (program["--dump-ast"] == true)
    {
        g_parsedFile->Dump();
        return 0;
    }

    g_parsedFile->Codegen();

    g_context->Finalize();

    if (program["--dump-ir"] == true)
        g_context->module->print(llvm::outs(), nullptr);
    else if (program["--dump-asm"] == true)
        g_context->Write(Context::OutputFileType::Assembly, program.present("-o"));
    else if (program["-c"] == true)
        g_context->Write(Context::OutputFileType::Object, program.present("-o"));
    else
        g_context->Write(Context::OutputFileType::Executable, program.present("-o"), program["--run"] == true);

    return 0;
}
