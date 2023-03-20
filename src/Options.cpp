#include "lcc.hpp"

namespace lcc
{
    llvm::codegen::RegisterCodeGenFlags Options::CGF{};

    llvm::cl::opt<std::string>
        Options::TokenDumpPath("token", llvm::cl::desc("Token dump file"), llvm::cl::init("-"));

    llvm::cl::opt<std::string>
        Options::ASTDumpPath("ast", llvm::cl::desc("AST dump file"), llvm::cl::init("-"));

    llvm::cl::opt<std::string>
        Options::LR1GrammarFilePath("lr1", llvm::cl::desc("LR1 grammar file path"),
                                    llvm::cl::init("-"));

    llvm::cl::opt<std::string>
        Options::IRDumpPath("ir", llvm::cl::desc("IR dump file"),
                            llvm::cl::init("-"));

    llvm::cl::opt<bool>
        Options::ShouldPrintLog("log", llvm::cl::desc("Print log"), llvm::cl::init(false));

    llvm::cl::opt<std::string>
        Options::InputFilename(llvm::cl::Positional, llvm::cl::desc("<input source file>"), llvm::cl::init("-"));

    llvm::cl::opt<std::string>
        Options::InputLanguage("x", llvm::cl::desc("Input language ('ir' or 'mir')"));

    llvm::cl::opt<std::string>
        Options::OutputFilename("o", llvm::cl::desc("Output filename"), llvm::cl::value_desc("filename"),
                                llvm::cl::init("-"));

    llvm::cl::opt<std::string>
        Options::SplitDwarfOutputFile("split-dwarf-output",
                                      llvm::cl::desc(".dwo output filename"),
                                      llvm::cl::value_desc("filename"));

    llvm::cl::opt<unsigned>
        Options::TimeCompilations("time-compilations", llvm::cl::Hidden, llvm::cl::init(1u),
                                  llvm::cl::value_desc("N"),
                                  llvm::cl::desc("Repeat compilation N times for timing"));

    llvm::cl::opt<bool>
        Options::TimeTrace("time-trace", llvm::cl::desc("Record time trace"));

    llvm::cl::opt<unsigned>
        Options::TimeTraceGranularity(
            "time-trace-granularity",
            llvm::cl::desc(
                "Minimum time granularity (in microseconds) traced by time profiler"),
            llvm::cl::init(500), llvm::cl::Hidden);

    llvm::cl::opt<std::string>
        Options::TimeTraceFile("time-trace-file",
                               llvm::cl::desc("Specify time trace file destination"),
                               llvm::cl::value_desc("filename"));

    llvm::cl::opt<std::string>
        Options::BinutilsVersion("binutils-version", llvm::cl::Hidden,
                                 llvm::cl::desc("Produced object files can use all ELF features "
                                                "supported by this binutils version and newer."
                                                "If -no-integrated-as is specified, the generated "
                                                "assembly will consider GNU as support."
                                                "'none' means that all ELF features can be used, "
                                                "regardless of binutils support"));

    llvm::cl::opt<bool>
        Options::NoIntegratedAssembler("no-integrated-as", llvm::cl::Hidden,
                                       llvm::cl::desc("Disable integrated assembler"));

    llvm::cl::opt<bool>
        Options::PreserveComments("preserve-as-comments", llvm::cl::Hidden,
                                  llvm::cl::desc("Preserve Comments in outputted assembly"),
                                  llvm::cl::init(true));

    // Determine optimization level.
    llvm::cl::opt<char>
        Options::OptLevel("O",
                          llvm::cl::desc("Optimization level. [-O0, -O1, -O2, or -O3] "
                                         "(default = '-O2')"),
                          llvm::cl::Prefix, llvm::cl::init(' '));

    llvm::cl::opt<std::string>
        Options::TargetTriple("mtriple", llvm::cl::desc("Override target triple for module"));

    llvm::cl::opt<std::string>
        Options::SplitDwarfFile("split-dwarf-file", llvm::cl::desc("Specify the name of the .dwo file to encode in the DWARF output"));

    llvm::cl::opt<bool>
        Options::NoVerify("disable-verify", llvm::cl::Hidden, llvm::cl::desc("Do not verify input module"));

    llvm::cl::opt<bool>
        Options::DisableSimplifyLibCalls("disable-simplify-libcalls", llvm::cl::desc("Disable simplify-libcalls"));

    llvm::cl::opt<bool>
        Options::ShowMCEncoding("show-mc-encoding", llvm::cl::Hidden, llvm::cl::desc("Show encoding in .s output"));

    llvm::cl::opt<bool>
        Options::DwarfDirectory("dwarf-directory", llvm::cl::Hidden,
                                llvm::cl::desc("Use .file directives with an explicit directory"),
                                llvm::cl::init(true));

    llvm::cl::opt<bool>
        Options::AsmVerbose("asm-verbose",
                            llvm::cl::desc("Add comments to directives."),
                            llvm::cl::init(true));

    llvm::cl::opt<bool>
        Options::CompileTwice("compile-twice", llvm::cl::Hidden,
                              llvm::cl::desc("Run everything twice, re-using the same pass "
                                             "manager and verify the result is the same."),
                              llvm::cl::init(false));

    llvm::cl::opt<bool>
        Options::DiscardValueNames(
            "discard-value-names",
            llvm::cl::desc("Discard names from Value (other than GlobalValue)."),
            llvm::cl::init(false), llvm::cl::Hidden);

    llvm::cl::list<std::string>
        Options::IncludeDirs("I", llvm::cl::desc("include search path"));

    llvm::cl::opt<bool>
        Options::RemarksWithHotness(
            "pass-remarks-with-hotness",
            llvm::cl::desc("With PGO, include profile count in optimization remarks"),
            llvm::cl::Hidden);

    llvm::cl::opt<std::optional<uint64_t>, false, llvm::remarks::HotnessThresholdParser>
        Options::RemarksHotnessThreshold(
            "pass-remarks-hotness-threshold",
            llvm::cl::desc("Minimum profile count required for "
                           "an optimization remark to be output. "
                           "Use 'auto' to apply the threshold from profile summary."),
            llvm::cl::value_desc("N or 'auto'"), llvm::cl::init(0), llvm::cl::Hidden);

    llvm::cl::opt<std::string>
        Options::RemarksFilename("pass-remarks-output",
                                 llvm::cl::desc("Output filename for pass remarks"),
                                 llvm::cl::value_desc("filename"));

    llvm::cl::opt<std::string>
        Options::RemarksPasses("pass-remarks-filter",
                               llvm::cl::desc("Only record optimization remarks from passes whose "
                                              "names match the given regular expression"),
                               llvm::cl::value_desc("regex"));

    llvm::cl::opt<std::string>
        Options::RemarksFormat(
            "pass-remarks-format",
            llvm::cl::desc("The format used for serializing remarks (default: YAML)"),
            llvm::cl::value_desc("format"), llvm::cl::init("yaml"));

    int Options::args = 0;
    char** Options::argv = nullptr;

    bool Options::ParseOpts(int args, char **argv)
    {
        Options::args = args;
        Options::argv = argv;
        llvm::cl::ParseCommandLineOptions(args, argv, "LCC");

        if (InputFilename == "-")
        {
            FATAL_ERROR("No input file specified");
            return false;
        }

        if (ShouldPrintLog && LR1GrammarFilePath == "-")
        {
            WARNING("--log option must be used with --LR1 option, ignored --log");
        }

        if (TokenDumpPath == "-")
        {
            TokenDumpPath = DEFAULT_TOKEN_DUMP_PATH;
            WARNING("Token dump path not set, assuming " << DEFAULT_TOKEN_DUMP_PATH);
        }

        if (ASTDumpPath == "-")
        {
            ASTDumpPath = DEFAULT_AST_DUMP_PATH;
            WARNING("AST dump path not set, assuming " << DEFAULT_AST_DUMP_PATH);
        }

        if (OutputFilename == "-")
        {
            OutputFilename = DEFAULT_OUT_FILENAME;
            WARNING("Output filename not set, assuming " << DEFAULT_OUT_FILENAME);
        }

        if (IRDumpPath == "-")
        {
            IRDumpPath = DEFAULT_IR_DUMP_PATH;
            WARNING("IR dump path not set, assuming " << DEFAULT_IR_DUMP_PATH);
        }

        return true;
    }
}