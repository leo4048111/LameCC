#include "lcc.hpp"

#define EMIT(op, arg1, arg2, result) emit(IRGenerator::QuaternionOperator::op, arg1, arg2, result)
#define INVALID_SYMBOLTBL_ENTRY(entry) (entry == _currentTable->items.end())
#define INT32_WIDTH sizeof(uint32_t)
#define INT "int"
#define MAKE_NIL_ARG() std::make_shared<Arg>()
#define MAKE_VALUE_ARG(val) std::make_shared<Value>(val)
#define MAKE_ENTRY_ARG(entry) std::make_shared<SymbTblEntry>(entry)

#define DEBUG_

namespace lcc
{
    std::unique_ptr<IRGenerator> IRGenerator::_inst;

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
            EMIT(ASSIGN, MAKE_ENTRY_ARG(arg1Entry), MAKE_NIL_ARG(), MAKE_ENTRY_ARG(resultEntry));
        }

        return true;
    }

    bool IRGenerator::gen(AST::IntegerLiteral *integerLiteral)
    {
        auto newTmpEntry = newtemp("int", INT32_WIDTH);
        if (INVALID_SYMBOLTBL_ENTRY(newTmpEntry))
            return false;
        integerLiteral->place = newTmpEntry->name;

        EMIT(ASSIGN, MAKE_VALUE_ARG(integerLiteral->value()), MAKE_NIL_ARG(), MAKE_ENTRY_ARG(newTmpEntry));
        return true;
    }

    std::shared_ptr<IRGenerator::SymbolTable> IRGenerator::mkTable(std::shared_ptr<SymbolTable> previous)
    {
        return std::make_shared<SymbolTable>(previous);
    }

    void IRGenerator::changeTable(std::shared_ptr<SymbolTable> table)
    {
        _currentTable = table;
    }

    std::vector<IRGenerator::SymbolTableItem>::iterator IRGenerator::lookup(std::string name)
    {
        for (auto it = _currentTable->items.begin(); it != _currentTable->items.end(); it++)
        {
            if ((*it).name == name)
                return it;
        }

        return _currentTable->items.end();
    }

    bool IRGenerator::enter(std::string name, std::string type, int width)
    {
        auto entry = lookup(name);
        if (!INVALID_SYMBOLTBL_ENTRY(entry)) // duplication check
            return false;

        _currentTable->items.push_back({name, type, _currentTable->totalWidth});
        _currentTable->totalWidth += width;

        return true;
    }

    void IRGenerator::emit(QuaternionOperator op, std::shared_ptr<Arg> arg1, std::shared_ptr<Arg> arg2, std::shared_ptr<Arg> result)
    {
        Quaternion code = {op, arg1, arg2, result};
        _codes.push_back(code);
    }

    std::vector<IRGenerator::SymbolTableItem>::iterator IRGenerator::newtemp(std::string type, int width)
    {
        static int id = 0;
        std::string name = "@T" + std::to_string(id);
        id++;

        if (!enter(name, type, width))
        {
            FATAL_ERROR("Internal error.");
            return _currentTable->items.end();
        }

        return lookup(name);
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
            switch (code.op)
            {
            case QuaternionOperator::ASSIGN:
                op = ":=";
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
                arg1 = std::to_string(pArg1->integerVal);
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
                arg2 = std::to_string(pArg2->integerVal);
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
                arg1 = "_";
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
                result = std::to_string(pResult->integerVal);
                break;
            }
            case ArgType::CODEADDR:
            {
                auto pResult = std::dynamic_pointer_cast<CodeAddr>(code.result);
                result = std::to_string(pResult->codeAddr);
                break;
            }
            }

            printf("%4d: (%-20s, %-20s, %-20s, %-20s)\n", id, op.c_str(), arg1.c_str(), arg2.c_str(), result.c_str());
            id++;
        }
    }
}