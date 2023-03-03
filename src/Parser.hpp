#pragma once

#include <memory>

#include "AST.hpp"
#include "Lexer.hpp"

namespace lcc
{
    // Parser class(Parser.cpp)
    class Parser
    {
    private:
        Parser() = default;
        Parser(const Parser &) = delete;
        Parser &operator=(const Parser &) = delete;

    public:
        static Parser *getInstance()
        {
            if (_inst.get() == nullptr)
                _inst.reset(new Parser);

            return _inst.get();
        }

    private:
        static std::unique_ptr<Parser> _inst;

    public:
        std::unique_ptr<AST::Decl> run(const std::vector<std::shared_ptr<Token>> &tokens);

    private:
        void nextToken();

    private:
        // decl parsers
        std::unique_ptr<AST::Decl> nextTopLevelDecl();
        std::unique_ptr<AST::Decl> nextFunctionDecl(const std::string name, const std::string type, const bool isExtern);
        std::unique_ptr<AST::Decl> nextVarDecl(const std::string name, const std::string type);
        // stmt parsers
        std::unique_ptr<AST::Stmt> nextCompoundStmt();
        std::unique_ptr<AST::Stmt> nextStmt();
        std::unique_ptr<AST::Stmt> nextNullStmt();
        std::unique_ptr<AST::Stmt> nextWhileStmt();
        std::unique_ptr<AST::Stmt> nextIfStmt();
        std::unique_ptr<AST::Stmt> nextReturnStmt();
        std::unique_ptr<AST::Stmt> nextDeclStmt();
        std::unique_ptr<AST::Stmt> nextValueStmt();
        std::unique_ptr<AST::Stmt> nextAsmStmt();
        // expr parsers
        std::unique_ptr<AST::Expr> nextExpression();
        std::unique_ptr<AST::Expr> nextUnaryOperator();
        std::unique_ptr<AST::Expr> nextBinaryOperator();
        std::unique_ptr<AST::Expr> nextRValue();
        std::unique_ptr<AST::Expr> nextPrimaryExpr();
        std::unique_ptr<AST::Expr> nextVarRefOrFuncCall();
        std::unique_ptr<AST::Expr> nextNumber();
        std::unique_ptr<AST::Expr> nextParenExpr();
        std::unique_ptr<AST::Expr> nextRHSExpr(std::unique_ptr<AST::Expr> lhs, AST::BinaryOperator::Precedence lastBiOpPrec);

    private:
        std::vector<std::shared_ptr<Token>> _tokens;
        int _curTokenIdx{0};
        std::shared_ptr<Token> _pCurToken{nullptr};
    };
}