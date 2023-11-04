#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>

#include <AST.h>
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

    std::pair<uint32_t, uint32_t> LineColumnFromLocation(Location location) const;
    [[noreturn]] void Error(uint32_t location, const char* fmt, ...) const;

    void Finalize();

    enum class OutputFileType
    {
        Assembly,
        Object,
        Executable,
        ExecutableRun
    };

    void Write(OutputFileType fileType);

private:
    void CreateSyscall0();
    void CreateSyscall1();
    void CreateSyscall2();
    void CreateSyscall3();
};

extern Ref<Context> g_context;
