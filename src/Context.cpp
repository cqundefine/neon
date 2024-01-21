#include <Context.h>

#include <filesystem>
#include <llvm/IR/InlineAsm.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#include <stdarg.h>
#include <sys/prctl.h>
#include <sys/wait.h>

Ref<Context> g_context;

#define CREATE_SYSCALL(N, REGS) \
    { \
    auto int64Type = llvm::Type::getInt64Ty(*llvmContext); \
    std::vector<llvm::Type*> syscall##N##Args {}; \
    for (uint32_t i = 0; i < N + 1; i++) \
        syscall##N##Args.push_back(int64Type); \
    auto syscall##N##Type = llvm::FunctionType::get(int64Type, syscall##N##Args, false); \
    auto syscall##N = llvm::Function::Create(syscall##N##Type, llvm::Function::LinkageTypes::ExternalLinkage, "syscall" #N, *module); \
    std::vector<llvm::Value*> syscall##N##ArgsValues {}; \
    for (auto& arg : syscall##N->args()) \
        syscall##N##ArgsValues.push_back(&arg); \
    auto syscall##N##Block = llvm::BasicBlock::Create(*llvmContext, "entry", syscall##N); \
    builder->SetInsertPoint(syscall##N##Block); \
    auto syscall##N##AsmCall = llvm::InlineAsm::get(syscall##N##Type, "syscall", "={ax}," REGS ",~{memory},~{dirflag},~{fpsr},~{flags}", true, true, llvm::InlineAsm::AsmDialect::AD_Intel, false); \
    auto syscall##N##Result = builder->CreateCall(syscall##N##AsmCall, syscall##N##ArgsValues); \
    builder->CreateRet(syscall##N##Result); \
    }


Context::Context()
{
    llvmContext = MakeOwn<llvm::LLVMContext>();
    module = MakeOwn<llvm::Module>(filename, *llvmContext);
    builder = MakeOwn<llvm::IRBuilder<>>(*llvmContext);
    functionPassManager = MakeOwn<llvm::legacy::FunctionPassManager>(module.get());

    functionPassManager->add(llvm::createPromoteMemoryToRegisterPass());
    functionPassManager->add(llvm::createInstructionCombiningPass());
    functionPassManager->add(llvm::createReassociatePass());
    functionPassManager->add(llvm::createGVNPass());
    functionPassManager->add(llvm::createCFGSimplificationPass());

    functionPassManager->doInitialization();

    // FIXME: Unhardcode the target
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

    targetMachine = target->createTargetMachine(targetTriple, "generic", "", {}, {});

    module->setDataLayout(targetMachine->createDataLayout());
    module->setTargetTriple(targetTriple);

    std::vector<llvm::Type*> stringMembers { llvm::Type::getInt8PtrTy(*llvmContext), llvm::Type::getInt64Ty(*llvmContext) };
    stringType = llvm::StructType::create(*llvmContext, stringMembers, "String");

    CREATE_SYSCALL(0, "{ax}");
    CREATE_SYSCALL(1, "{ax},{di}");
    CREATE_SYSCALL(2, "{ax},{di},{si}");
    CREATE_SYSCALL(3, "{ax},{di},{si},{dx}");
    CREATE_SYSCALL(4, "{ax},{di},{si},{dx},{r10}");
    CREATE_SYSCALL(5, "{ax},{di},{si},{dx},{r10},{r8}");
    CREATE_SYSCALL(6, "{ax},{di},{si},{dx},{r10},{r8},{r9}");
}

std::pair<uint32_t, uint32_t> Context::LineColumnFromLocation(uint32_t location) const
{
    uint32_t line = 1;
    uint32_t column = 1;

    for (uint32_t index = 0; index <= location; index++)
    {
        if (fileContent[index] == '\n')
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
    fprintf(stderr, "%s:%d:%d ", filename.c_str(),lineColumn.first, lineColumn.second);

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

    auto sourceFilenameWithoutExtension = filename.substr(0, filename.length() - filename.substr(filename.find_last_of('.') + 1).length() - 1);
    auto outputFilename = fileType == OutputFileType::Assembly ? sourceFilenameWithoutExtension + ".asm" : sourceFilenameWithoutExtension + ".o";

    std::error_code errorCode;
    llvm::raw_fd_ostream outputStream(outputFilename, errorCode, llvm::sys::fs::OF_None);

    if (errorCode)
    {
        fprintf(stderr, "Error writing output: %s", errorCode.message().c_str());
        exit(1);
    }

    llvm::legacy::PassManager pass;

    if (g_context->targetMachine->addPassesToEmitFile(pass, outputStream, nullptr, fileType == OutputFileType::Assembly ? llvm::CGFT_AssemblyFile : llvm::CGFT_ObjectFile))
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
