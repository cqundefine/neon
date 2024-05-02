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

#define CREATE_SYSCALL(N, REGS)                                                                                                                                                                         \
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
        auto syscall##N##AsmCall = llvm::InlineAsm::get(syscall##N##Type, "syscall", "={ax}," REGS ",~{memory},~{dirflag},~{fpsr},~{flags}", true, true, llvm::InlineAsm::AsmDialect::AD_Intel, false); \
        auto syscall##N##Result = builder->CreateCall(syscall##N##AsmCall, syscall##N##ArgsValues);                                                                                                     \
        builder->CreateRet(syscall##N##Result);                                                                                                                                                         \
    }

Context::Context(const std::string& baseFile)
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

    auto targetTriple = llvm::sys::getDefaultTargetTriple();

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

    CREATE_SYSCALL(0, "{ax}");
    CREATE_SYSCALL(1, "{ax},{di}");
    CREATE_SYSCALL(2, "{ax},{di},{si}");
    CREATE_SYSCALL(3, "{ax},{di},{si},{dx}");
    CREATE_SYSCALL(4, "{ax},{di},{si},{dx},{r10}");
    CREATE_SYSCALL(5, "{ax},{di},{si},{dx},{r10},{r8}");
    CREATE_SYSCALL(6, "{ax},{di},{si},{dx},{r10},{r8},{r9}");
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
    auto lineColumn = LineColumnFromLocation(location);
    if (location.fileID != UINT32_MAX)
        fprintf(stderr, "%s:%d:%d ", files.at(location.fileID).filename.c_str(), lineColumn.first, lineColumn.second);

    va_list va;
    va_start(va, fmt);

    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);

    va_end(va);

    exit(1);
}

void Context::Write(OutputFileType fileType, bool run) const
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

    auto sourceFilenameWithoutExtension = files.at(0).filename.substr(0, files.at(0).filename.length() - files.at(0).filename.substr(files.at(0).filename.find_last_of('.') + 1).length() - 1);
    auto outputFilename = fileType == OutputFileType::Assembly ? sourceFilenameWithoutExtension + ".asm" : sourceFilenameWithoutExtension + ".o";

    std::error_code errorCode;
    llvm::raw_fd_ostream outputStream(outputFilename, errorCode, llvm::sys::fs::OF_None);

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
        system((std::string("gcc ") + outputFilename + " -o " + sourceFilenameWithoutExtension + " -no-pie").c_str());

    if (run)
        system((std::string("./") + sourceFilenameWithoutExtension).c_str());
}
