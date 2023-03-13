#pragma once

#include "llvm/Support/ToolOutputFile.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/CodeGen/MIRParser/MIRParser.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IRReader/IRReader.h"

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