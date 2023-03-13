#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Remarks/HotnessThresholdParser.h"

#define DEFAULT_TOKEN_DUMP_PATH "tokens.json"
#define DEFAULT_AST_DUMP_PATH "ast.json"
#define DEFAULT_IR_DUMP_PATH "ir.ll"
#define DEFAULT_OUT_FILENAME "a.out"

namespace lcc
{
    class Options
    {
    private:
        Options() = delete;
        Options(const Options &) = delete;
        Options(Options &&) = delete;
        Options &operator=(const Options &) = delete;

        ~Options() = delete;

    public:
        static int args;
        static char** argv;

        static bool ParseOpts(int args, char **argv);

        static llvm::cl::opt<std::string> InputFilename;

        static llvm::cl::opt<std::string> InputLanguage;

        static llvm::cl::opt<std::string> TokenDumpPath;

        static llvm::cl::opt<std::string> ASTDumpPath;

        static llvm::cl::opt<std::string> IRDumpPath;

        static llvm::cl::opt<std::string> LR1GrammarFilePath;

        static llvm::cl::opt<bool> ShouldPrintLog;

        static llvm::cl::opt<std::string> OutputFilename;

        static llvm::cl::opt<std::string> SplitDwarfOutputFile;

        static llvm::cl::opt<unsigned> TimeCompilations;

        static llvm::cl::opt<bool> TimeTrace;

        static llvm::cl::opt<unsigned> TimeTraceGranularity;

        static llvm::cl::opt<std::string> TimeTraceFile;

        static llvm::cl::opt<std::string> BinutilsVersion;

        static llvm::cl::opt<bool> NoIntegratedAssembler;

        static llvm::cl::opt<bool> PreserveComments;

        // Determine optimization level.
        static llvm::cl::opt<char> OptLevel;

        static llvm::cl::opt<std::string> TargetTriple;

        static llvm::cl::opt<std::string> SplitDwarfFile;

        static llvm::cl::opt<bool> NoVerify;

        static llvm::cl::opt<bool> DisableSimplifyLibCalls;

        static llvm::cl::opt<bool> ShowMCEncoding;

        static llvm::cl::opt<bool> DwarfDirectory;

        static llvm::cl::opt<bool> AsmVerbose;

        static llvm::cl::opt<bool> CompileTwice;

        static llvm::cl::opt<bool> DiscardValueNames;

        static llvm::cl::list<std::string> IncludeDirs;

        static llvm::cl::opt<bool> RemarksWithHotness;

        static llvm::cl::opt<std::optional<uint64_t>, false, llvm::remarks::HotnessThresholdParser> RemarksHotnessThreshold;

        static llvm::cl::opt<std::string> RemarksFilename;

        static llvm::cl::opt<std::string> RemarksPasses;

        static llvm::cl::opt<std::string> RemarksFormat;
    };
}