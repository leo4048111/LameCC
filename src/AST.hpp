#pragma once

#include <iostream>
#include <memory>

#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

// AST nodes
namespace lcc
{
    class LR1Parser;
    class IRGeneratorBase;
    class QuaternionIRGenerator;
    class LLVMIRGenerator;

    namespace AST
    {
        // AST node base class
        class ASTNode
        {
        public:
            std::string place{""}; // for IR generation
        public:
            virtual json asJson() const = 0;
            virtual bool gen(lcc::IRGeneratorBase *generator) { return true; }; // CHANGE THIS TO PURE VIRTUAL LATER!!!
            virtual ~ASTNode(){};
        };

        // Cecls
        class Decl;
        class TranslationUnitDecl;
        class NamedDecl;
        class VarDecl;
        class ParmVarDecl;
        class FunctionDecl;

        // Exprs
        class Expr;
        class IntegerLiteral;
        class DeclRefExpr;
        class BinaryOperator;
        class UnaryOperator;
        class ParenExpr;
        class CallExpr;
        class CastExpr;
        class ImplicitCastExpr;

        // Stmts
        class Stmt;
        class NullStmt;
        class ValueStmt;
        class IfStmt;
        class WhileStmt;
        class DeclStmt;
        class CompoundStmt;
        class ReturnStmt;
    }

    // Declarations
    namespace AST
    {
        // Decl base class
        class Decl : public ASTNode
        {
        };

        // root node for AST
        class TranslationUnitDecl : public Decl
        {
            friend class lcc::LR1Parser;
            friend class lcc::IRGeneratorBase;
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::vector<std::unique_ptr<Decl>> _decls;

        public:
            TranslationUnitDecl(std::vector<std::unique_ptr<Decl>> &decls) : _decls(std::move(decls)){};
            ~TranslationUnitDecl() = default;

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // This represents a decl that may have a name
        class NamedDecl : public Decl
        {
        protected:
            std::string _name;

        public:
            NamedDecl(const std::string &name) : _name(name){};
            ~NamedDecl() = default;

            virtual json asJson() const override;

            const std::string name() const { return _name; };
        };

        // Represents a variable declaration or definition.
        class VarDecl : public NamedDecl
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::string _type;
            bool _isInitialized;
            std::unique_ptr<Expr> _value;

        public:
            VarDecl(const std::string &name, const std::string &type,
                    bool isInitialized = false, std::unique_ptr<Expr> value = nullptr) : NamedDecl(name), _type(type), _isInitialized(isInitialized), _value(std::move(value)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;

            const std::string type() const { return _type; };
        };

        // Represents a parameter to a function.
        class ParmVarDecl : public VarDecl
        {
            friend class lcc::LR1Parser;

        protected:
            std::unique_ptr<ParmVarDecl> _nextParmVarDecl;

        public:
            ParmVarDecl(const std::string &name, const std::string &type, std::unique_ptr<ParmVarDecl> nextParmVarDecl = nullptr) : VarDecl(name, type), _nextParmVarDecl(std::move(nextParmVarDecl)){};

            virtual json asJson() const override;
        };

        // Represents a function declaration or definition.
        class FunctionDecl : public NamedDecl
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::string _type;
            std::vector<std::unique_ptr<ParmVarDecl>> _params;
            std::unique_ptr<Stmt> _body;

        public:
            FunctionDecl(const std::string &name, const std::string &type, std::vector<std::unique_ptr<ParmVarDecl>> &params, std::unique_ptr<Stmt> body) : NamedDecl(name), _type(type), _params(std::move(params)), _body(std::move(body)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };
    } // Decl end

    // Expressions
    namespace AST
    {
        // Binary operator types
        enum class BinaryOpType
        {
            BO_UNDEFINED = 0,
#define BINARY_OPERATION(name, disc) BO_##name,
#define UNARY_OPERATION(name, disc)
#include "OperationType.inc"
#undef UNARY_OPERATION
#undef BINARY_OPERATION
        };

        enum class UnaryOpType
        {
            UO_UNDEFINED = 0,
#define BINARY_OPERATION(name, disc)
#define UNARY_OPERATION(name, disc) UO_##name,
#include "OperationType.inc"
#undef UNARY_OPERATION
#undef BINARY_OPERATION
        };

        // Expr base class
        class Expr : public ASTNode
        {
        protected:
            bool _isLValue{false};

        public:
            bool isLValue() const { return _isLValue; };
        };

        // Integer literal value
        class IntegerLiteral : public Expr
        {
            friend class lcc::QuaternionIRGenerator;

        protected:
            int _value;

        public:
            IntegerLiteral(int value) : _value(value) { _isLValue = false; }; // Integer literal should be LValue instead of RValue

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;

            int value() const { return _value; };
        };

        // Floating literal value
        class FloatingLiteral : public Expr
        {
            friend class lcc::QuaternionIRGenerator;

        protected:
            float _value;

        public:
            FloatingLiteral(float value) : _value(value) { _isLValue = false; }; // Floating literal should be LValue instead of RValue

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;

            float value() const { return _value; };
        };

        // A reference to a declared variable, function, enum, etc.
        class DeclRefExpr : public Expr
        {
            friend class lcc::QuaternionIRGenerator;

        protected:
            std::string _name;
            bool _isCall;

        public:
            DeclRefExpr(const std::string &name, bool isCall = false) : _name(name), _isCall(isCall) { _isLValue = !isCall; };

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;

            const std::string name() const { return _name; };
        };

        // Binary operator type expression
        class BinaryOperator : public Expr
        {
            friend class lcc::QuaternionIRGenerator;

        public:
            // https://en.cppreference.com/w/cpp/language/operator_precedence
            enum class Precedence
            {
                UNDEFINED = 0,
                COMMA = 1,          // ,
                ASSIGNMENT = 2,     // =, +=, -=, *=, /=, %=, <<=M >>=, &=, ^=, |=
                LOGICALOR = 3,      // ||
                LOGICALAND = 4,     // &&
                BITWISEOR = 5,      // |
                BITWISEXOR = 6,     // ^
                BITWISEAND = 7,     // &
                EQUALITY = 8,       // == !=
                RELATIONAL = 9,     // < <= > >=
                SPACESHIP = 10,     // <=>
                BITWISESHIFT = 11,  // << >>
                ADDITIVE = 12,      // + -
                MULTIPLICATIVE = 13 // * / %
            };

        protected:
            BinaryOpType _type;
            std::unique_ptr<Expr> _lhs, _rhs;

        public:
            BinaryOperator(BinaryOpType type, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs);

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;

            bool isAssignment() const;

            BinaryOpType type() const { return _type; };
        };

        // Unary operator type expression
        class UnaryOperator : public Expr
        {
            friend class lcc::QuaternionIRGenerator;

        protected:
            UnaryOpType _type;
            std::unique_ptr<Expr> _body;

        public:
            UnaryOperator(UnaryOpType type, std::unique_ptr<Expr> body) : _type(type), _body(std::move(body)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;

            UnaryOpType type() const { return _type; };
        };

        // This represents a parethesized expression
        class ParenExpr : public Expr
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::unique_ptr<Expr> _subExpr;

        public:
            ParenExpr(std::unique_ptr<Expr> expr) : _subExpr(std::move(expr))
            {
                _isLValue = _subExpr->isLValue();
            };

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // Represents a function call (C99 6.5.2.2, C++ [expr.call]).
        class CallExpr : public Expr
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::unique_ptr<DeclRefExpr> _functionExpr;
            std::vector<std::unique_ptr<Expr>> _params;

        public:
            CallExpr(std::unique_ptr<DeclRefExpr> function, std::vector<std::unique_ptr<Expr>> &params) : _functionExpr(std::move(function)), _params(std::move(params)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // Base class for type casts, including both implicit casts (ImplicitCastExpr) and explicit casts
        class CastExpr : public Expr
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;
        
        public:
            enum class CastType
            {
                LValueToRValue = 0
            };

        protected:
            CastType _type;
            std::unique_ptr<Expr> _subExpr;

        public:
            CastExpr(std::unique_ptr<Expr> expr, const CastType type) : _subExpr(std::move(expr)), _type(type){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // Allows us to explicitly represent implicit type conversions
        class ImplicitCastExpr : public CastExpr
        {
        public:
            ImplicitCastExpr(std::unique_ptr<Expr> expr, CastType type) : CastExpr(std::move(expr), type){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };
    } // Expr end

    // Statements
    namespace AST
    {
        // Stmt base class
        class Stmt : public ASTNode
        {
            friend class lcc::LR1Parser;

        protected:
            std::unique_ptr<Stmt> _nextStmt;

        public:
            Stmt(std::unique_ptr<Stmt> nextStmt = nullptr) : _nextStmt(std::move(nextStmt)){};
        };

        // This is the null statement ";": C99 6.8.3p3.
        class NullStmt : public Stmt
        {
        public:
            virtual json asJson() const override;
        };

        // Represents a statement that could possibly have a value and type.
        class ValueStmt : public Stmt
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::unique_ptr<Expr> _expr;

        public:
            ValueStmt(std::unique_ptr<Expr> expr) : _expr(std::move(expr)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // This represents an if/then/else.
        class IfStmt : public Stmt
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::unique_ptr<Expr> _condition;
            std::unique_ptr<Stmt> _body;
            std::unique_ptr<Stmt> _elseBody;

        public:
            IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, std::unique_ptr<Stmt> elseBody = nullptr) : _condition(std::move(condition)), _body(std::move(body)), _elseBody(std::move(elseBody)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // This represents a 'while' stmt.
        class WhileStmt : public Stmt
        {
            friend class lcc::QuaternionIRGenerator;

        protected:
            std::unique_ptr<Expr> _condition;
            std::unique_ptr<Stmt> _body;

        public:
            WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body) : _condition(std::move(condition)), _body(std::move(body)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // Adaptor class for mixing declarations with statements and expressions.
        class DeclStmt : public Stmt
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::vector<std::unique_ptr<Decl>> _decls;

        public:
            DeclStmt(std::vector<std::unique_ptr<Decl>> &decls) : _decls(std::move(decls)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // This represents a group of statements like { stmt stmt }.
        class CompoundStmt : public Stmt
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::vector<std::unique_ptr<Stmt>> _body;

        public:
            CompoundStmt(std::vector<std::unique_ptr<Stmt>> &body) : _body(std::move(body)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };

        // This represents a return, optionally of an expression: return; return 4;.
        class ReturnStmt : public Stmt
        {
            friend class lcc::QuaternionIRGenerator;
            friend class lcc::LLVMIRGenerator;

        protected:
            std::unique_ptr<Expr> _value;

        public:
            ReturnStmt(std::unique_ptr<Expr> value = nullptr) : _value(std::move(value)){};

            virtual json asJson() const override;

            virtual bool gen(lcc::IRGeneratorBase *generator) override;
        };
    }
} // AST end