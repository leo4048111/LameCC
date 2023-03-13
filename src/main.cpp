#include "lcc.hpp"

int main(int argc, char **argv)
{
    if(!lcc::Options::ParseOpts(argc, argv)) return false;

    auto file = std::make_shared<lcc::File>(lcc::Options::InputFilename);
    if (file->fail())
    {
        FATAL_ERROR(lcc::Options::InputFilename << ": No such file or directory");
        return 0;
    }

    auto tokens = lcc::Lexer::getInstance()->run(file);

    std::unique_ptr<lcc::AST::Decl> astRoot = nullptr;
    if (lcc::Options::LR1GrammarFilePath != "-")
        astRoot = lcc::LR1Parser::getInstance()->run(
            tokens, lcc::Options::LR1GrammarFilePath, lcc::Options::ShouldPrintLog);
    else
        astRoot = lcc::Parser::getInstance()->run(tokens);

    if (astRoot == nullptr)
        WARNING("Parsed empty AST");


    lcc::dumpJson(lcc::Lexer::jsonifyTokens(tokens), lcc::Options::TokenDumpPath);
    INFO("Tokens have been dumped to " << lcc::Options::TokenDumpPath);

    if (astRoot)
    {
        lcc::dumpJson(astRoot->asJson(), lcc::Options::ASTDumpPath);
        INFO("AST has been dumped to " << lcc::Options::ASTDumpPath);
    }

    if (astRoot)
    {
        if (!astRoot->gen(lcc::LLVMIRGenerator::getInstance()))
            FATAL_ERROR("Failed to generate IR.");
        else
        {
            // lcc::LLVMIRGenerator::getInstance()->printCode();
            lcc::LLVMIRGenerator::getInstance()->dumpCode(lcc::Options::IRDumpPath);
            INFO("IR has been dumped to " << lcc::Options::IRDumpPath);

            if(!lcc::Codegen::getInstance()->run())
            {
                FATAL_ERROR("Failed to generate target assembly.");
            }
            else
            {
                INFO("Target assembly has been generated and dumped to " << lcc::Options::OutputFilename);
            }
        }
    }
}