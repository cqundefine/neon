#pragma once

#include <cstdint>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Target/TargetMachine.h>

#include <Type.h>
#include <Utils.h>
#include <format>
#include <iostream>
#include <map>
#include <optional>
#include <print>

struct Context
{
    Context(const std::string& baseFile, std::optional<std::string> target, bool optimize, bool debug);

    Own<llvm::LLVMContext> llvmContext;
    Own<llvm::Module> module;

    Own<llvm::IRBuilder<>> builder;

    Own<llvm::DIBuilder> debugBuilder;
    llvm::DICompileUnit* debugCompileUnit;

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
        llvm::DIType* debugType;
    };

    std::map<std::string, StructInfo> structs;

    bool optimize;
    bool debug;

    uint32_t rootFileID;
    std::map<uint32_t, FileInfo> files;

    std::vector<std::string> defines;

    void CreateSyscall(uint32_t number, std::string mnemonic, std::string returnRegister, std::string registers);

    uint32_t LoadFile(const std::string& filename);

    std::pair<uint32_t, uint32_t> LineColumnFromLocation(uint32_t fileID, size_t index) const;

    template <class... Args>
    [[noreturn]] void Error(Location location, std::string_view msg, Args&&... args) const
    {
        if (location.fileID.has_value())
            std::print(std::cerr, "{}:{}:{} ", location.GetFile().filename, location.line, location.column);

        std::println(std::cerr, "{}", std::vformat(msg, std::make_format_args(args...)));

        exit(1);
    }

    void Finalize();

    enum class OutputFileType
    {
        Assembly,
        Object,
        Executable
    };

    void Write(OutputFileType fileType, std::optional<std::string> outputLocation, bool run = false) const;
};

extern Ref<Context> g_context;
