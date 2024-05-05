#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Target/TargetMachine.h>

#include <Type.h>
#include <Utils.h>
#include <map>

struct Context
{
    Context(const std::string& baseFile, std::optional<std::string> target);

    Own<llvm::LLVMContext> llvmContext;
    Own<llvm::Module> module;

    Own<llvm::IRBuilder<>> builder;

    Own<llvm::FunctionPassManager> functionPassManager;
    Own<llvm::LoopAnalysisManager> loopAnalysisManager;
    Own<llvm::FunctionAnalysisManager> functionAnalysisManager;
    Own<llvm::CGSCCAnalysisManager> cgsccAnalysisManager;
    Own<llvm::ModuleAnalysisManager> moduleAnalysisManager;
    Own<llvm::PassInstrumentationCallbacks> passInstrumentationCallbacks;
    Own<llvm::StandardInstrumentations> standardInstrumentations;

    llvm::TargetMachine* targetMachine;

    struct StructInfo
    {
        std::string name;
        std::map<std::string, Ref<Type>> members;
        llvm::StructType* llvmType;
    };

    std::map<std::string, StructInfo> structs;

    bool optimize;

    struct FileInfo
    {
        std::string filename;
        std::string content;
    };

    uint32_t rootFileID;
    std::map<uint32_t, FileInfo> files;

    std::vector<std::string> defines;

    uint32_t LoadFile(const std::string& filename);

    std::pair<uint32_t, uint32_t> LineColumnFromLocation(Location location) const;
    [[noreturn]] void Error(Location location, const char* fmt, ...) const;

    enum class OutputFileType
    {
        Assembly,
        Object,
        Executable
    };

    void Write(OutputFileType fileType, std::optional<std::string> outputLocation, bool run = false) const;
};

extern Ref<Context> g_context;
