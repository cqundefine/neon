#include <Context.h>

#include <filesystem>
#include <llvm/IR/InlineAsm.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <stdarg.h>
#include <sys/prctl.h>
#include <sys/wait.h>

Ref<Context> g_context;

uint32_t lastFileID = 0;

#define CREATE_SYSCALL(N, MNEMONIC, RET, REGS)                                                                                                                                                                         \
    {                                                                                                                                                                                                   \
        auto int64Type = llvm::Type::getInt64Ty(*llvmContext);                                                                                                                                          \
        std::vector<llvm::Type*> syscall##N##Args {};                                                                                                                                                   \
        for (uint32_t i = 0; i < N + 1; i++)                                                                                                                                                            \
            syscall##N##Args.push_back(int64Type);                                                                                                                                                      \
        auto syscall##N##Type = llvm::FunctionType::get(int64Type, syscall##N##Args, false);                                                                                                            \
        auto syscall##N = llvm::Function::Create(syscall##N##Type, llvm::Function::LinkageTypes::ExternalLinkage, "syscall" #N, *module);                                                               \
        std::vector<llvm::Value*> syscall##N##ArgsValues {};                                                                                                                                            \
        for (auto& arg : syscall##N->args())                                                                                                                                                            \
            syscall##N##ArgsValues.push_back(&arg);                                                                                                                                                     \
        auto syscall##N##Block = llvm::BasicBlock::Create(*llvmContext, "entry", syscall##N);                                                                                                           \
        builder->SetInsertPoint(syscall##N##Block);                                                                                                                                                     \
        auto syscall##N##AsmCall = llvm::InlineAsm::get(syscall##N##Type, MNEMONIC, "=" RET "," REGS ",~{memory},~{dirflag},~{fpsr},~{flags}", true, true, llvm::InlineAsm::AsmDialect::AD_ATT, false); \
        auto syscall##N##Result = builder->CreateCall(syscall##N##AsmCall, syscall##N##ArgsValues);                                                                                                     \
        builder->CreateRet(syscall##N##Result);                                                                                                                                                         \
    }

Context::Context(const std::string& baseFile, std::optional<std::string> passedTarget)
{
    rootFileID = LoadFile(baseFile);

    llvmContext = MakeOwn<llvm::LLVMContext>();
    module = MakeOwn<llvm::Module>(baseFile, *llvmContext);
    builder = MakeOwn<llvm::IRBuilder<>>(*llvmContext);

    // Create new pass and analysis managers.
    functionPassManager = MakeOwn<llvm::FunctionPassManager>();
    loopAnalysisManager = MakeOwn<llvm::LoopAnalysisManager>();
    functionAnalysisManager = MakeOwn<llvm::FunctionAnalysisManager>();
    cgsccAnalysisManager = MakeOwn<llvm::CGSCCAnalysisManager>();
    moduleAnalysisManager = MakeOwn<llvm::ModuleAnalysisManager>();
    passInstrumentationCallbacks = MakeOwn<llvm::PassInstrumentationCallbacks>();
    standardInstrumentations = MakeOwn<llvm::StandardInstrumentations>(*llvmContext, true);
    standardInstrumentations->registerCallbacks(*passInstrumentationCallbacks, moduleAnalysisManager.get());

    functionPassManager->addPass(llvm::PromotePass());
    functionPassManager->addPass(llvm::DCEPass());
    functionPassManager->addPass(llvm::InstCombinePass());
    functionPassManager->addPass(llvm::ReassociatePass());
    functionPassManager->addPass(llvm::GVNPass());
    functionPassManager->addPass(llvm::SimplifyCFGPass());

    llvm::PassBuilder passBuilder;
    passBuilder.registerModuleAnalyses(*moduleAnalysisManager);
    passBuilder.registerFunctionAnalyses(*functionAnalysisManager);
    passBuilder.crossRegisterProxies(*loopAnalysisManager, *functionAnalysisManager, *cgsccAnalysisManager, *moduleAnalysisManager);

    auto targetTriple = passedTarget.has_value()  ? passedTarget.value() : llvm::sys::getDefaultTargetTriple();

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target)
    {
        fprintf(stderr, "Error loading target %s: %s", targetTriple.c_str(), error.c_str());
        exit(1);
    }

    targetMachine = target->createTargetMachine(targetTriple, "generic", "", {}, {});

    module->setDataLayout(targetMachine->createDataLayout());
    module->setTargetTriple(targetTriple);

    std::string arch = targetMachine->getTarget().getName();

    if (arch == "x86-64")
    {
        CREATE_SYSCALL(0, "syscall", "{ax}", "{ax}");
        CREATE_SYSCALL(1, "syscall", "{ax}", "{ax},{di}");
        CREATE_SYSCALL(2, "syscall", "{ax}", "{ax},{di},{si}");
        CREATE_SYSCALL(3, "syscall", "{ax}", "{ax},{di},{si},{dx}");
        CREATE_SYSCALL(4, "syscall", "{ax}", "{ax},{di},{si},{dx},{r10}");
        CREATE_SYSCALL(5, "syscall", "{ax}", "{ax},{di},{si},{dx},{r10},{r8}");
        CREATE_SYSCALL(6, "syscall", "{ax}", "{ax},{di},{si},{dx},{r10},{r8},{r9}");
        defines.push_back("X86_64");
    }
    else if (arch == "aarch64")
    {
        CREATE_SYSCALL(0, "svc #0", "{x0}", "{x8}");
        CREATE_SYSCALL(1, "svc #0", "{x0}", "{x8},{x0}");
        CREATE_SYSCALL(2, "svc #0", "{x0}", "{x8},{x0},{x1}");
        CREATE_SYSCALL(3, "svc #0", "{x0}", "{x8},{x0},{x1},{x2}");
        CREATE_SYSCALL(4, "svc #0", "{x0}", "{x8},{x0},{x1},{x2},{x3}");
        CREATE_SYSCALL(5, "svc #0", "{x0}", "{x8},{x0},{x1},{x2},{x3},{x4}");
        CREATE_SYSCALL(6, "svc #0", "{x0}", "{x8},{x0},{x1},{x2},{x3},{x4},{x5}");
        defines.push_back("AArch64");
    }
    else
    {
        fprintf(stderr, "Unsupported architecture: %s, not enabling syscalls or preprocessor arch directives!\n", arch.c_str());
    }
}

uint32_t Context::LoadFile(const std::string& filename)
{
    for (const auto& [fileID, fileInfo] : files)
    {
        if (fileInfo.filename == filename)
            return fileID;
    }

    uint32_t fileID = lastFileID++;
    files[fileID] = { filename, ReadFile(filename) };
    return fileID;
}

std::pair<uint32_t, uint32_t> Context::LineColumnFromLocation(Location location) const
{
    assert(location.fileID != UINT32_MAX);

    uint32_t line = 1;
    uint32_t column = 1;

    for (uint32_t index = 0; index <= location.index; index++)
    {
        if (files.at(location.fileID).content[index] == '\n')
        {
            line++;
            column = 1;
        }
        else
        {
            column++;
        }
    }

    return { line, column };
}

[[noreturn]] void Context::Error(Location location, const char* fmt, ...) const
{
    if (location.fileID != UINT32_MAX)
    {
        auto lineColumn = LineColumnFromLocation(location);
        fprintf(stderr, "%s:%d:%d ", files.at(location.fileID).filename.c_str(), lineColumn.first, lineColumn.second);
    }

    va_list va;
    va_start(va, fmt);

    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);

    va_end(va);

    exit(1);
}

void Context::Write(OutputFileType fileType, std::optional<std::string> outputLocation, bool run) const
{
    assert(!run || fileType == OutputFileType::Executable);

    bool foundMain = false;
    for (auto& function : module->functions())
    {
        if (function.isDeclaration())
            continue;

        if (function.getName() == "main")
        {
            foundMain = true;
            break;
        }
    }

    if (!foundMain)
    {
        g_context->Error(LocationNone, "No main function found");
    }

    auto mainFile = files.at(0).filename;

    auto baseFilename = mainFile.substr(mainFile.find_last_of("/\\") + 1);
    auto fileWithoutExtension = baseFilename.substr(0, baseFilename.find_last_of('.'));

    std::string objectFilename;
    if (fileType == OutputFileType::Executable)
    {
        objectFilename = std::filesystem::temp_directory_path() / ("tempobject" + baseFilename + ".o");
    }
    else if (outputLocation.has_value())
    {
        objectFilename = outputLocation.value();
    }
    else
    {
        objectFilename = fileType == OutputFileType::Assembly ? fileWithoutExtension + ".asm" : fileWithoutExtension + ".o";;
    }

    std::error_code errorCode;
    llvm::raw_fd_ostream outputStream(objectFilename, errorCode, llvm::sys::fs::OF_None);

    if (errorCode)
    {
        fprintf(stderr, "Error writing output: %s", errorCode.message().c_str());
        exit(1);
    }

    llvm::legacy::PassManager pass;

    if (g_context->targetMachine->addPassesToEmitFile(pass, outputStream, nullptr, fileType == OutputFileType::Assembly ? llvm::CodeGenFileType::AssemblyFile : llvm::CodeGenFileType::ObjectFile))
    {
        fprintf(stderr, "This machine can't emit this file type");
        exit(1);
    }

    pass.run(*g_context->module);
    outputStream.flush();

    if (fileType == OutputFileType::Executable)
    {
        std::string binaryFilename = outputLocation.has_value() ? outputLocation.value() : fileWithoutExtension;

        system((std::string("gcc ") + objectFilename + " -o " + binaryFilename + " -no-pie").c_str());

        std::filesystem::remove(objectFilename);

        if (run)
            system((std::string("./") + binaryFilename).c_str());
    }
}
