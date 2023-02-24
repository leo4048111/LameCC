#include "lcc.hpp"

#define LLVMIRGEN_RET_TRUE(val) \
    {                           \
        _retVal = (val);        \
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

        pVal = _module->getGlobalVariable(name); // llvm::GlobalVariable is a pointer to the actual global var

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
        else if (type == "char")
            return builder.CreateAlloca(llvm::Type::getInt8Ty(_context), 0, name.c_str());
        else
            return nullptr;
    }

    void LLVMIRGenerator::updateFuncContext(llvm::BasicBlock *entryBB, llvm::BasicBlock *retBB, llvm::AllocaInst *retValAlloca)
    {
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
        else if (functionDecl->_type == "char")
            funcRetType = llvm::Type::getInt8Ty(_context);
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
            functionDecl->_type == "float" ||
            functionDecl->_type == "char")
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

        updateFuncContext(entryBB, retBB, retValAlloca);

        functionDecl->_body->gen(this);

        _builder->CreateBr(retBB); // unconditional jump to return bb after function body

        std::string err;
        llvm::raw_ostream *out = new llvm::raw_string_ostream(err);
        if (llvm::verifyFunction(*func, out))
        {
            FATAL_ERROR(err);
            // LLVMIRGEN_RET_FALSE();
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
        auto var = lookup(declRefExpr->name());

        if (var == nullptr)
        {
            FATAL_ERROR("Referencing undefined symbol " << declRefExpr->name());
            LLVMIRGEN_RET_FALSE();
        }

        declRefExpr->place = declRefExpr->name();

        LLVMIRGEN_RET_TRUE(var);
    }

    bool LLVMIRGenerator::gen(AST::CastExpr *castExpr)
    {
        if (!castExpr->_subExpr->gen(this))
            LLVMIRGEN_RET_FALSE();

        auto ld = _builder->CreateLoad(_retVal->getType(), _retVal);

        castExpr->place = castExpr->_subExpr->place;

        LLVMIRGEN_RET_TRUE(ld);
    }

    bool LLVMIRGenerator::gen(AST::ImplicitCastExpr *implicitCastExpr)
    {
        if (!implicitCastExpr->_subExpr->gen(this))
            LLVMIRGEN_RET_FALSE();

        if (implicitCastExpr->_type == AST::CastExpr::CastType::LValueToRValue)
        {
            if (auto alloca = llvm::dyn_cast<llvm::AllocaInst>(_retVal))
            {
                auto ld = _builder->CreateLoad(alloca->getAllocatedType(), alloca);
                LLVMIRGEN_RET_TRUE(ld);
            }
            else if (auto glbVar = llvm::dyn_cast<llvm::GlobalVariable>(_retVal))
            {
                auto ld = _builder->CreateLoad(glbVar->getValueType(), glbVar);
                LLVMIRGEN_RET_TRUE(ld);
            }
            else
            {
                LLVMIRGEN_RET_TRUE(_retVal);
            }
        }

        implicitCastExpr->place = implicitCastExpr->_subExpr->place;
        LLVMIRGEN_RET_TRUE(_retVal);
    }

    bool LLVMIRGenerator::gen(AST::BinaryOperator *binaryOperator)
    {
        if (binaryOperator->isAssignment())
        {
            auto lhs = dynamic_pointer_cast<AST::DeclRefExpr>(std::move(binaryOperator->_lhs));

            if (lhs == nullptr)
            {
                FATAL_ERROR("Invalid lhs expression for an assignment");
                LLVMIRGEN_RET_FALSE();
            }

            if (!binaryOperator->_rhs->gen(this))
                LLVMIRGEN_RET_FALSE();

            llvm::Value *rhsVal = _retVal;
            llvm::Value *lhsVar = lookup(lhs->name());

            if (!lhsVar)
            {
                FATAL_ERROR("Referencing undefined symbol" << lhs->name());
                LLVMIRGEN_RET_FALSE();
            }

            llvm::Value *lhsVal = nullptr;

            if (llvm::AllocaInst *alloca = llvm::dyn_cast<llvm::AllocaInst>(lhsVar))
                lhsVal = _builder->CreateLoad(alloca->getAllocatedType(), alloca);
            else if (llvm::GlobalVariable *glbVar = llvm::dyn_cast<llvm::GlobalVariable>(lhsVar))
                lhsVal = _builder->CreateLoad(glbVar->getValueType(), glbVar);
            else
                LLVMIRGEN_RET_FALSE();

            llvm::Value *exprVal = nullptr;

            switch (binaryOperator->type())
            {
            case AST::BinaryOpType::BO_Assign:
            {
                exprVal = rhsVal;
                break;
            }
            case AST::BinaryOpType::BO_AddAssign:
            {
                exprVal = _builder->CreateAdd(lhsVal, rhsVal, "addassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_SubAssign:
            {
                exprVal = _builder->CreateSub(lhsVal, rhsVal, "subassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_MulAssign:
            {
                exprVal = _builder->CreateMul(lhsVal, rhsVal, "mulassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_DivAssign:
            {
                exprVal = _builder->CreateMul(lhsVal, rhsVal, "mulassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_RemAssign:
            {
                exprVal = _builder->CreateSRem(lhsVal, rhsVal, "remassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_ShlAssign:
            {
                exprVal = _builder->CreateShl(lhsVal, rhsVal, "shlassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_ShrAssign:
            {
                exprVal = _builder->CreateAShr(lhsVal, rhsVal, "shrassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_AndAssign:
            {
                exprVal = _builder->CreateAnd(lhsVal, rhsVal, "andassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_XorAssign:
            {
                exprVal = _builder->CreateXor(lhsVal, rhsVal, "xorassigntmp");
                break;
            }
            case AST::BinaryOpType::BO_OrAssign:
            {
                exprVal = _builder->CreateOr(lhsVal, rhsVal, "orassigntmp");
                break;
            }
            default:
                FATAL_ERROR("Unsupported binary operator type.");
                LLVMIRGEN_RET_FALSE();
            }

            _builder->CreateStore(exprVal, lhsVar);
            LLVMIRGEN_RET_TRUE(exprVal);
        }
        else
        {
            if (!binaryOperator->_lhs->gen(this))
                LLVMIRGEN_RET_FALSE();

            llvm::Value *lhsVal = _retVal;

            if (!binaryOperator->_rhs->gen(this))
                LLVMIRGEN_RET_FALSE();

            llvm::Value *rhsVal = _retVal;

            if (!lhsVal || !rhsVal)
                LLVMIRGEN_RET_FALSE();

            if (lhsVal->getType() != rhsVal->getType())
            {
                if (lhsVal->getType() == llvm::Type::getFloatTy(_context))
                    rhsVal = _builder->CreateFPCast(rhsVal, llvm::Type::getFloatTy(_context), "integralToFloating");
                else if (rhsVal->getType() == llvm::Type::getFloatTy(_context))
                    lhsVal = _builder->CreateFPCast(lhsVal, llvm::Type::getFloatTy(_context), "integralToFloating");
                else
                {
                    if (lhsVal->getType() != llvm::Type::getInt32Ty(_context))
                        lhsVal = _builder->CreateIntCast(lhsVal, llvm::Type::getInt32Ty(_context), true);
                    if (rhsVal->getType() != llvm::Type::getInt32Ty(_context))
                        rhsVal = _builder->CreateIntCast(rhsVal, llvm::Type::getInt32Ty(_context), true);
                }
            }

            llvm::Value *exprVal = nullptr;

            switch (binaryOperator->type())
            {
            case AST::BinaryOpType::BO_Mul:
                exprVal = _builder->CreateMul(lhsVal, rhsVal, "multmp");
                break;
            case AST::BinaryOpType::BO_Div:
                exprVal = _builder->CreateSDiv(lhsVal, rhsVal, "divtmp");
                break;
            case AST::BinaryOpType::BO_Add:
                exprVal = _builder->CreateAdd(lhsVal, rhsVal, "addtmp");
                break;
            case AST::BinaryOpType::BO_Sub:
                exprVal = _builder->CreateSub(lhsVal, rhsVal, "subtmp");
                break;
            case AST::BinaryOpType::BO_Rem:
                exprVal = _builder->CreateSRem(lhsVal, rhsVal, "remtmp");
                break;
            case AST::BinaryOpType::BO_Shl:
                exprVal = _builder->CreateShl(lhsVal, rhsVal, "shltmp");
                break;
            case AST::BinaryOpType::BO_Shr:
                exprVal = _builder->CreateAShr(lhsVal, rhsVal, "shrtmp");
                break;
            case AST::BinaryOpType::BO_LT:
                exprVal = _builder->CreateICmpSLT(lhsVal, rhsVal, "lttmp");
                break;
            case AST::BinaryOpType::BO_GT:
                exprVal = _builder->CreateICmpSGT(lhsVal, rhsVal, "gttmp");
                break;
            case AST::BinaryOpType::BO_LE:
                exprVal = _builder->CreateICmpSLE(lhsVal, rhsVal, "letmp");
                break;
            case AST::BinaryOpType::BO_GE:
                exprVal = _builder->CreateICmpSGE(lhsVal, rhsVal, "getmp");
                break;
            case AST::BinaryOpType::BO_EQ:
                exprVal = _builder->CreateICmpEQ(lhsVal, rhsVal, "eqtmp");
                break;
            case AST::BinaryOpType::BO_NE:
                exprVal = _builder->CreateICmpNE(lhsVal, rhsVal, "netmp");
                break;
            case AST::BinaryOpType::BO_And:
                exprVal = _builder->CreateAnd(lhsVal, rhsVal, "andtmp");
                break;
            case AST::BinaryOpType::BO_Xor:
                exprVal = _builder->CreateXor(lhsVal, rhsVal, "xortmp");
                break;
            case AST::BinaryOpType::BO_Or:
                exprVal = _builder->CreateOr(lhsVal, rhsVal, "ortmp");
                break;
            case AST::BinaryOpType::BO_LAnd:
            {
                llvm::Value *l = _builder->CreateICmpNE(lhsVal, llvm::ConstantInt::get(_context, llvm::APInt(32, 0, true)));
                llvm::Value *r = _builder->CreateICmpNE(rhsVal, llvm::ConstantInt::get(_context, llvm::APInt(32, 0, true)));
                exprVal = _builder->CreateAnd(l, r, "landtmp");
                break;
            }
            case AST::BinaryOpType::BO_LOr:
            {
                llvm::Value *l = _builder->CreateICmpNE(lhsVal, llvm::ConstantInt::get(_context, llvm::APInt(32, 0, true)));
                llvm::Value *r = _builder->CreateICmpNE(rhsVal, llvm::ConstantInt::get(_context, llvm::APInt(32, 0, true)));
                exprVal = _builder->CreateOr(l, r, "lortmp");
                break;
            }
            default:
                break;
            }

            LLVMIRGEN_RET_TRUE(exprVal);
        }
    }

    bool LLVMIRGenerator::gen(AST::UnaryOperator *unaryOperator)
    {
        if (!unaryOperator->_body->gen(this))
            LLVMIRGEN_RET_FALSE();

        llvm::Value *bodyStore = _retVal;

        if (llvm::AllocaInst *alloca = llvm::dyn_cast<llvm::AllocaInst>(bodyStore))
        {
            bodyStore = _builder->CreateLoad(alloca->getAllocatedType(), alloca);
        }

        switch (unaryOperator->type())
        {
        case AST::UnaryOpType::UO_PreInc:
        {
            llvm::Value *addVal = _builder->CreateNSWAdd(bodyStore, llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(_context), llvm::APInt(32, 1, true)), "preInc");
            llvm::Value *addStore = _builder->CreateStore(addVal, bodyStore);
            LLVMIRGEN_RET_TRUE(addStore);
        }
        case AST::UnaryOpType::UO_PreDec:
        {
            llvm::Value *decVal = _builder->CreateNSWSub(bodyStore, llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(_context), llvm::APInt(32, 1, true)), "preDec");
            llvm::Value *decStore = _builder->CreateStore(decVal, bodyStore);
            LLVMIRGEN_RET_TRUE(decStore);
        }
        case AST::UnaryOpType::UO_PostInc:
        {
            llvm::Value *addVal = _builder->CreateNSWAdd(bodyStore, llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(_context), llvm::APInt(32, 1, true)), "postInc");
            llvm::Value *addStore = _builder->CreateStore(addVal, bodyStore);
            LLVMIRGEN_RET_TRUE(addVal);
        }
        case AST::UnaryOpType::UO_PostDec:
        {
            llvm::Value *decVal = _builder->CreateNSWSub(bodyStore, llvm::Constant::getIntegerValue(llvm::Type::getInt32Ty(_context), llvm::APInt(32, 1, true)), "postDec");
            llvm::Value *decStore = _builder->CreateStore(decVal, bodyStore);
            LLVMIRGEN_RET_TRUE(decVal);
        }
        case AST::UnaryOpType::UO_Not:
        {
            llvm::Value *notVal = _builder->CreateNot(bodyStore, "not");
            LLVMIRGEN_RET_TRUE(notVal);
        }
        case AST::UnaryOpType::UO_Minus:
        {
            llvm::Value *minusVal = _builder->CreateNeg(bodyStore, "neg");
            LLVMIRGEN_RET_TRUE(minusVal);
        }
        case AST::UnaryOpType::UO_Plus:
        {
            LLVMIRGEN_RET_TRUE(bodyStore);
        }
        case AST::UnaryOpType::UO_LNot:
        {
            llvm::Value *lnotVal = _builder->CreateICmpEQ(bodyStore, llvm::ConstantInt::get(_context, llvm::APInt(32, 0, true)));
            LLVMIRGEN_RET_TRUE(lnotVal);
        }
        default:
            LLVMIRGEN_RET_TRUE(bodyStore);
        }

        LLVMIRGEN_RET_FALSE();
    }

    bool LLVMIRGenerator::gen(AST::ParenExpr *parenExpr)
    {
        if (!parenExpr->_subExpr->gen(this))
            LLVMIRGEN_RET_FALSE();

        parenExpr->place = parenExpr->_subExpr->place;
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

    bool LLVMIRGenerator::gen(AST::IfStmt *ifStmt)
    {
        if (!ifStmt->_condition->gen(this))
            LLVMIRGEN_RET_FALSE();

        llvm::Type *type = nullptr;
        llvm::Value *condVal = _retVal;

        if (llvm::AllocaInst *alloca = llvm::dyn_cast<llvm::AllocaInst>(condVal))
        {
            type = alloca->getAllocatedType();
            condVal = _builder->CreateLoad(type, alloca, "");
        }
        else
            type = condVal->getType();

        if (type != llvm::Type::getInt1Ty(_context))
            condVal = _builder->CreateICmpNE(condVal, llvm::ConstantInt::get(_context, llvm::APInt(32, 0)), "ifcond");

        llvm::Function *func = _builder->GetInsertBlock()->getParent();

        llvm::BasicBlock *endBB = llvm::BasicBlock::Create(_context, "if.end", func, _builder->GetInsertBlock()->getNextNode());
        llvm::BasicBlock *elseBB = endBB;

        if (ifStmt->_elseBody != nullptr)
            elseBB = llvm::BasicBlock::Create(_context, "if.else", func, endBB);

        llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(_context, "if.body", func, elseBB);

        _builder->CreateCondBr(condVal, bodyBB, elseBB);

        _builder->SetInsertPoint(bodyBB); // gen body ir

        if (!ifStmt->_body->gen(this))
            LLVMIRGEN_RET_FALSE();

        _builder->CreateBr(endBB);

        if (ifStmt->_elseBody != nullptr)
        {
            _builder->SetInsertPoint(elseBB);
            if (!ifStmt->_elseBody->gen(this))
                LLVMIRGEN_RET_FALSE();
            _builder->CreateBr(endBB);
        }

        _builder->SetInsertPoint(endBB);

        LLVMIRGEN_RET_TRUE(_retVal);
    }

    bool LLVMIRGenerator::gen(AST::ValueStmt *valueStmt)
    {
        if (!valueStmt->_expr->gen(this))
            LLVMIRGEN_RET_FALSE();

        LLVMIRGEN_RET_TRUE(_retVal);
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

    bool LLVMIRGenerator::gen(AST::WhileStmt *whileStmt)
    {
        llvm::Function *func = _builder->GetInsertBlock()->getParent();

        llvm::BasicBlock *endBB = llvm::BasicBlock::Create(_context, "while.end", func, _builder->GetInsertBlock()->getNextNode());
        llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(_context, "while.body", func, endBB);
        llvm::BasicBlock *condBB = llvm::BasicBlock::Create(_context, "while.cond", func, bodyBB);

        _builder->CreateBr(condBB);

        _builder->SetInsertPoint(condBB);
        if (!whileStmt->_condition->gen(this))
            LLVMIRGEN_RET_FALSE();

        llvm::Type *type = nullptr;
        llvm::Value *condVal = _retVal;

        if (llvm::AllocaInst *alloca = llvm::dyn_cast<llvm::AllocaInst>(condVal))
        {
            type = alloca->getAllocatedType();
            condVal = _builder->CreateLoad(type, alloca, "");
        }
        else
            type = condVal->getType();

        if (type != llvm::Type::getInt1Ty(_context))
            condVal = _builder->CreateICmpNE(condVal, llvm::ConstantInt::get(_context, llvm::APInt(32, 0)), "whilecond");

        _builder->CreateCondBr(condVal, bodyBB, endBB);

        _builder->SetInsertPoint(bodyBB);
        if (!whileStmt->_body->gen(this))
            LLVMIRGEN_RET_FALSE();

        _builder->CreateBr(condBB);

        _builder->SetInsertPoint(endBB);

        LLVMIRGEN_RET_TRUE(_retVal);
    }

    bool LLVMIRGenerator::gen(AST::CallExpr *callExpr)
    {
        std::string funcName = callExpr->_functionExpr->name();
        llvm::Function *func = _module->getFunction(funcName);

        if (func == nullptr)
        {
            FATAL_ERROR("Referencing undefined function " << funcName);
            LLVMIRGEN_RET_FALSE();
        }

        if (callExpr->_params.size() != func->arg_size())
        {
            FATAL_ERROR("Function " << funcName << " required for " << std::to_string(func->arg_size()) << " arguments, but " << std::to_string(callExpr->_params.size()) << " were given");
            LLVMIRGEN_RET_FALSE();
        }

        std::vector<llvm::Value *> argVals;

        for (auto &param : callExpr->_params)
        {
            if (!param->gen(this))
                LLVMIRGEN_RET_FALSE();
            argVals.push_back(_retVal);
        }

        llvm::Value *funcCall = _builder->CreateCall(func, argVals, "calltmp");

        LLVMIRGEN_RET_TRUE(funcCall);
    }

    // Only AT&T style inline asm is supported
    std::string LLVMIRGenerator::generateAsmString(AST::AsmStmt *asmStmt)
    {
        // Analyze the asm string to decompose it into its pieces.  We know that Sema
        // has already done this, so it is guaranteed to be successful.
        llvm::SmallVector<AST::AsmStmt::AsmStringPiece, 4> Pieces;
        analyzeAsmString(asmStmt, Pieces);

        std::string AsmString;
        for (const auto &Piece : Pieces)
        {
            if (Piece.isString())
                AsmString += Piece.getString();
            else if (Piece.getModifier() == '\0')
                AsmString += '$' + llvm::utostr(Piece.getOperandNo());
            else
                AsmString += "${" + llvm::utostr(Piece.getOperandNo()) + ':' +
                             Piece.getModifier() + '}';
        }
        return AsmString;
    }

    bool LLVMIRGenerator::analyzeAsmString(AST::AsmStmt *asmStmt, llvm::SmallVectorImpl<AST::AsmStmt::AsmStringPiece> &Pieces)
    {
        llvm::StringRef Str = asmStmt->_asmString;
        const char *StrStart = Str.begin();
        const char *StrEnd = Str.end();
        const char *CurPtr = StrStart;

        // "Simple" inline asms have no constraints or operands, just convert the asm
        // string to escape $'s.
        if (!asmStmt->isExtendedAsm())
        {
            std::string Result;
            for (; CurPtr != StrEnd; ++CurPtr)
            {
                switch (*CurPtr)
                {
                case '$':
                    Result += "$$";
                    break;
                default:
                    Result += *CurPtr;
                    break;
                }
            }
            Pieces.push_back(AST::AsmStmt::AsmStringPiece(Result));
            return 0;
        }

        // CurStringPiece - The current string that we are building up as we scan the
        // asm string.
        std::string CurStringPiece;

        // Fixme: wtf does hasVariants do idk, leave it = true for now
        bool HasVariants = true;

        unsigned LastAsmStringToken = 0;
        unsigned LastAsmStringOffset = 0;

        while (true)
        {
            // Done with the string?
            if (CurPtr == StrEnd)
            {
                if (!CurStringPiece.empty())
                    Pieces.push_back(AST::AsmStmt::AsmStringPiece(CurStringPiece));
                return 0;
            }

            char CurChar = *CurPtr++;
            switch (CurChar)
            {
            case '$':
                CurStringPiece += "$$";
                continue;
            case '{':
                CurStringPiece += (HasVariants ? "$(" : "{");
                continue;
            case '|':
                CurStringPiece += (HasVariants ? "$|" : "|");
                continue;
            case '}':
                CurStringPiece += (HasVariants ? "$)" : "}");
                continue;
            case '%':
                break;
            default:
                CurStringPiece += CurChar;
                continue;
            }

            char EscapedChar = *CurPtr++;
            switch (EscapedChar)
            {
            default: // the letter might be a digit or letter, will be parsed later...
                break;
            case '%': // %% -> %
            case '{': // %{ -> {
            case '}': // %} -> }
                CurStringPiece += EscapedChar;
                continue;
            case '=': // %= -> Generate a unique ID.
                CurStringPiece += "${:uid}";
                continue;
            }

            // Otherwise, we have an operand.  If we have accumulated a string so far,
            // add it to the Pieces list.
            if (!CurStringPiece.empty())
            {
                Pieces.push_back(AST::AsmStmt::AsmStringPiece(CurStringPiece));
                CurStringPiece.clear();
            }

            const char *Begin = CurPtr - 1;  // Points to the character following '%'.
            const char *Percent = Begin - 1; // Points to '%'.

            if (isLetter(EscapedChar))
            {
                if (CurPtr == StrEnd) // Premature end.
                {
                    return false;
                }
                EscapedChar = *CurPtr++;
            }

            if (isDigit(EscapedChar))
            {
                // %n - Assembler operand n
                unsigned N = 0;

                --CurPtr;
                while (CurPtr != StrEnd && isDigit(*CurPtr))
                    N = N * 10 + ((*CurPtr++) - '0');

                // Str contains "x4" (Operand without the leading %).
                std::string Str(Begin, CurPtr - Begin);
            }
        }
        return true;
    }

    bool LLVMIRGenerator::gen(AST::AsmStmt *asmStmt)
    {
        std::string asmString = generateAsmString(asmStmt);
        FATAL_ERROR(asmString);
        LLVMIRGEN_RET_TRUE(_retVal);
    }
}