#include "lcc.hpp"

#define LLVMIRGEN_RET_TRUE(val) \
    {                           \
        _retVal = val;          \
        return true;            \
    }
#define LLVMIRGEN_RET_FALSE() \
    {                         \
        _retVal = nullptr;    \
        return false;         \
    }

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

    llvm::Value *LLVMIRGenerator::lookupCurrentTbl(std::string name)
    {
        if (_currentSymbolTable->symbls.find(name) == _currentSymbolTable->symbls.end())
            return nullptr;

        return _currentSymbolTable->symbls[name];
    }

    llvm::Value *LLVMIRGenerator::lookup(std::string name)
    {
        auto currentTbl = _currentSymbolTable;
        llvm::Value *pVal = nullptr;
        for (_currentSymbolTable; _currentSymbolTable != nullptr; changeTable(_currentSymbolTable->previous))
        {
            pVal = lookupCurrentTbl(name);
            if (pVal != nullptr)
                break;
        }
        changeTable(currentTbl);

        if (pVal != nullptr)
            return pVal;

        pVal = _module->getGlobalVariable(name);

        return pVal;
    }

    bool LLVMIRGenerator::enter(std::string name, llvm::AllocaInst *alloca)
    {
        if (lookupCurrentTbl(name) != nullptr)
            return false;

        _currentSymbolTable->symbls.insert(std::make_pair(name, alloca));
        return true;
    }

    llvm::AllocaInst *LLVMIRGenerator::createEntryBlockAlloca(llvm::Function *function, const std::string &name, const std::string type)
    {
        llvm::IRBuilder<> builder(&function->getEntryBlock(), function->getEntryBlock().begin());

        if (type == "int")
            return builder.CreateAlloca(llvm::Type::getInt32Ty(_context), 0, name.c_str());
        else if (type == "float")
            return builder.CreateAlloca(llvm::Type::getFloatTy(_context), 0, name.c_str());
        else
            return nullptr;
    }

    void LLVMIRGenerator::updateFuncContext(llvm::Function *pFunc, llvm::BasicBlock *entryBB, llvm::BasicBlock *retBB, llvm::AllocaInst *retValAlloca)
    {
        _fc.pFunc = pFunc;
        _fc.entryBB = entryBB;
        _fc.retBB = retBB;
        _fc.retValAlloca = retValAlloca;
    }

    bool LLVMIRGenerator::gen(AST::TranslationUnitDecl *translationUnitDecl)
    {
        for (auto &decl : translationUnitDecl->_decls)
            if (!decl->gen(this))
                LLVMIRGEN_RET_FALSE();

        LLVMIRGEN_RET_TRUE(nullptr);
    }

    bool LLVMIRGenerator::gen(AST::FunctionDecl *functionDecl)
    {
        std::vector<llvm::Type *> params;
        for (auto &param : functionDecl->_params)
        {
            if (param->type() == "int")
                params.push_back(llvm::Type::getInt32Ty(_context));
            else if (param->type() == "char")
                params.push_back(llvm::Type::getFloatTy(_context));
            else
                LLVMIRGEN_RET_FALSE();
        }

        llvm::FunctionType *ft = nullptr;
        llvm::Type *funcRetType = nullptr;

        if (functionDecl->_type == "void")
            funcRetType = llvm::Type::getVoidTy(_context);
        else if (functionDecl->_type == "int")
            funcRetType = llvm::Type::getInt32Ty(_context);
        else if (functionDecl->_type == "float")
            funcRetType = llvm::Type::getFloatTy(_context);
        else
        {
            FATAL_ERROR("Unsupported return type for function " << functionDecl->name());
            LLVMIRGEN_RET_FALSE();
        }

        ft = llvm::FunctionType::get(funcRetType, params, false);
        auto func = _module->getFunction(functionDecl->name());

        if (func != nullptr)
        {
            if (!func->empty() || (functionDecl->_body != nullptr))
            {
                FATAL_ERROR("Redefinition function " << functionDecl->name());
                LLVMIRGEN_RET_FALSE();
            }
            else if (func->arg_size() != functionDecl->_params.size())
            {
                FATAL_ERROR("Function " << functionDecl->name() << "definition deosn't match with declaration");
                LLVMIRGEN_RET_FALSE();
            }
        }
        else
            func = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, functionDecl->name(), _module.get());

        unsigned int idx = 0;
        for (auto &arg : func->args())
        {
            arg.setName(functionDecl->_params[idx++]->name());
        }

        if (functionDecl->_body == nullptr)
            LLVMIRGEN_RET_TRUE(func);

        llvm::BasicBlock *entryBB = llvm::BasicBlock::Create(_context, "entry", func);
        llvm::BasicBlock *retBB = llvm::BasicBlock::Create(_context, "return", func);
        llvm::AllocaInst *retValAlloca = nullptr;

        _builder->SetInsertPoint(retBB);

        if (functionDecl->_type == "void")
            retValAlloca = nullptr;
        else if (
            functionDecl->_type == "int" ||
            functionDecl->_type == "float")
        {
            retValAlloca = createEntryBlockAlloca(func, "retVal", functionDecl->_type);
        }
        else
        {
            FATAL_ERROR("Unsupported return value in function " << functionDecl->name());
            LLVMIRGEN_RET_FALSE();
        }

        if (retValAlloca)
        {
            auto retVal = _builder->CreateLoad(funcRetType, retValAlloca);
            _builder->CreateRet(retVal);
        }
        else
            _builder->CreateRetVoid(); // emit ret void

        _builder->SetInsertPoint(entryBB);

        auto previousTable = _currentSymbolTable;
        changeTable(mkTable(previousTable)); // create a new scope for function params

        // Alloc space for function params
        for (auto &arg : func->args())
        {
            llvm::AllocaInst *alloca = nullptr;
            if (arg.getType()->isIntegerTy())
                alloca = createEntryBlockAlloca(func, arg.getName().str(), "int");
            else if (arg.getType()->isFloatTy())
                alloca = createEntryBlockAlloca(func, arg.getName().str(), "float");
            else
            {
                FATAL_ERROR("Unsupported param type in function " << func->getName().str());
                arg.getType()->print(llvm::outs());
                LLVMIRGEN_RET_FALSE();
            }

            _builder->CreateStore(&arg, alloca);
            enter(arg.getName().str(), alloca);
        }

        updateFuncContext(func, entryBB, retBB, retValAlloca);

        functionDecl->_body->gen(this);

        _builder->CreateBr(retBB); // unconditional jump to return bb after function body

        std::string err;
        llvm::raw_ostream *out = new llvm::raw_string_ostream(err);
        if (llvm::verifyFunction(*func, out))
        {
            FATAL_ERROR(err);
            LLVMIRGEN_RET_FALSE();
        }

        changeTable(previousTable);
        LLVMIRGEN_RET_TRUE(nullptr);
    }

    bool LLVMIRGenerator::gen(AST::VarDecl *varDecl)
    {
        if (lookupCurrentTbl(varDecl->name()) != nullptr)
        {
            FATAL_ERROR("Redefinition " << varDecl->type() << " " << varDecl->name());
            LLVMIRGEN_RET_FALSE();
        }

        varDecl->place = varDecl->name();

        llvm::Value *initVal = nullptr;
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
            if (initVal != nullptr)
            {
                auto store = _builder->CreateStore(initVal, alloca);
                LLVMIRGEN_RET_TRUE(store);
            }
        }
        else
        {
            if (_module->getGlobalVariable(varDecl->name()))
            {
                FATAL_ERROR("Redeclaration global variable" << varDecl->type() << " " << varDecl->name());
                LLVMIRGEN_RET_FALSE();
            }

            if (varDecl->type() == "int")
            {
                llvm::GlobalVariable *gVar = new llvm::GlobalVariable(
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

    bool LLVMIRGenerator::gen(AST::FloatingLiteral *floatingLiteral)
    {
        auto val = llvm::ConstantFP::get(_context, llvm::APFloat(floatingLiteral->value()));
        LLVMIRGEN_RET_TRUE(val);
    }

    bool LLVMIRGenerator::gen(AST::DeclRefExpr *declRefExpr)
    {
        auto alloca = lookup(declRefExpr->name());

        if (alloca == nullptr)
        {
            FATAL_ERROR("Referencing undefined symbol " << declRefExpr->name());
            LLVMIRGEN_RET_FALSE();
        }

        declRefExpr->place = declRefExpr->name();

        LLVMIRGEN_RET_TRUE(alloca);
    }

    bool LLVMIRGenerator::gen(AST::CastExpr *castExpr)
    {
        if (!castExpr->_subExpr->gen(this))
            LLVMIRGEN_RET_FALSE();

        auto ld = _builder->CreateLoad(_retVal->getType(), _retVal);

        LLVMIRGEN_RET_TRUE(ld);
    }

    bool LLVMIRGenerator::gen(AST::BinaryOperator *binaryOperator)
    {
        return true;
    }

    bool LLVMIRGenerator::gen(AST::UnaryOperator *unaryOperator)
    {
        return true;
    }

    bool LLVMIRGenerator::gen(AST::ParenExpr *parenExpr)
    {
        if (!parenExpr->gen(this))
            LLVMIRGEN_RET_FALSE();

        LLVMIRGEN_RET_TRUE(_retVal);
    }

    bool LLVMIRGenerator::gen(AST::CompoundStmt *compoundStmt)
    {
        auto previousTable = _currentSymbolTable;
        changeTable(mkTable(previousTable));
        for (auto &stmt : compoundStmt->_body)
        {
            if (!stmt->gen(this))
                LLVMIRGEN_RET_FALSE();
        }

        changeTable(previousTable);
        LLVMIRGEN_RET_TRUE(_retVal);
    }

    bool LLVMIRGenerator::gen(AST::DeclStmt *declStmt)
    {
        for (auto &decl : declStmt->_decls)
        {
            if (!decl->gen(this))
                LLVMIRGEN_RET_FALSE();
        }

        LLVMIRGEN_RET_TRUE(_retVal);
    }

    bool LLVMIRGenerator::gen(AST::IfStmt *ifStmt) { return true; }
    bool LLVMIRGenerator::gen(AST::ValueStmt *valueStmt)
    {
        LLVMIRGEN_RET_TRUE(nullptr);
    }

    bool LLVMIRGenerator::gen(AST::ReturnStmt *returnStmt)
    {
        llvm::Function *func = _builder->GetInsertBlock()->getParent();

        if (returnStmt->_value == nullptr)
        {
            _builder->CreateBr(_fc.retBB);
        }
        else
        {
            if (!returnStmt->_value->gen(this))
                LLVMIRGEN_RET_FALSE();

            _builder->CreateStore(_retVal, _fc.retValAlloca);
        }

        LLVMIRGEN_RET_TRUE(_retVal);
    }

    bool LLVMIRGenerator::gen(AST::WhileStmt *whileStmt) { return true; }
    bool LLVMIRGenerator::gen(AST::CallExpr *callExpr)
    {
        return true;
    }
}