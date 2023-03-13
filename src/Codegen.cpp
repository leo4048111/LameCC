#include "lcc.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/CodeGen/MIRParser/MIRParser.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/AutoUpgrade.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LLVMRemarkStreamer.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/MCTargetOptionsCommandFlags.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Remarks/HotnessThresholdParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TimeProfiler.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <memory>
#include <optional>

namespace lcc
{
    std::unique_ptr<Codegen> Codegen::_inst;

    // Default enable debug stream buffering.
    bool Codegen::EnableDebugBuffering = true;

    int Codegen::compileModule(char **args, llvm::LLVMContext &context)
    {
        // TODO
        return 0;
    }

    int Codegen::run()
    {
        llvm::InitLLVM X(Options::args, Options::argv);

        // Initialize targets first, so that --version shows registered targets.
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmPrinters();
        llvm::InitializeAllAsmParsers();

        // Initialize codegen and IR passes used by llc so that the -print-after,
        // -print-before, and -stop-after options work.
        llvm::PassRegistry *Registry = llvm::PassRegistry::getPassRegistry();
        llvm::initializeCore(*Registry);
        llvm::initializeCodeGen(*Registry);
        llvm::initializeLoopStrengthReducePass(*Registry);
        llvm::initializeLowerIntrinsicsPass(*Registry);
        llvm::initializeUnreachableBlockElimLegacyPassPass(*Registry);
        llvm::initializeConstantHoistingLegacyPassPass(*Registry);
        llvm::initializeScalarOpts(*Registry);
        llvm::initializeVectorization(*Registry);
        llvm::initializeScalarizeMaskedMemIntrinLegacyPassPass(*Registry);
        llvm::initializeExpandReductionsPass(*Registry);
        llvm::initializeExpandVectorPredicationPass(*Registry);
        llvm::initializeHardwareLoopsPass(*Registry);
        llvm::initializeTransformUtils(*Registry);
        llvm::initializeReplaceWithVeclibLegacyPass(*Registry);
        llvm::initializeTLSVariableHoistLegacyPassPass(*Registry);

        // Initialize debugging passes.
        llvm::initializeScavengerTestPass(*Registry);

        // Register the Target and CPU printer for --version.
        llvm::cl::AddExtraVersionPrinter(llvm::sys::printDefaultTargetAndDetectedCPU);
        // Register the target printer for --version.
        llvm::cl::AddExtraVersionPrinter(llvm::TargetRegistry::printRegisteredTargetsForVersion);

        llvm::LLVMContext Context;
        Context.setDiscardValueNames(Options::DiscardValueNames);

        // Set a diagnostic handler that doesn't exit on the first error
        bool HasError = false;
        Context.setDiagnosticHandler(
            std::make_unique<llvm::DiagnosticHandler>(&HasError)); // TODO: Implement my own diagnostic handler

        llvm::Expected<std::unique_ptr<llvm::ToolOutputFile>> RemarksFileOrErr =
            setupLLVMOptimizationRemarks(Context, Options::RemarksFilename, Options::RemarksPasses,
                                         Options::RemarksFormat, Options::RemarksWithHotness,
                                         Options::RemarksHotnessThreshold);
        if (llvm::Error E = RemarksFileOrErr.takeError())
            llvm::handleAllErrors(std::move(E),
                                  [&](const llvm::ErrorInfoBase &EI)
                                  { FATAL_ERROR(EI.message()); });

        std::unique_ptr<llvm::ToolOutputFile> RemarksFile = std::move(*RemarksFileOrErr);

        if (Options::InputLanguage != "" && Options::InputLanguage != "ir" && Options::InputLanguage != "mir")
            FATAL_ERROR("input language must be '', 'IR' or 'MIR'");

        // Compile the module TimeCompilations times to give better compile time
        // metrics.
        for (unsigned I = Options::TimeCompilations; I; --I)
            if (int RetVal = compileModule(Options::argv, Context))
                return RetVal;

        if (RemarksFile)
            RemarksFile->keep();
        return 0;
    }
}