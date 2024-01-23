#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>

#include <AST.h>
#include <Utils.h>

struct Context
{
    Context(const std::string& baseFile);

    Own<llvm::LLVMContext> llvmContext;
    Own<llvm::IRBuilder<>> builder;
    Own<llvm::Module> module;
    Own<llvm::legacy::FunctionPassManager> functionPassManager;
    llvm::TargetMachine* targetMachine;
    llvm::StructType* stringType;

    bool optimize;

    struct FileInfo
    {
        std::string filename;
        std::string content;
    };

    uint32_t rootFileID;
    std::map<uint32_t, FileInfo> files;

    uint32_t LoadFile(const std::string& filename);

    std::pair<uint32_t, uint32_t> LineColumnFromLocation(Location location) const;
    [[noreturn]] void Error(Location location, const char* fmt, ...) const;

    enum class OutputFileType
    {
        Assembly,
        Object,
        Executable
    };

    void Write(OutputFileType fileType, bool run = false) const;
};

extern Ref<Context> g_context;
