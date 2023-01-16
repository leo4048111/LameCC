#include "IRGenerator.hpp"

namespace lcc
{
    std::unique_ptr<LLVMIRGenerator> LLVMIRGenerator::_inst = nullptr;

    LLVMIRGenerator::LLVMIRGenerator()
    {
        _builder = std::make_unique<llvm::IRBuilder<>>(_context);
        _module = std::make_unique<llvm::Module>("LLVMIRGenerator", _context);
    }

    void LLVMIRGenerator::dumpCode(const std::string outPath) const
    {
        // TODO dumper
        std::error_code err;
        auto os = std::make_unique<llvm::raw_fd_ostream>(outPath, err);
        _module->print(*os, nullptr, false, false);
        return;
    }

    void LLVMIRGenerator::printCode() const
    {
        // TODO debug print
        _module->print(llvm::outs(), nullptr);
    }

    bool LLVMIRGenerator::gen(AST::TranslationUnitDecl *translationUnitDecl) { return true; }
    bool LLVMIRGenerator::gen(AST::FunctionDecl *functionDecl) { return true; }
    bool LLVMIRGenerator::gen(AST::VarDecl *varDecl) { return true; }
    bool LLVMIRGenerator::gen(AST::IntegerLiteral *integerLiteral) { return true; }
    bool LLVMIRGenerator::gen(AST::FloatingLiteral *floatingLiteral) { return true; }
    bool LLVMIRGenerator::gen(AST::DeclRefExpr *declRefExpr) { return true; }
    bool LLVMIRGenerator::gen(AST::CastExpr *castExpr) { return true; }
    bool LLVMIRGenerator::gen(AST::BinaryOperator *binaryOperator) { return true; }
    bool LLVMIRGenerator::gen(AST::UnaryOperator *unaryOperator) { return true; }
    bool LLVMIRGenerator::gen(AST::ParenExpr *parenExpr) { return true; }
    bool LLVMIRGenerator::gen(AST::CompoundStmt *compoundStmt) { return true; }
    bool LLVMIRGenerator::gen(AST::DeclStmt *declStmt) { return true; }
    bool LLVMIRGenerator::gen(AST::IfStmt *ifStmt) { return true; }
    bool LLVMIRGenerator::gen(AST::ValueStmt *valueStmt) { return true; }
    bool LLVMIRGenerator::gen(AST::ReturnStmt *returnStmt) { return true; }
    bool LLVMIRGenerator::gen(AST::WhileStmt *whileStmt) { return true; }
    bool LLVMIRGenerator::gen(AST::CallExpr *callExpr) { return true; }
}