#pragma once

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
// #include "llvm/Support/PluginLoader.h"
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
    class Codegen
    {
    public:
        ~Codegen() = default;
        Codegen(const Codegen &) = delete;
        Codegen &operator=(const Codegen &) = delete;

    private:
        static std::unique_ptr<Codegen> _inst;

        static bool EnableDebugBuffering;

        Codegen() = default;

    public:
        static Codegen *getInstance()
        {
            if (_inst.get() == nullptr)
                _inst.reset(new Codegen);

            return _inst.get();
        }

        bool run();

    private:
        int compileModule(char **argv, llvm::LLVMContext &Context);

        std::unique_ptr<llvm::ToolOutputFile> getOutputStream(const char *TargetName,
                                                              llvm::Triple::OSType OS,
                                                              const char *ProgName);

        std::vector<std::string> &getRunPassNames();

        bool addPass(llvm::PassManagerBase &PM, const char *argv0,
                     llvm::StringRef PassName, llvm::TargetPassConfig &TPC);
    };
}