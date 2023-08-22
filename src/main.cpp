#include <assert.h>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

#include <Lexer.h>
#include <Parser.h>

std::unique_ptr<llvm::LLVMContext> g_LLVMContext;
std::unique_ptr<llvm::IRBuilder<>> g_Builder;
std::unique_ptr<llvm::Module> g_LLVMModule;
std::unique_ptr<llvm::legacy::FunctionPassManager> g_LLVMFPM;

std::string g_FileName;

Lexer g_Lexer("");

int main(int argc, char** argv)
{
    assert(argc == 3);

    g_FileName = argv[1];

    g_LLVMContext = std::make_unique<llvm::LLVMContext>();
    g_LLVMModule = std::make_unique<llvm::Module>("root", *g_LLVMContext);
    g_Builder = std::make_unique<llvm::IRBuilder<>>(*g_LLVMContext);

    g_LLVMFPM = std::make_unique<llvm::legacy::FunctionPassManager>(g_LLVMModule.get());

    // Promote allocas to registers.
    g_LLVMFPM->add(llvm::createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    g_LLVMFPM->add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    g_LLVMFPM->add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    g_LLVMFPM->add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    g_LLVMFPM->add(llvm::createCFGSimplificationPass());

    g_LLVMFPM->doInitialization();

    auto targetTriple = "x86_64-pc-linux-gnu";

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target)
    {
        fprintf(stderr, "Error loading target %s: %s", targetTriple, error.c_str());
        exit(1);
    }

    auto targetMachine = target->createTargetMachine(targetTriple, "generic", "", {}, {});

    g_LLVMModule->setDataLayout(targetMachine->createDataLayout());
    g_LLVMModule->setTargetTriple(targetTriple);

    std::ifstream ifs(g_FileName);
    std::stringstream fileBuffer;
    fileBuffer << ifs.rdbuf();

    g_Lexer = Lexer(fileBuffer.str());
    
    if (strcmp(argv[2], "token") == 0)
    {
        Token token;
        do {
            token = g_Lexer.NextToken();
            printf("%s\n", token.ToString().c_str());
        } while (token.type != TokenType::Eof);
        return 0;
    }

    Parser parser(g_Lexer);

    auto parsedFile = parser.Parse();
    
    if (strcmp(argv[2], "ast") == 0)
    {
        parsedFile->Dump();
    }
    else if (strcmp(argv[2], "ir") == 0)
    {
        parsedFile->Codegen();
        g_LLVMModule->print(llvm::errs(), nullptr);
    }
    else if (strcmp(argv[2], "asm") == 0)
    {
        parsedFile->Codegen();
        std::error_code errorCode;
        llvm::raw_fd_ostream outputStream("/dev/stdout", errorCode, llvm::sys::fs::OF_None);

        if (errorCode)
        {
            fprintf(stderr, "Error writing output: %s", errorCode.message().c_str());
            exit(1);
        }

        llvm::legacy::PassManager pass;

        if (targetMachine->addPassesToEmitFile(pass, outputStream, nullptr, llvm::CGFT_AssemblyFile))
        {
            fprintf(stderr, "This machine can't emit this file type");
            exit(1);
        }

        pass.run(*g_LLVMModule);
        outputStream.flush();
    }
}
