#include "lcc.hpp"

#define EMIT(op, arg1, arg2, result) emit(op, arg1, arg2, result)
#define INVALID_SYMBOLTBL_ENTRY(entry) (entry == nullptr)
#define INT32_WIDTH sizeof(uint32_t)
#define FLOAT_WIDTH sizeof(float)
#define INT "int"
#define FLOAT "float"
//#define VOID "void"
#define MAKE_NIL_ARG() std::make_shared<Arg>()
#define MAKE_VALUE_ARG(val) std::make_shared<Value>(val)
#define MAKE_ENTRY_ARG(entry) std::make_shared<SymbTblEntry>(entry)
#define MAKE_ADDR_ARG(codeAddr) std::make_shared<CodeAddr>(codeAddr)

#define DEBUG_

namespace lcc
{
    std::unique_ptr<IRGenerator> IRGenerator::_inst;

    IRGenerator::QuaternionOperator IRGenerator::BinaryOpToQuaternionOp(AST::BinaryOpType op)
    {
        switch (op)
        {
#define BINARY_OPERATION(name, disc)   \
    case AST::BinaryOpType::BO_##name: \
        return QuaternionOperator::name;
#define UNARY_OPERATION(name, disc)
#include "OperationType.inc"
#undef UNARY_OPERATION
#undef BINARY_OPERATION
        default:
            return IRGenerator::QuaternionOperator::Invalid;
            break;
        }
    }

    IRGenerator::QuaternionOperator IRGenerator::UnaryOpToQuaternionOp(AST::UnaryOpType op)
    {
        switch (op)
        {
#define BINARY_OPERATION(name, disc)
#define UNARY_OPERATION(name, disc)   \
    case AST::UnaryOpType::UO_##name: \
        return QuaternionOperator::name;
#include "OperationType.inc"
#undef UNARY_OPERATION
#undef BINARY_OPERATION
        default:
            return IRGenerator::QuaternionOperator::Invalid;
            break;
        }
    }

    IRGenerator::IRGenerator()
    {
        changeTable(mkTable(nullptr));
    }

    bool IRGenerator::gen(AST::TranslationUnitDecl *translationUnitDecl)
    {
        for (auto &decl : translationUnitDecl->_decls)
            if (!decl->gen())
                return false;

        return true;
    }

    bool IRGenerator::gen(AST::VarDecl *varDecl)
    {
        bool result = enter(varDecl->name(), varDecl->type(), 4);
        if (!result)
        {
            FATAL_ERROR("Redeclaration " << varDecl->type() << " " << varDecl->name());
            return false;
        }

        if (varDecl->_isInitialized)
        {
            if (!varDecl->_value->gen())
                return false;
            auto arg1Entry = lookup(varDecl->_value->place);
            auto resultEntry = lookup(varDecl->name());
            EMIT(QuaternionOperator::Assign, MAKE_ENTRY_ARG(arg1Entry), MAKE_NIL_ARG(), MAKE_ENTRY_ARG(resultEntry));
        }

        return true;
    }

    bool IRGenerator::gen(AST::FunctionDecl *functionDecl)
    {
        if (!registerFunc(functionDecl->name(), _codes.size()))
        {
            FATAL_ERROR("Function " << functionDecl->name() << " redeclaration.");
            return false;
        }

        auto previousTable = _currentSymbolTable;
        changeTable(mkTable(previousTable));

        for (auto &param : functionDecl->_params)
        {
            if (!param->gen())
                return false;
        }

        if (!functionDecl->_body->gen())
            return false;

        changeTable(previousTable);
        return true;
    }

    bool IRGenerator::gen(AST::IntegerLiteral *integerLiteral)
    {
        auto newTmpEntry = newtemp(INT, INT32_WIDTH);
        if (INVALID_SYMBOLTBL_ENTRY(newTmpEntry))
            return false;
        integerLiteral->place = newTmpEntry->name;

        EMIT(QuaternionOperator::Assign, MAKE_VALUE_ARG(integerLiteral->value()), MAKE_NIL_ARG(), MAKE_ENTRY_ARG(newTmpEntry));
        return true;
    }

    bool IRGenerator::gen(AST::FloatingLiteral *floatingLiteral)
    {
        auto newTmpEntry = newtemp(FLOAT, FLOAT_WIDTH);
        if (INVALID_SYMBOLTBL_ENTRY(newTmpEntry))
            return false;
        floatingLiteral->place = newTmpEntry->name;

        EMIT(QuaternionOperator::Assign, MAKE_VALUE_ARG(floatingLiteral->value()), MAKE_NIL_ARG(), MAKE_ENTRY_ARG(newTmpEntry));
        return true;
    }

    bool IRGenerator::gen(AST::DeclRefExpr *declRefExpr)
    {
        if (!declRefExpr->_isCall) // referencing a param
        {
            auto tblEntry = lookup(declRefExpr->name());
            if (INVALID_SYMBOLTBL_ENTRY(tblEntry)) // symbol not declared
            {
                FATAL_ERROR("Symbol " << declRefExpr->name() << " not declared.");
                return false;
            }

            declRefExpr->place = tblEntry->name;
            return true;
        }

        // TODO function call
        return true;
    }

    bool IRGenerator::gen(AST::CastExpr *castExpr)
    {
        // CURRENTLY TYPE CAST DOES NOTHING AT ALL!
        if (!castExpr->_subExpr->gen())
            return false;

        castExpr->place = castExpr->_subExpr->place;
        return true;
    }

    bool IRGenerator::gen(AST::BinaryOperator *binaryOperator)
    {
        if (!binaryOperator->_lhs->gen())
            return false;
        if (!binaryOperator->_rhs->gen())
            return false;

        auto arg1Entry = lookup(binaryOperator->_lhs->place);
        auto arg2Entry = lookup(binaryOperator->_rhs->place);

        std::string resultType;
        int resultWidth;
        if (arg1Entry->type == FLOAT || arg2Entry->type == FLOAT)
        {
            resultType = FLOAT;
            resultWidth = FLOAT_WIDTH;
        }
        else
        {
            resultType = INT;
            resultWidth = INT32_WIDTH;
        }

        auto newTmpEntry = newtemp(resultType, resultWidth);
        if (INVALID_SYMBOLTBL_ENTRY(newTmpEntry))
            return false;

        binaryOperator->place = newTmpEntry->name;

        auto resultEntry = lookup(newTmpEntry->name);

        EMIT(BinaryOpToQuaternionOp(binaryOperator->type()), MAKE_ENTRY_ARG(arg1Entry), MAKE_ENTRY_ARG(arg2Entry), MAKE_ENTRY_ARG(resultEntry));
        return true;
    }

    bool IRGenerator::gen(AST::ParenExpr *parenExpr)
    {
        if (!parenExpr->_subExpr->gen())
            return false;

        parenExpr->place = parenExpr->_subExpr->place;
        return true;
    }

    bool IRGenerator::gen(AST::CompoundStmt *compoundStmt)
    {
        auto previousTable = _currentSymbolTable;
        changeTable(mkTable(previousTable));
        for (auto &stmt : compoundStmt->_body)
        {
            if (!stmt->gen())
                return false;
        }

        changeTable(previousTable);
        return true;
    }

    bool IRGenerator::gen(AST::DeclStmt *declStmt)
    {
        for (auto &decl : declStmt->_decls)
        {
            if (!decl->gen())
                return false;
        }

        return true;
    }

    bool IRGenerator::gen(AST::IfStmt *ifStmt)
    {
        // currently all conditional jumps are implemented with jnz
        if (!ifStmt->_condition->gen())
            return false; // gen ir for condition first
        int bodyCodeEntryAddr = _codes.size() + 2;
        int elseBodyEntryAddr = 0; // this will be filled in after if body is generated
        auto conditionExprResultEntry = lookup(ifStmt->_condition->place);
        EMIT(QuaternionOperator::Jnz, MAKE_ENTRY_ARG(conditionExprResultEntry), MAKE_NIL_ARG(), MAKE_ADDR_ARG(bodyCodeEntryAddr)); // if condition is true, jump to if body
        EMIT(QuaternionOperator::J, MAKE_NIL_ARG(), MAKE_NIL_ARG(), MAKE_ADDR_ARG(elseBodyEntryAddr));

        auto &jumpToElseBodyCode = _codes.back();

        if (!ifStmt->_body->gen())
            return false;
        elseBodyEntryAddr = _codes.size();

        jumpToElseBodyCode.result = MAKE_ADDR_ARG(elseBodyEntryAddr); // replace 0 with correct else body addr

        if (ifStmt->_elseBody != nullptr) // gen else body if exists
            if (!ifStmt->_elseBody->gen())
                return false;

        return true;
    }

    bool IRGenerator::gen(AST::ValueStmt *valueStmt)
    {
        if (!valueStmt->_expr->gen())
            return false;

        return true;
    }

    bool IRGenerator::gen(AST::ReturnStmt *returnStmt)
    {
        // TODO return type check!
        if (returnStmt->_value == nullptr)
            EMIT(QuaternionOperator::Ret, MAKE_NIL_ARG(), MAKE_NIL_ARG(), MAKE_NIL_ARG());
        else
        {
            if (!returnStmt->_value->gen())
                return false;

            auto returnValueEntry = lookup(returnStmt->_value->place);
            EMIT(QuaternionOperator::Ret, MAKE_NIL_ARG(), MAKE_NIL_ARG(), MAKE_ENTRY_ARG(returnValueEntry));
        }

        return true;
    }

    bool IRGenerator::gen(AST::UnaryOperator *unaryOperator)
    {
        if (!unaryOperator->_body->gen())
            return false;

        auto bodyResultEntry = lookup(unaryOperator->_body->place);

        std::shared_ptr<SymbolTableItem> newTempResult = nullptr;

        if(bodyResultEntry->type == INT)
            newTempResult = newtemp(bodyResultEntry->type, INT32_WIDTH);
        else 
            newTempResult = newtemp(bodyResultEntry->type, FLOAT_WIDTH);

        
        EMIT(UnaryOpToQuaternionOp(unaryOperator->type()), MAKE_ENTRY_ARG(bodyResultEntry), MAKE_NIL_ARG(), MAKE_ENTRY_ARG(newTempResult));

        unaryOperator->place = newTempResult->name;

        return true;
    }

    bool IRGenerator::gen(AST::WhileStmt *whileStmt)
    {
        int whileConditionEntryAddr = _codes.size();
        if (!whileStmt->_condition->gen())
            return false;

        auto conditionExprResultEntry = lookup(whileStmt->_condition->place);
        int whileBodyEntryAddr = _codes.size() + 2;
        int whileExitAddr = 0; // this will be filled in after body codes are emitted
        EMIT(QuaternionOperator::Jnz, MAKE_ENTRY_ARG(conditionExprResultEntry), MAKE_NIL_ARG(), MAKE_ADDR_ARG(whileBodyEntryAddr));
        EMIT(QuaternionOperator::J, MAKE_NIL_ARG(), MAKE_NIL_ARG(), MAKE_ADDR_ARG(whileExitAddr));

        auto &jumpToWhileExitCode = _codes.back();

        if (!whileStmt->_body->gen())
            return false;

        EMIT(QuaternionOperator::J, MAKE_NIL_ARG(), MAKE_NIL_ARG(), MAKE_ADDR_ARG(whileConditionEntryAddr)); // go back to while condition entry to calculate loop condition again
        whileExitAddr = _codes.size();

        jumpToWhileExitCode.result = MAKE_ADDR_ARG(whileExitAddr); // replace 0 with while exit addr

        return true;
    }

    std::shared_ptr<IRGenerator::SymbolTable> IRGenerator::mkTable(std::shared_ptr<SymbolTable> previous)
    {
        auto tbl = std::make_shared<SymbolTable>(previous);
        _tables.push_back(tbl);
        return tbl;
    }

    void IRGenerator::changeTable(std::shared_ptr<SymbolTable> table)
    {
        _currentSymbolTable = table;
    }

    std::shared_ptr<IRGenerator::SymbolTableItem> IRGenerator::lookupCurrentTbl(std::string name)
    {
        for (auto &item : _currentSymbolTable->items)
        {
            if (item->name == name)
                return item;
        }

        return nullptr;
    }

    std::shared_ptr<IRGenerator::SymbolTableItem> IRGenerator::lookup(std::string name)
    {
        auto currentTbl = _currentSymbolTable;
        std::shared_ptr<SymbolTableItem> tblEntry = nullptr;
        for (_currentSymbolTable; _currentSymbolTable != nullptr; changeTable(_currentSymbolTable->previous))
        {
            tblEntry = lookupCurrentTbl(name);
            if (!(INVALID_SYMBOLTBL_ENTRY(tblEntry)))
                break;
        }
        changeTable(currentTbl);

        return tblEntry;
    }

    bool IRGenerator::enter(std::string name, std::string type, int width)
    {
        auto entry = lookupCurrentTbl(name);
        if (!INVALID_SYMBOLTBL_ENTRY(entry)) // duplication check
            return false;

        _currentSymbolTable->items.push_back(std::make_shared<SymbolTableItem>(name, type, _currentSymbolTable->totalWidth));
        _currentSymbolTable->totalWidth += width;

        return true;
    }

    void IRGenerator::emit(QuaternionOperator op, std::shared_ptr<Arg> arg1, std::shared_ptr<Arg> arg2, std::shared_ptr<Arg> result)
    {
        Quaternion code = {op, arg1, arg2, result};
        _codes.push_back(code);
    }

    std::shared_ptr<IRGenerator::SymbolTableItem> IRGenerator::newtemp(std::string type, int width)
    {
        static int id = 0;
        std::string name = "@T" + std::to_string(id);
        id++;

        if (!enter(name, type, width))
        {
            FATAL_ERROR("Internal error.");
            return nullptr;
        }

        return lookup(name);
    }

    bool IRGenerator::registerFunc(std::string name, int entry)
    {
        for (auto &item : _functionTable)
        {
            if (item.name == name)
                return false;
        }

        _functionTable.push_back({name, entry});
        return true;
    }

    void IRGenerator::printCode() const
    {
        int id = 0;
        std::string op;
        std::string arg1;
        std::string arg2;
        std::string result;
        for (auto &code : _codes)
        {
            for (auto &item : _functionTable)
            {
                if (id == item.entry)
                    printf("%s:\n", item.name.c_str());
            }

            switch (code.op)
            {
#define BINARY_OPERATION(name, disc) \
    case QuaternionOperator::name:   \
        op = disc;                   \
        break;
#define UNARY_OPERATION(name, disc) BINARY_OPERATION(name, disc)
#include "OperationType.inc"
#undef UNARY_OPERATION
#undef BINARY_OPERATION
            case QuaternionOperator::Jnz:
                op = "Jnz";
                break;
            case QuaternionOperator::J:
                op = "J";
                break;
            case QuaternionOperator::Call:
                op = "Call";
                break;
            case QuaternionOperator::Ret:
                op = "Return";
                break;
            default:
                op = "_";
                break;
            }

            switch (code.arg1->type())
            {
            case ArgType::NIL:
                arg1 = "_";
                break;
            case ArgType::ENTRY:
            {
                auto pArg1 = std::dynamic_pointer_cast<SymbTblEntry>(code.arg1);
                arg1 = pArg1->pEntry->name;
                break;
            }
            case ArgType::VALUE:
            {
                auto pArg1 = std::dynamic_pointer_cast<Value>(code.arg1);
                if (pArg1->valueType == Value::ValueType::INTEGER)
                    arg1 = std::to_string(pArg1->integerVal);
                else
                    arg1 = std::to_string(pArg1->floatVal);
                break;
            }
            case ArgType::CODEADDR:
            {
                auto pArg1 = std::dynamic_pointer_cast<CodeAddr>(code.arg1);
                arg1 = std::to_string(pArg1->codeAddr);
                break;
            }
            }

            switch (code.arg2->type())
            {
            case ArgType::NIL:
                arg2 = "_";
                break;
            case ArgType::ENTRY:
            {
                auto pArg2 = std::dynamic_pointer_cast<SymbTblEntry>(code.arg2);
                arg2 = pArg2->pEntry->name;
                break;
            }
            case ArgType::VALUE:
            {
                auto pArg2 = std::dynamic_pointer_cast<Value>(code.arg2);
                if (pArg2->valueType == Value::ValueType::INTEGER)
                    arg2 = std::to_string(pArg2->integerVal);
                else
                    arg2 = std::to_string(pArg2->floatVal);
                break;
            }
            case ArgType::CODEADDR:
            {
                auto pArg2 = std::dynamic_pointer_cast<CodeAddr>(code.arg2);
                arg2 = std::to_string(pArg2->codeAddr);
                break;
            }
            }

            switch (code.result->type())
            {
            case ArgType::NIL:
                result = "_";
                break;
            case ArgType::ENTRY:
            {
                auto pResult = std::dynamic_pointer_cast<SymbTblEntry>(code.result);
                result = pResult->pEntry->name;
                break;
            }
            case ArgType::VALUE:
            {
                auto pResult = std::dynamic_pointer_cast<Value>(code.result);
                if (pResult->valueType == Value::ValueType::INTEGER)
                    result = std::to_string(pResult->integerVal);
                else
                    result = std::to_string(pResult->floatVal);
                break;
            }
            case ArgType::CODEADDR:
            {
                auto pResult = std::dynamic_pointer_cast<CodeAddr>(code.result);
                result = std::to_string(pResult->codeAddr);
                break;
            }
            }

            printf("%4d: (%-10s, %-10s, %-10s, %-10s)\n", id, op.c_str(), arg1.c_str(), arg2.c_str(), result.c_str());
            id++;
        }
    }
}