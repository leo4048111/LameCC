#include "lcc.hpp"

#define LLVMIRGEN_RET_TRUE(val) { _retVal = val; return true; }
#define LLVMIRGEN_RET_FALSE() { _retVal = nullptr; return false; }

namespace lcc
{
    std::unique_ptr<LLVMIRGenerator> LLVMIRGenerator::_inst = nullptr;

    LLVMIRGenerator::LLVMIRGenerator()
    {
        _builder = std::make_unique<llvm::IRBuilder<>>(_context);
        _module = std::make_unique<llvm::Module>("LCC_LLVMIRGenerator", _context);

        changeTable(mkTable());
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

    std::shared_ptr<LLVMIRGenerator::SymbolTable> LLVMIRGenerator::mkTable(std::shared_ptr<SymbolTable> previous)
    {
        auto tbl = std::make_shared<SymbolTable>(previous);
        _tables.push_back(tbl);
        return tbl;
    }

    void LLVMIRGenerator::changeTable(std::shared_ptr<SymbolTable> table)
    {
        _currentSymbolTable = table;
    }

    llvm::Value* LLVMIRGenerator::lookupCurrentTbl(std::string name)
    {
        if (_currentSymbolTable->symbls.find(name) == _currentSymbolTable->symbls.end())
            return nullptr;

        return _currentSymbolTable->symbls[name];
    }

    llvm::Value* LLVMIRGenerator::lookup(std::string name)
    {
        auto currentTbl = _currentSymbolTable;
        llvm::Value* pVal = nullptr;
        for (_currentSymbolTable; _currentSymbolTable != nullptr; changeTable(_currentSymbolTable->previous))
        {
            pVal = lookupCurrentTbl(name);
            if (pVal != nullptr)
                break;
        }
        changeTable(currentTbl);

        return pVal;
    }

    bool LLVMIRGenerator::enter(std::string name, llvm::AllocaInst* alloca)
    {
        if (lookupCurrentTbl(name) != nullptr)
            return false;

        _currentSymbolTable->symbls.insert(std::make_pair(name, alloca));
        return true;
    }

    llvm::AllocaInst* LLVMIRGenerator::createEntryBlockAlloca(llvm::Function *function, const std::string name, const std::string type)
    {
        llvm::IRBuilder<> builder(&function->getEntryBlock(), function->getEntryBlock().begin());

        if(type == "int") return builder.CreateAlloca(llvm::Type::getInt32Ty(_context), 0, name.c_str());
        else if(type == "float") return builder.CreateAlloca(llvm::Type::getFloatTy(_context), 0, name.c_str());
        else return nullptr;
    }

    bool LLVMIRGenerator::gen(AST::TranslationUnitDecl *translationUnitDecl)
    {
        for (auto &decl : translationUnitDecl->_decls)
            if (!decl->gen(this))
                LLVMIRGEN_RET_FALSE();

        LLVMIRGEN_RET_TRUE(nullptr);
    }

    bool LLVMIRGenerator::gen(AST::FunctionDecl *functionDecl) { return true; }

    bool LLVMIRGenerator::gen(AST::VarDecl *varDecl)
    {
        if (lookupCurrentTbl(varDecl->name()) != nullptr)
        {
            FATAL_ERROR("Redeclaration " << varDecl->type() << " " << varDecl->name());
            LLVMIRGEN_RET_FALSE();
        }

        varDecl->place = varDecl->name();

        llvm::Value* initVal = nullptr;
        if (varDecl->_isInitialized)
        {
            if (!varDecl->_value->gen(this))
                LLVMIRGEN_RET_FALSE();

            initVal = _retVal;
        }

        auto ib = _builder->GetInsertBlock();
        if (ib)
        {
            auto function = ib->getParent();
            auto alloca = createEntryBlockAlloca(function, varDecl->name(), varDecl->type());
            enter(varDecl->name(), alloca);
            if(initVal != nullptr)
            {
                auto store = _builder->CreateStore(initVal, alloca);
                LLVMIRGEN_RET_TRUE(store);
            }
        }
        else
        {
            if(_module->getGlobalVariable(varDecl->name()))
            {
                FATAL_ERROR("Redeclaration global variable" << varDecl->type() << " " << varDecl->name());
                LLVMIRGEN_RET_FALSE();
            }

            if(varDecl->type() == "int") 
            {
                llvm::GlobalVariable* gVar = new llvm::GlobalVariable(
                    *_module, llvm::Type::getInt32Ty(_context), false, 
                    llvm::GlobalValue::ExternalLinkage, 
                    _builder->getInt32(0),
                    varDecl->name());

                LLVMIRGEN_RET_TRUE(gVar);    
            }
            // else if(varDecl->type() == "float")
            // {
            //     llvm::GlobalVariable* gVar = new llvm::GlobalVariable(
            //         *_module, llvm::Type::getFloatTy(_context), false, 
            //         llvm::GlobalValue::ExternalLinkage, 
            //         llvm::Constant::ConstantFPVal(0.f),
            //         varDecl->name());

            //     LLVMIRGEN_RET_TRUE(gVar);    
            // }
            else
            {
                LLVMIRGEN_RET_FALSE();
            }
        }

        return true;
    }

    bool LLVMIRGenerator::gen(AST::IntegerLiteral *integerLiteral) 
    { 
        auto val = llvm::ConstantInt::get(_context, llvm::APInt(32, integerLiteral->value(), true));
        LLVMIRGEN_RET_TRUE(val);
    }

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