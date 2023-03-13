#include "lcc.hpp"

namespace lcc
{
    std::unique_ptr<Codegen> Codegen::_inst;

    // Default enable debug stream buffering.
    bool Codegen::EnableDebugBuffering = true;

    std::vector<std::string> &Codegen::getRunPassNames()
    {
        static std::vector<std::string> RunPassNames;
        return RunPassNames;
    }

    bool Codegen::addPass(llvm::PassManagerBase &PM, const char *argv0,
                          llvm::StringRef PassName, llvm::TargetPassConfig &TPC)
    {
        if (PassName == "none")
            return false;

        const llvm::PassRegistry *PR = llvm::PassRegistry::getPassRegistry();
        const llvm::PassInfo *PI = PR->getPassInfo(PassName);
        if (!PI)
        {
            FATAL_ERROR("run-pass " << PassName.str() << " is not registered");
            return true;
        }

        llvm::Pass *P;
        if (PI->getNormalCtor())
            P = PI->getNormalCtor()();
        else
        {
            FATAL_ERROR("cannot create pass: " << PI->getPassName().str());
            return true;
        }
        std::string Banner = std::string("After ") + std::string(P->getPassName());
        TPC.addMachinePrePasses();
        PM.add(P);
        TPC.addMachinePostPasses(Banner);
        return false;
    }

    std::unique_ptr<llvm::ToolOutputFile> Codegen::getOutputStream(const char *TargetName,
                                                                   llvm::Triple::OSType OS,
                                                                   const char *ProgName)
    {
        // If we don't yet have an output filename, make one.
        if (Options::OutputFilename.empty())
        {
            if (Options::IRDumpPath == "-")
                Options::OutputFilename = "-";
            else
            {
                // If InputFilename ends in .bc or .ll, remove it.
                llvm::StringRef IFN = Options::IRDumpPath;
                if (IFN.endswith(".bc") || IFN.endswith(".ll"))
                    Options::OutputFilename = std::string(IFN.drop_back(3));
                else if (IFN.endswith(".mir"))
                    Options::OutputFilename = std::string(IFN.drop_back(4));
                else
                    Options::OutputFilename = std::string(IFN);

                switch (llvm::codegen::getFileType())
                {
                case llvm::CGFT_AssemblyFile:
                    if (TargetName[0] == 'c')
                    {
                        if (TargetName[1] == 0)
                            Options::OutputFilename += ".cbe.c";
                        else if (TargetName[1] == 'p' && TargetName[2] == 'p')
                            Options::OutputFilename += ".cpp";
                        else
                            Options::OutputFilename += ".s";
                    }
                    else
                        Options::OutputFilename += ".s";
                    break;
                case llvm::CGFT_ObjectFile:
                    if (OS == llvm::Triple::Win32)
                        Options::OutputFilename += ".obj";
                    else
                        Options::OutputFilename += ".o";
                    break;
                case llvm::CGFT_Null:
                    Options::OutputFilename = "-";
                    break;
                }
            }
        }

        // Decide if we need "binary" output.
        bool Binary = false;
        switch (llvm::codegen::getFileType())
        {
        case llvm::CGFT_AssemblyFile:
            break;
        case llvm::CGFT_ObjectFile:
        case llvm::CGFT_Null:
            Binary = true;
            break;
        }

        // Open the file.
        std::error_code EC;
        llvm::sys::fs::OpenFlags OpenFlags = llvm::sys::fs::OF_None;
        if (!Binary)
            OpenFlags |= llvm::sys::fs::OF_TextWithCRLF;
        auto FDOut = std::make_unique<llvm::ToolOutputFile>(Options::OutputFilename, EC, OpenFlags);
        if (EC)
        {
            FATAL_ERROR(EC.message());
            return nullptr;
        }

        return FDOut;
    }

    int Codegen::compileModule(char **argv, llvm::LLVMContext &Context)
    {
        // Load the module to be compiled...
        llvm::SMDiagnostic Err;
        std::unique_ptr<llvm::Module> M;
        std::unique_ptr<llvm::MIRParser> MIR;
        llvm::Triple TheTriple;
        std::string CPUStr = llvm::codegen::getCPUStr(),
                    FeaturesStr = llvm::codegen::getFeaturesStr();

        // Set attributes on functions as loaded from MIR from command line arguments.
        auto setMIRFunctionAttributes = [&CPUStr, &FeaturesStr](llvm::Function &F)
        {
            llvm::codegen::setFunctionAttributes(CPUStr, FeaturesStr, F);
        };

        auto MAttrs = llvm::codegen::getMAttrs();
        bool SkipModule =
            CPUStr == "help" || (!MAttrs.empty() && MAttrs.front() == "help");

        llvm::CodeGenOpt::Level OLvl = llvm::CodeGenOpt::Default;
        switch (Options::OptLevel)
        {
        default:
            FATAL_ERROR("invalid optimization level");
            return 1;
        case ' ':
            break;
        case '0':
            OLvl = llvm::CodeGenOpt::None;
            break;
        case '1':
            OLvl = llvm::CodeGenOpt::Less;
            break;
        case '2':
            OLvl = llvm::CodeGenOpt::Default;
            break;
        case '3':
            OLvl = llvm::CodeGenOpt::Aggressive;
            break;
        }

        // Parse 'none' or '$major.$minor'. Disallow -binutils-version=0 because we
        // use that to indicate the MC default.
        if (!Options::BinutilsVersion.empty() && Options::BinutilsVersion != "none")
        {
            llvm::StringRef V = Options::BinutilsVersion.getValue();
            unsigned Num;
            if (V.consumeInteger(10, Num) || Num == 0 ||
                !(V.empty() ||
                  (V.consume_front(".") && !V.consumeInteger(10, Num) && V.empty())))
            {
                FATAL_ERROR("invalid -binutils-version, accepting 'none' or major.minor");
                return 1;
            }
        }
        llvm::TargetOptions Opts;
        auto InitializeOptions = [&](const llvm::Triple &TheTriple)
        {
            Opts = llvm::codegen::InitTargetOptionsFromCodeGenFlags(TheTriple);
            Opts.BinutilsVersion =
                llvm::TargetMachine::parseBinutilsVersion(Options::BinutilsVersion);
            Opts.DisableIntegratedAS = Options::NoIntegratedAssembler;
            Opts.MCOptions.ShowMCEncoding = Options::ShowMCEncoding;
            Opts.MCOptions.AsmVerbose = Options::AsmVerbose;
            Opts.MCOptions.PreserveAsmComments = Options::PreserveComments;
            Opts.MCOptions.IASSearchPaths = Options::IncludeDirs;
            Opts.MCOptions.SplitDwarfFile = Options::SplitDwarfFile;
            if (Options::DwarfDirectory.getPosition())
            {
                Opts.MCOptions.MCUseDwarfDirectory =
                    Options::DwarfDirectory ? llvm::MCTargetOptions::EnableDwarfDirectory
                                            : llvm::MCTargetOptions::DisableDwarfDirectory;
            }
            else
            {
                // -dwarf-directory is not set explicitly. Some assemblers
                // (e.g. GNU as or ptxas) do not support `.file directory'
                // syntax prior to DWARFv5. Let the target decide the default
                // value.
                Opts.MCOptions.MCUseDwarfDirectory =
                    llvm::MCTargetOptions::DefaultDwarfDirectory;
            }
        };

        std::optional<llvm::Reloc::Model> RM = llvm::codegen::getExplicitRelocModel();
        std::optional<llvm::CodeModel::Model> CM = llvm::codegen::getExplicitCodeModel();

        const llvm::Target *TheTarget = nullptr;
        std::unique_ptr<llvm::TargetMachine> Target;

        // If user just wants to list available options, skip module loading
        if (!SkipModule)
        {
            auto SetDataLayout =
                [&](llvm::StringRef DataLayoutTargetTriple) -> std::optional<std::string>
            {
                // If we are supposed to override the target triple, do so now.
                std::string IRTargetTriple = DataLayoutTargetTriple.str();
                if (!Options::TargetTriple.empty())
                    IRTargetTriple = llvm::Triple::normalize(Options::TargetTriple);
                TheTriple = llvm::Triple(IRTargetTriple);
                if (TheTriple.getTriple().empty())
                    TheTriple.setTriple(llvm::sys::getDefaultTargetTriple());

                std::string Error;
                TheTarget =
                    llvm::TargetRegistry::lookupTarget(llvm::codegen::getMArch(), TheTriple, Error);
                if (!TheTarget)
                {
                    FATAL_ERROR(Error);
                    exit(1);
                }

                // On AIX, setting the relocation model to anything other than PIC is
                // considered a user error.
                if (TheTriple.isOSAIX() && RM && *RM != llvm::Reloc::PIC_)
                    FATAL_ERROR("invalid relocation model, AIX only supports PIC");

                InitializeOptions(TheTriple);
                Target = std::unique_ptr<llvm::TargetMachine>(TheTarget->createTargetMachine(
                    TheTriple.getTriple(), CPUStr, FeaturesStr, Opts, RM, CM, OLvl));
                assert(Target && "Could not allocate target machine!");

                return Target->createDataLayout().getStringRepresentation();
            };
            if (Options::InputLanguage == "mir" ||
                (Options::InputLanguage == "" && llvm::StringRef(Options::IRDumpPath).endswith(".mir")))
            {
                MIR = llvm::createMIRParserFromFile(Options::IRDumpPath, Err, Context,
                                                    setMIRFunctionAttributes);
                if (MIR)
                    M = MIR->parseIRModule(SetDataLayout);
            }
            else
            {
                M = llvm::parseIRFile(Options::IRDumpPath, Err, Context, SetDataLayout);
            }
            if (!M)
            {
                return 1;
            }
            if (!Options::TargetTriple.empty())
                M->setTargetTriple(llvm::Triple::normalize(Options::TargetTriple));

            std::optional<llvm::CodeModel::Model> CM_IR = M->getCodeModel();
            if (!CM && CM_IR)
                Target->setCodeModel(*CM_IR);
        }
        else
        {
            TheTriple = llvm::Triple(llvm::Triple::normalize(Options::TargetTriple));
            if (TheTriple.getTriple().empty())
                TheTriple.setTriple(llvm::sys::getDefaultTargetTriple());

            // Get the target specific parser.
            std::string Error;
            TheTarget =
                llvm::TargetRegistry::lookupTarget(llvm::codegen::getMArch(), TheTriple, Error);
            if (!TheTarget)
            {
                FATAL_ERROR(Error);
                return 1;
            }

            // On AIX, setting the relocation model to anything other than PIC is
            // considered a user error.
            if (TheTriple.isOSAIX() && RM && *RM != llvm::Reloc::PIC_)
            {
                FATAL_ERROR("invalid relocation model, AIX only supports PIC.\n");
                return 1;
            }

            InitializeOptions(TheTriple);
            Target = std::unique_ptr<llvm::TargetMachine>(TheTarget->createTargetMachine(
                TheTriple.getTriple(), CPUStr, FeaturesStr, Opts, RM, CM, OLvl));
            assert(Target && "Could not allocate target machine!");

            // If we don't have a module then just exit now. We do this down
            // here since the CPU/Feature help is underneath the target machine
            // creation.
            return 0;
        }

        assert(M && "Should have exited if we didn't have a module!");
        if (llvm::codegen::getFloatABIForCalls() != llvm::FloatABI::Default)
            Opts.FloatABIType = llvm::codegen::getFloatABIForCalls();

        // Build up all of the passes that we want to do to the module.
        llvm::legacy::PassManager PM;

        // Figure out where we are going to send the output.
        std::unique_ptr<llvm::ToolOutputFile> Out =
            getOutputStream(TheTarget->getName(), TheTriple.getOS(), argv[0]);
        if (!Out)
            return 1;

        // Ensure the filename is passed down to CodeViewDebug.
        Target->Options.ObjectFilenameForDebug = Out->outputFilename();

        std::unique_ptr<llvm::ToolOutputFile> DwoOut;
        if (!Options::SplitDwarfOutputFile.empty())
        {
            std::error_code EC;
            DwoOut = std::make_unique<llvm::ToolOutputFile>(Options::SplitDwarfOutputFile, EC, llvm::sys::fs::OF_None);
            if (EC)
                FATAL_ERROR(EC.message());
        }

        // Add an appropriate TargetLibraryInfo pass for the module's triple.
        llvm::TargetLibraryInfoImpl TLII(llvm::Triple(M->getTargetTriple()));

        // The -disable-simplify-libcalls flag actually disables all builtin optzns.
        if (Options::DisableSimplifyLibCalls)
            TLII.disableAllFunctions();
        PM.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

        // Verify module immediately to catch problems before doInitialization() is
        // called on any passes.
        if (!Options::NoVerify && verifyModule(*M, &llvm::errs()))
            FATAL_ERROR("input module cannot be verified: " << Options::IRDumpPath);

        // Override function attributes based on CPUStr, FeaturesStr, and command line
        // flags.
        llvm::codegen::setFunctionAttributes(CPUStr, FeaturesStr, *M);

        if (llvm::mc::getExplicitRelaxAll() && llvm::codegen::getFileType() != llvm::CGFT_ObjectFile)
            WARNING("ignoring -mc-relax-all because filetype != obj");

        {
            llvm::raw_pwrite_stream *OS = &Out->os();

            // Manually do the buffering rather than using buffer_ostream,
            // so we can memcmp the contents in CompileTwice mode
            llvm::SmallVector<char, 0> Buffer;
            std::unique_ptr<llvm::raw_svector_ostream> BOS;
            if ((llvm::codegen::getFileType() != llvm::CGFT_AssemblyFile && !Out->os().supportsSeeking()) || Options::CompileTwice)
            {
                BOS = std::make_unique<llvm::raw_svector_ostream>(Buffer);
                OS = BOS.get();
            }

            const char *argv0 = argv[0];
            llvm::LLVMTargetMachine &LLVMTM = static_cast<llvm::LLVMTargetMachine &>(*Target);
            llvm::MachineModuleInfoWrapperPass *MMIWP =
                new llvm::MachineModuleInfoWrapperPass(&LLVMTM);

            // Construct a custom pass pipeline that starts after instruction
            // selection.
            if (!getRunPassNames().empty())
            {
                if (!MIR)
                {
                    WARNING("run-pass is for .mir file only");
                    return 1;
                }
                llvm::TargetPassConfig &TPC = *LLVMTM.createPassConfig(PM);
                if (TPC.hasLimitedCodeGenPipeline())
                {
                    WARNING("run-pass cannot be used with " << TPC.getLimitedCodeGenPipelineReason(" and "));
                    return 1;
                }

                TPC.setDisableVerify(Options::NoVerify);
                PM.add(&TPC);
                PM.add(MMIWP);
                TPC.printAndVerify("");
                for (const std::string &RunPassName : getRunPassNames())
                {
                    if (addPass(PM, argv0, RunPassName, TPC))
                        return 1;
                }
                TPC.setInitialized();
                PM.add(llvm::createPrintMIRPass(*OS));
                PM.add(llvm::createFreeMachineFunctionPass());
            }
            else if (Target->addPassesToEmitFile(
                         PM, *OS, DwoOut ? &DwoOut->os() : nullptr,
                         llvm::codegen::getFileType(), Options::NoVerify, MMIWP))
            {
                FATAL_ERROR("target does not support generation of this file type");
            }

            const_cast<llvm::TargetLoweringObjectFile *>(LLVMTM.getObjFileLowering())
                ->Initialize(MMIWP->getMMI().getContext(), *Target);
            if (MIR)
            {
                assert(MMIWP && "Forgot to create MMIWP?");
                if (MIR->parseMachineFunctions(*M, MMIWP->getMMI()))
                    return 1;
            }

            // Before executing passes, print the final values of the LLVM options.
            llvm::cl::PrintOptionValues();

            // If requested, run the pass manager over the same module again,
            // to catch any bugs due to persistent state in the passes. Note that
            // opt has the same functionality, so it may be worth abstracting this out
            // in the future.
            llvm::SmallVector<char, 0> CompileTwiceBuffer;
            if (Options::CompileTwice)
            {
                std::unique_ptr<llvm::Module> M2(llvm::CloneModule(*M));
                PM.run(*M2);
                CompileTwiceBuffer = Buffer;
                Buffer.clear();
            }

            PM.run(*M);

            // Compare the two outputs and make sure they're the same
            if (Options::CompileTwice)
            {
                if (Buffer.size() != CompileTwiceBuffer.size() ||
                    (memcmp(Buffer.data(), CompileTwiceBuffer.data(), Buffer.size()) !=
                     0))
                {
                    llvm::errs()
                        << "Running the pass manager twice changed the output.\n"
                           "Writing the result of the second run to the specified output\n"
                           "To generate the one-run comparison binary, just run without\n"
                           "the compile-twice option\n";
                    Out->os() << Buffer;
                    Out->keep();
                    return 1;
                }
            }

            if (BOS)
            {
                Out->os() << Buffer;
            }
        }

        // Declare success.
        Out->keep();
        if (DwoOut)
            DwoOut->keep();

        return 0;
    }

    bool Codegen::run()
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
                return false;

        if (RemarksFile)
            RemarksFile->keep();
        return true;
    }
}