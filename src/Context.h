#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>

#include <Utils.h>

struct Context
{
    Context();

    std::string filename;
    std::string fileContent;

    Own<llvm::LLVMContext> llvmContext;
    Own<llvm::IRBuilder<>> builder;
    Own<llvm::Module> module;
    Own<llvm::legacy::FunctionPassManager> functionPassManager;
    llvm::TargetMachine* targetMachine;

    std::pair<uint32_t, uint32_t> LineColumnFromOffset(uint32_t offset);
    [[noreturn]] void Error(uint32_t offset, const char* fmt, ...);

    enum class OutputFileType
    {
        Assembly,
        Object
    };

    void Write(OutputFileType fileType);
};

extern Ref<Context> g_context;
