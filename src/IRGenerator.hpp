#pragma once

#include <string>
#include <memory>
#include <vector>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST.hpp"

namespace lcc
{
    // Base class for IRGenerators
    class IRGeneratorBase
    {
    public:
        virtual bool gen(AST::TranslationUnitDecl *translationUnitDecl) = 0;
        virtual bool gen(AST::FunctionDecl *functionDecl) = 0;
        virtual bool gen(AST::VarDecl *varDecl) = 0;
        virtual bool gen(AST::IntegerLiteral *integerLiteral) = 0;
        virtual bool gen(AST::FloatingLiteral *floatingLiteral) = 0;
        virtual bool gen(AST::DeclRefExpr *declRefExpr) = 0;
        virtual bool gen(AST::CastExpr *castExpr) = 0;
        virtual bool gen(AST::BinaryOperator *binaryOperator) = 0;
        virtual bool gen(AST::UnaryOperator *unaryOperator) = 0;
        virtual bool gen(AST::ParenExpr *parenExpr) = 0;
        virtual bool gen(AST::CompoundStmt *compoundStmt) = 0;
        virtual bool gen(AST::DeclStmt *declStmt) = 0;
        virtual bool gen(AST::IfStmt *ifStmt) = 0;
        virtual bool gen(AST::ValueStmt *valueStmt) = 0;
        virtual bool gen(AST::ReturnStmt *returnStmt) = 0;
        virtual bool gen(AST::WhileStmt *whileStmt) = 0;
        virtual bool gen(AST::CallExpr *callExpr) = 0;

    public:
        virtual void printCode() const = 0;
        virtual void dumpCode(const std::string outPath) const = 0;
    };

    // Quaternion intermediate representation generator class(QuaternionIRGenerator.cpp)
    class QuaternionIRGenerator : public IRGeneratorBase
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
        QuaternionIRGenerator();
        QuaternionIRGenerator(const QuaternionIRGenerator &) = delete;
        QuaternionIRGenerator &operator=(const QuaternionIRGenerator &) = delete;

    public:
        static QuaternionIRGenerator *getInstance()
        {
            if (_inst.get() == nullptr)
                _inst.reset(new QuaternionIRGenerator);

            return _inst.get();
        }

    private:
        static std::unique_ptr<QuaternionIRGenerator> _inst;

    public:
        // gen methods implementations for QuaternionIRGenerator
        virtual bool gen(AST::TranslationUnitDecl *translationUnitDecl) override;
        virtual bool gen(AST::FunctionDecl *functionDecl) override;
        virtual bool gen(AST::VarDecl *varDecl) override;
        virtual bool gen(AST::IntegerLiteral *integerLiteral) override;
        virtual bool gen(AST::FloatingLiteral *floatingLiteral) override;
        virtual bool gen(AST::DeclRefExpr *declRefExpr) override;
        virtual bool gen(AST::CastExpr *castExpr) override;
        virtual bool gen(AST::BinaryOperator *binaryOperator) override;
        virtual bool gen(AST::UnaryOperator *unaryOperator) override;
        virtual bool gen(AST::ParenExpr *parenExpr) override;
        virtual bool gen(AST::CompoundStmt *compoundStmt) override;
        virtual bool gen(AST::DeclStmt *declStmt) override;
        virtual bool gen(AST::IfStmt *ifStmt) override;
        virtual bool gen(AST::ValueStmt *valueStmt) override;
        virtual bool gen(AST::ReturnStmt *returnStmt) override;
        virtual bool gen(AST::WhileStmt *whileStmt) override;
        virtual bool gen(AST::CallExpr *callExpr) override;

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
        virtual void printCode() const override;
        virtual void dumpCode(const std::string outPath) const override;

    private:
        std::vector<std::shared_ptr<SymbolTable>> _tables;
        std::shared_ptr<SymbolTable> _currentSymbolTable;
        std::vector<FunctionTableItem> _functionTable;
        std::vector<Quaternion> _codes;
    };

    class LLVMIRGenerator : public IRGeneratorBase
    {
    private:
        LLVMIRGenerator();
        LLVMIRGenerator(const LLVMIRGenerator &) = delete;
        LLVMIRGenerator &operator=(const LLVMIRGenerator &) = delete;

    public:
        static LLVMIRGenerator *getInstance()
        {
            if (_inst.get() == nullptr)
                _inst.reset(new LLVMIRGenerator);

            return _inst.get();
        }

    private:
        static std::unique_ptr<LLVMIRGenerator> _inst;

    public:
        virtual bool gen(AST::TranslationUnitDecl *translationUnitDecl) override;
        virtual bool gen(AST::FunctionDecl *functionDecl) override;
        virtual bool gen(AST::VarDecl *varDecl) override;
        virtual bool gen(AST::IntegerLiteral *integerLiteral) override;
        virtual bool gen(AST::FloatingLiteral *floatingLiteral) override;
        virtual bool gen(AST::DeclRefExpr *declRefExpr) override;
        virtual bool gen(AST::CastExpr *castExpr) override;
        virtual bool gen(AST::BinaryOperator *binaryOperator) override;
        virtual bool gen(AST::UnaryOperator *unaryOperator) override;
        virtual bool gen(AST::ParenExpr *parenExpr) override;
        virtual bool gen(AST::CompoundStmt *compoundStmt) override;
        virtual bool gen(AST::DeclStmt *declStmt) override;
        virtual bool gen(AST::IfStmt *ifStmt) override;
        virtual bool gen(AST::ValueStmt *valueStmt) override;
        virtual bool gen(AST::ReturnStmt *returnStmt) override;
        virtual bool gen(AST::WhileStmt *whileStmt) override;
        virtual bool gen(AST::CallExpr *callExpr) override;

    public:
        virtual void printCode() const override;
        virtual void dumpCode(const std::string outPath) const override;

    private:
        llvm::LLVMContext _context;
        std::unique_ptr<llvm::IRBuilder<>> _builder;
        std::unique_ptr<llvm::Module> _module;
    };
}