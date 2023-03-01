#pragma once

#include <string>
#include <memory>
#include <set>
#include <stack>
#include <queue>

#include "AST.hpp"
#include "lexer.hpp"

namespace lcc
{
    // LR1 Parser class(LR1Parser.cpp)
    class LR1Parser
    {
        enum class SymbolType
        {
            Terminal = 0,
            NonTerminal
        };

        class Symbol
        {
        public:
            virtual SymbolType type() const = 0;
            virtual std::string name() const = 0;
            bool operator==(const Symbol &symbol) const
            {
                return (this->type() == symbol.type()) && (this->name() == symbol.name());
            }

            bool operator<(const Symbol &symbol) const
            {
                if (*this == symbol)
                    return false;

                if (this->type() != symbol.type())
                    return this->type() < symbol.type();

                return this->name() < symbol.name();
            }
        };

        class Terminal : public Symbol
        {
            friend class LR1Parser;

        private:
            std::string _name;
            std::shared_ptr<Token> _token;

        public:
            virtual SymbolType type() const override { return SymbolType::Terminal; };
            virtual std::string name() const override { return _name; };
            Terminal(const std::string &name, std::shared_ptr<Token> token = nullptr) : _name(name), _token(token){};
        };

        class NonTerminal : public Symbol
        {
            friend class LR1Parser;

        private:
            std::string _name;
            std::unique_ptr<AST::ASTNode> _node;

        public:
            virtual SymbolType type() const override { return SymbolType::NonTerminal; };
            virtual std::string name() const override { return _name; };
            NonTerminal(const std::string &name, std::unique_ptr<AST::ASTNode> node = nullptr) : _name(name), _node(std::move(node)){};
            NonTerminal(NonTerminal &&nonTerminal) : _name(nonTerminal.name()), _node(std::move(nonTerminal._node)){};
        };

        typedef struct
        {
            template <typename T>
            bool operator()(const std::shared_ptr<T> &lhs, const std::shared_ptr<T> &rhs) const
            {
                if (*lhs == *rhs)
                    return false;

                return *lhs < *rhs;
            };
        } SharedPtrComp; // this is implemented so as to do pointer comparisons

        // production is an expression which looks like this: lhs -> rhs[0] rhs[1] ... rhs[n]
        typedef struct _Production
        {
            int id;
            std::shared_ptr<NonTerminal> lhs;
            std::vector<std::shared_ptr<Symbol>> rhs;
            _Production &operator=(const _Production &p)
            {
                if (this != &p)
                {
                    this->id = p.id;
                    this->lhs = p.lhs;
                    this->rhs = p.rhs;
                }
                return *this;
            }

            bool operator==(const _Production &p) const
            {
                if (!(this->id == p.id))
                    return false;
                if (!(*lhs == *p.lhs))
                    return false;
                if (rhs.size() != p.rhs.size())
                    return false;
                for (int i = 0; i < rhs.size(); i++)
                    if (!(*rhs[i] == *p.rhs[i]))
                        return false;

                return true;
            }

            bool operator<(const _Production &p) const
            {
                if (*this == p)
                    return false;
                if (!(this->id == p.id))
                    return this->id < p.id;
                if (!(*lhs == *p.lhs))
                    return *lhs < *p.lhs;
                if (rhs.size() != p.rhs.size())
                    return rhs.size() < p.rhs.size();
                for (int i = 0; i < rhs.size(); i++)
                    if (!(*rhs[i] == *p.rhs[i]))
                        return *rhs[i] < *p.rhs[i];

                return false;
            }
        } Production;

        // LR(1) item
        typedef struct _LR1Item
        {
            Production production;
            int dotPos;
            std::shared_ptr<Symbol> lookahead;

            bool operator==(const _LR1Item &item) const
            {
                return (production == item.production) && (dotPos == item.dotPos) && (*lookahead == *item.lookahead);
            }

            bool operator<(const _LR1Item &item) const
            {
                if (*this == item)
                    return false;

                if (!(production == item.production))
                    return production < item.production;
                if (!(dotPos == item.dotPos))
                    return dotPos < item.dotPos;

                return *lookahead < *item.lookahead;
            }
        } LR1Item;

        // LR(1) item set
        typedef struct _LR1ItemSet
        {
            int id;
            std::set<LR1Item> items;
            std::map<std::shared_ptr<Symbol>, int> transitions;
            bool operator==(const _LR1ItemSet &itemSet) const
            {
                if (items.size() != itemSet.items.size())
                    return false;
                for (auto i = items.begin(), j = itemSet.items.begin(); i != items.end() && j != itemSet.items.end(); i++, j++)
                {
                    if (!(*i == *j))
                        return false;
                }

                return true;
            }

            bool operator<(const _LR1ItemSet &itemSet) const
            {
                if (*this == itemSet)
                    return false;
                if (this->items.size() != itemSet.items.size())
                    return this->items.size() < itemSet.items.size();
                for (auto i = items.begin(), j = itemSet.items.begin(); i != items.end() && j != itemSet.items.end(); i++, j++)
                {
                    if (!(*i == *j))
                        return *i < *j;
                }

                return false;
            }
        } LR1ItemSet;

        // ACTION table action definitions
        enum class ActionType
        {
            INVALID = 0,
            ACC,
            SHIFT, // s
            REDUCE // r
        };

        typedef struct
        {
            ActionType type;
            int id;
        } Action;

    private:
        LR1Parser();
        LR1Parser(const LR1Parser &) = delete;
        LR1Parser &operator=(const LR1Parser &) = delete;

    public:
        static LR1Parser *getInstance()
        {
            if (_inst.get() == nullptr)
                _inst.reset(new LR1Parser);

            return _inst.get();
        }

    private:
        static std::unique_ptr<LR1Parser> _inst;

    public:
        std::unique_ptr<AST::Decl> run(const std::vector<std::shared_ptr<Token>> &tokens, const std::string &productionFilePath, bool shouldPrintProcess);

        // parsing
    private:
        std::unique_ptr<AST::Decl> parse(const std::vector<std::shared_ptr<Token>> &tokens, bool shouldPrintProcess);

        void nextToken();

        // expression parsers
        std::shared_ptr<NonTerminal> nextExpr(); // expr parser implemented with OperatorPrecedence Parse
        std::unique_ptr<AST::Expr> nextExpression();
        std::unique_ptr<AST::Expr> nextRHSExpr(std::unique_ptr<AST::Expr> lhs, AST::BinaryOperator::Precedence lastBiOpPrec);
        std::unique_ptr<AST::Expr> nextUnaryOperator();
        std::unique_ptr<AST::Expr> nextPrimaryExpr();
        std::unique_ptr<AST::Expr> nextVarRefOrFuncCall();
        std::unique_ptr<AST::Expr> nextNumber();
        std::unique_ptr<AST::Expr> nextParenExpr();
        std::unique_ptr<AST::Expr> nextRValue();

        // AsmStmt parser
        std::shared_ptr<NonTerminal> nextAsmStmt();

        // production func
        static std::shared_ptr<NonTerminal> nextStartSymbolR1(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);         // production 1
        static std::shared_ptr<NonTerminal> nextTranslationUnitDeclR2(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack); // production 2
        static std::shared_ptr<NonTerminal> nextTranslationUnitDeclR3(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack); // production 3
        static std::shared_ptr<NonTerminal> nextDecl(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);                  // production 4, 16
        static std::shared_ptr<NonTerminal> nextFunctionDeclR5(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);        // production 5
        static std::shared_ptr<NonTerminal> nextFunctionDeclR6(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);        // production 6
        static std::shared_ptr<NonTerminal> nextParmVarDeclR7(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);         // production 7
        static std::shared_ptr<NonTerminal> nextParmVarDeclR8(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);         // production 8
        static std::shared_ptr<NonTerminal> nextFunctionDeclR9(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);        // production 9
        static std::shared_ptr<NonTerminal> nextCompoundStmtR10(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);       // production 10
        static std::shared_ptr<NonTerminal> nextFunctionDeclR11(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);       // production 11
        static std::shared_ptr<NonTerminal> nextFunctionDeclR12(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);       // production 12
        static std::shared_ptr<NonTerminal> nextFunctionDeclR13(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);       // production 13
        static std::shared_ptr<NonTerminal> nextFunctionDeclR14(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);       // production 14
        static std::shared_ptr<NonTerminal> nextFunctionDeclR15(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);       // production 15
        static std::shared_ptr<NonTerminal> nextVarDeclR17(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);            // production 17
        static std::shared_ptr<NonTerminal> nextVarDeclR18(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);            // production 18
        static std::shared_ptr<NonTerminal> nextCompoundStmtR19(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);       // production 19
        static std::shared_ptr<NonTerminal> nextStmtsR20(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);              // production 20
        static std::shared_ptr<NonTerminal> nextStmtsR21(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);              // production 21
        static std::shared_ptr<NonTerminal> nextStmt(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);                  // production 22 ~ 28
        static std::shared_ptr<NonTerminal> nextWhileStmtR29(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);          // production 29
        static std::shared_ptr<NonTerminal> nextIfStmtR30(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);             // production 30
        static std::shared_ptr<NonTerminal> nextIfStmtR31(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);             // production 31
        static std::shared_ptr<NonTerminal> nextReturnStmtR32(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);         // production 32
        static std::shared_ptr<NonTerminal> nextReturnStmtR33(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);         // production 33
        static std::shared_ptr<NonTerminal> nextNullStmtR34(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);           // production 34
        static std::shared_ptr<NonTerminal> nextDeclStmtR35(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);           // production 35
        static std::shared_ptr<NonTerminal> nextValueStmtR36(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);          // production 36
        static std::shared_ptr<NonTerminal> nextFunctionDeclR38(std::stack<int> &stateStack, std::stack<std::shared_ptr<Symbol>> &symbolStack);          // production 38

        // grammar initialization
    private:
        bool parseProductionsFromFile(const std::string &grammarFilePath);

        void findFirstSetForSymbols();

        void constructCanonicalCollections();

        void constructLR1ParseTable();

        void closure(LR1ItemSet &itemSet);

        LR1ItemSet go(LR1ItemSet &itemSet, std::shared_ptr<Symbol> symbol);

        // some helpers
        bool isTerminal(const std::shared_ptr<Symbol> &symbol) const
        {
            return symbol->type() == SymbolType::Terminal;
        }

        bool isNonTerminal(const std::shared_ptr<Symbol> &symbol) const
        {
            return symbol->type() == SymbolType::NonTerminal;
        }

        bool isEpsilon(const std::shared_ptr<Symbol> &symbol)
        {
            return symbol->name() == "$";
        }

        template <typename T, typename _Pr = std::less<T>>
        bool doInsert(std::set<T, _Pr> &s, T elem) // if insertion is successful return true
        {
            auto lastSize = s.size();
            s.insert(elem);
            return lastSize != s.size();
        }

        void printItemSet(LR1ItemSet &itemSet) const;
        void printProduction(const Production &production, bool shouldAddDot = false, int dotPos = 0) const;
        void printActionAndGotoTable() const;

    private:
        std::shared_ptr<Terminal> _endSymbol;               // #
        std::shared_ptr<Terminal> _epsilonSymbol;           // $
        std::shared_ptr<NonTerminal> _extensiveStartSymbol; // S'
        std::set<std::shared_ptr<Terminal>, SharedPtrComp> _terminals;
        std::set<std::shared_ptr<NonTerminal>, SharedPtrComp> _nonTerminals;
        std::map<std::string, std::vector<Production>> _productions;
        std::map<std::string, std::set<std::shared_ptr<Symbol>, SharedPtrComp>> _first; // FIRST
        std::vector<LR1ItemSet> _canonicalCollections;

        std::vector<std::map<std::string, Action>> _actionTable; // ACTION
        std::vector<std::map<std::string, int>> _gotoTable;      // GOTO

        std::vector<std::shared_ptr<Token>> _tokens;
        int _curTokenIdx{0};
        std::shared_ptr<Token> _pCurToken{nullptr};

        std::map<int, std::function<std::shared_ptr<NonTerminal>(std::stack<int> &, std::stack<std::shared_ptr<Symbol>> &)>> _productionFuncMap;
    };
}