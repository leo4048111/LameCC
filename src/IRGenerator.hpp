#pragma once

#include <string>
#include <memory>
#include <vector>

#include "AST.hpp"

namespace lcc
{
    // Intermediate representation generator class(IRGenerator.cpp)
    class IRGenerator
    {
        typedef struct _SymbolTableItem
        {
            std::string name;
            std::string type;
            int offset;
            _SymbolTableItem(std::string name, std::string type, int offset) : name(name), type(type), offset(offset){};
        } SymbolTableItem;

        typedef struct _SymbolTable
        {
            _SymbolTable(std::shared_ptr<_SymbolTable> previous) : previous(previous){};
            std::shared_ptr<_SymbolTable> previous;
            std::vector<std::shared_ptr<SymbolTableItem>> items;
            int totalWidth{0};
        } SymbolTable;

        enum class ArgType
        {
            NIL = 0,
            CODEADDR,
            ENTRY,
            VALUE
        };

        class Arg
        {
        public:
            virtual ArgType type() { return ArgType::NIL; };
        };

        class CodeAddr : public Arg
        {
        public:
            virtual ArgType type() override { return ArgType::CODEADDR; };

            int codeAddr;

            CodeAddr(int codeAddr) : codeAddr(codeAddr){};
        };

        class SymbTblEntry : public Arg
        {
        public:
            virtual ArgType type() override { return ArgType::ENTRY; };

            std::shared_ptr<SymbolTableItem> pEntry;

            SymbTblEntry(std::shared_ptr<SymbolTableItem> pEntry) : pEntry(pEntry){};
        };

        class Value : public Arg
        {
        public:
            enum class ValueType
            {
                INTEGER = 0,
                FLOAT
            };

        public:
            virtual ArgType type() override { return ArgType::VALUE; };

            union
            {
                float floatVal;
                int integerVal;
            };

            ValueType valueType;
            Value(float floatVal) : floatVal(floatVal), valueType(ValueType::FLOAT){};
            Value(int integerVal) : integerVal(integerVal), valueType(ValueType::INTEGER){};
        };

        enum class QuaternionOperator
        {
            Invalid = 0,
            DefineEqual, // :=
#define BINARY_OPERATION(name, disc) name,
#define UNARY_OPERATION(name, disc) name,
#include "OperationType.inc"
#undef UNARY_OPERATION
#undef BINARY_OPERATION
            Jnz,  // conditional jump
            J,    // jump
            Call, // TODO function call
            Ret,  // function returns
        };

        typedef struct
        {
            QuaternionOperator op;
            std::shared_ptr<Arg> arg1;
            std::shared_ptr<Arg> arg2;
            std::shared_ptr<Arg> result;
        } Quaternion;

        typedef struct
        {
            std::string name;
            std::string type;
            int entry;
            bool isInitialized;
        } FunctionTableItem;

    private:
        IRGenerator();
        IRGenerator(const IRGenerator &) = delete;
        IRGenerator &operator=(const IRGenerator &) = delete;

    public:
        static IRGenerator *getInstance()
        {
            if (_inst.get() == nullptr)
                _inst.reset(new IRGenerator);

            return _inst.get();
        }

    private:
        static std::unique_ptr<IRGenerator> _inst;

    public:
        // gen methods
        bool gen(AST::TranslationUnitDecl *translationUnitDecl);
        bool gen(AST::FunctionDecl *functionDecl);
        bool gen(AST::VarDecl *varDecl);
        bool gen(AST::IntegerLiteral *integerLiteral);
        bool gen(AST::FloatingLiteral *floatingLiteral);
        bool gen(AST::DeclRefExpr *declRefExpr);
        bool gen(AST::CastExpr *castExpr);
        bool gen(AST::BinaryOperator *binaryOperator);
        bool gen(AST::UnaryOperator *unaryOperator);
        bool gen(AST::ParenExpr *parenExpr);
        bool gen(AST::CompoundStmt *compoundStmt);
        bool gen(AST::DeclStmt *declStmt);
        bool gen(AST::IfStmt *ifStmt);
        bool gen(AST::ValueStmt *valueStmt);
        bool gen(AST::ReturnStmt *returnStmt);
        bool gen(AST::WhileStmt *whileStmt);
        bool gen(AST::CallExpr* callExpr);

    private:
        std::shared_ptr<SymbolTable> mkTable(std::shared_ptr<SymbolTable> previous);
        void changeTable(std::shared_ptr<SymbolTable> table);
        bool enter(std::string name, std::string type, int width);
        bool registerFunc(std::string name, std::string type, int entry, bool isInitialized);
        std::shared_ptr<SymbolTableItem> lookup(std::string name);
        std::shared_ptr<SymbolTableItem> lookupCurrentTbl(std::string name);
        void emit(QuaternionOperator op, std::shared_ptr<Arg> arg1, std::shared_ptr<Arg> arg2, std::shared_ptr<Arg> result);
        std::shared_ptr<SymbolTableItem> newtemp(std::string type, int width);

        static QuaternionOperator BinaryOpToQuaternionOp(AST::BinaryOpType op);
        static QuaternionOperator UnaryOpToQuaternionOp(AST::UnaryOpType op);

    public:
        void printCode() const;
        void dumpCode(const std::string outPath) const;

    private:
        std::vector<std::shared_ptr<SymbolTable>> _tables;
        std::shared_ptr<SymbolTable> _currentSymbolTable;
        std::vector<FunctionTableItem> _functionTable;
        std::vector<Quaternion> _codes;
    };
}