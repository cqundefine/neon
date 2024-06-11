#include <Context.h>

#include <filesystem>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Verifier.h>
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

Context::Context(const std::string& baseFile, std::optional<std::string> passedTarget, bool optimize, bool debug)
    : optimize(optimize), debug(debug)
{
    llvmContext = MakeOwn<llvm::LLVMContext>();
    module = MakeOwn<llvm::Module>(baseFile, *llvmContext);
    builder = MakeOwn<llvm::IRBuilder<>>(*llvmContext);

    if (debug)
        debugBuilder = MakeOwn<llvm::DIBuilder>(*module);

    rootFileID = LoadFile(baseFile);

    if (debug)
        debugCompileUnit = debugBuilder->createCompileUnit(llvm::dwarf::DW_LANG_C, debugBuilder->createFile(files.at(rootFileID).filename, "."), "Neon", false, "", 0);

    if (optimize)
    {
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
    }

    auto targetTriple = passedTarget.has_value() ? passedTarget.value() : llvm::sys::getDefaultTargetTriple();

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

    if (targetMachine->getPointerSizeInBits(0) != 64)
    {
        fprintf(stderr, "Unsupported pointer size: %d!\n", targetMachine->getPointerSizeInBits(0));
        exit(1);
    }

    if (arch == "x86-64")
    {
        defines.push_back("X86_64");
    }
    else if (arch == "aarch64")
    {
        defines.push_back("AArch64");
    }
    else
    {
        fprintf(stderr, "Unsupported architecture: %s, not enabling syscalls or preprocessor arch directives!\n", arch.c_str());
    }
}

void Context::CreateSyscall(uint32_t number, std::string mnemonic, std::string returnRegister, std::string registers)
{
    if (auto function = module->getFunction(std::string("syscall") + std::to_string(number)))
    {
        std::vector<llvm::Value*> argsValues {};
        for (auto& arg : function->args())
            argsValues.push_back(&arg);

        if (g_context->debugBuilder)
            g_context->builder->SetCurrentDebugLocation(llvm::DebugLoc());

        auto block = llvm::BasicBlock::Create(*llvmContext, "entry", function);
        builder->SetInsertPoint(block);

        auto inlineAsm = llvm::InlineAsm::get(function->getFunctionType(), mnemonic, std::string("=") + returnRegister + "," + registers + ",~{memory},~{dirflag},~{fpsr},~{flags}", true, true, llvm::InlineAsm::AsmDialect::AD_ATT, false);
        auto result = builder->CreateCall(inlineAsm, argsValues);

        builder->CreateRet(result);

        llvm::verifyFunction(*function);

        if (g_context->optimize)
            g_context->functionPassManager->run(*function, *g_context->functionAnalysisManager);
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
    auto debugFile = debug ? debugBuilder->createFile(filename, ".") : nullptr;
    files[fileID] = { filename, ReadFile(filename), debugFile };
    return fileID;
}

std::pair<uint32_t, uint32_t> Context::LineColumnFromLocation(uint32_t fileID, size_t index) const
{
    assert(fileID != UINT32_MAX);

    uint32_t line = 1;
    uint32_t column = 1;

    for (uint32_t i = 0; i <= index; i++)
    {
        if (files.at(fileID).content[i] == '\n')
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
        fprintf(stderr, "%s:%d:%d ", location.GetFile().filename.c_str(), location.line, location.column);
    }

    va_list va;
    va_start(va, fmt);

    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);

    va_end(va);

    exit(1);
}

void Context::Finalize()
{
    std::string arch = targetMachine->getTarget().getName();

    if (arch == "x86-64")
    {
        CreateSyscall(0, "syscall", "{ax}", "{ax}");
        CreateSyscall(1, "syscall", "{ax}", "{ax},{di}");
        CreateSyscall(2, "syscall", "{ax}", "{ax},{di},{si}");
        CreateSyscall(3, "syscall", "{ax}", "{ax},{di},{si},{dx}");
        CreateSyscall(4, "syscall", "{ax}", "{ax},{di},{si},{dx},{r10}");
        CreateSyscall(5, "syscall", "{ax}", "{ax},{di},{si},{dx},{r10},{r8}");
        CreateSyscall(6, "syscall", "{ax}", "{ax},{di},{si},{dx},{r10},{r8},{r9}");
    }
    else if (arch == "aarch64")
    {
        CreateSyscall(0, "svc #0", "{x0}", "{x8}");
        CreateSyscall(1, "svc #0", "{x0}", "{x8},{x0}");
        CreateSyscall(2, "svc #0", "{x0}", "{x8},{x0},{x1}");
        CreateSyscall(3, "svc #0", "{x0}", "{x8},{x0},{x1},{x2}");
        CreateSyscall(4, "svc #0", "{x0}", "{x8},{x0},{x1},{x2},{x3}");
        CreateSyscall(5, "svc #0", "{x0}", "{x8},{x0},{x1},{x2},{x3},{x4}");
        CreateSyscall(6, "svc #0", "{x0}", "{x8},{x0},{x1},{x2},{x3},{x4},{x5}");
    }
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
        Error({}, "No main function found");
    }

    auto mainFile = files.at(rootFileID).filename;

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

    if (targetMachine->addPassesToEmitFile(pass, outputStream, nullptr, fileType == OutputFileType::Assembly ? llvm::CodeGenFileType::AssemblyFile : llvm::CodeGenFileType::ObjectFile))
    {
        fprintf(stderr, "This machine can't emit this file type");
        exit(1);
    }

    pass.run(*module);
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
