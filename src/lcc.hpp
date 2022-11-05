#pragma once

#include <iostream>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <set>
#include <queue>

#include <json.hpp>

using json = nlohmann::ordered_json;

#include <ProgramOptions.hpp>

#define INFO(msg) \
    std::cout << po::green << "Info: " << po::light_gray << msg << std::endl

#define FATAL_ERROR(msg) \
    std::cout << po::red << "Fatal error: " << po::light_gray << msg << std::endl

#define WARNING(msg) \
    std::cout << po::yellow << "Warning: " << po::light_gray << msg << std::endl

// AST nodes 
namespace cc
{
    namespace AST
    {
        // AST node base class
        class ASTNode
        {
        public:
            virtual json asJson() const = 0;
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
        protected:
            std::vector<std::unique_ptr<Decl>> _decls;

        public:
            TranslationUnitDecl(std::vector<std::unique_ptr<Decl>>& decls) :
            _decls(std::move(decls)) {};
            ~TranslationUnitDecl() = default;

            virtual json asJson() const override;
        };

        // This represents a decl that may have a name
        class NamedDecl : public Decl
        {
        protected:
            std::string _name;

        public:
            NamedDecl(const std::string& name) : 
            _name(name) {};
            ~NamedDecl() = default;

            virtual json asJson() const override;

            const std::string name() const { return _name; };
        };

        // Represents a variable declaration or definition. 
        class VarDecl : public NamedDecl
        {
        protected:
            std::string _type; 
            bool _isInitialized;
            std::unique_ptr<Expr> _value;

        public:
            VarDecl(const std::string& name, const std::string& type, 
            bool isInitialized = false, std::unique_ptr<Expr> value = nullptr) :
            NamedDecl(name), _type(type), _isInitialized(isInitialized), _value(std::move(value)) {};
            
            virtual json asJson() const override;

            const std::string type() const { return _type; };
        };

        // Represents a parameter to a function.
        class ParmVarDecl : public VarDecl
        {
        public:
            ParmVarDecl(const std::string& name, const std::string& type) : VarDecl(name, type)
            {};

            virtual json asJson() const override;
        };
        
        // Represents a function declaration or definition.
        class FunctionDecl : public NamedDecl
        {
        protected:
            std::string _type;
            std::vector<std::unique_ptr<ParmVarDecl>> _params;
            std::unique_ptr<Stmt> _body;

        public:
            FunctionDecl(const std::string& name, const std::string& type, std::vector<std::unique_ptr<ParmVarDecl>>& params, std::unique_ptr<Stmt> body) :
            NamedDecl(name), _type(type), _params(std::move(params)), _body(std::move(body)) {};

            virtual json asJson() const override;
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
            bool _isLValue{ false };

        public:
            bool isLValue() const { return _isLValue; };
        };

        // Integer literal value
        class IntegerLiteral : public Expr
        {
        protected:
            int _value;
        
        public: 
            IntegerLiteral(int value) : 
            _value(value) { _isLValue = false; }; // Integer literal should be LValue instead of RValue 

            virtual json asJson() const override;

            int value() const { return _value; };
        };

        // A reference to a declared variable, function, enum, etc.
        class DeclRefExpr : public Expr
        {
        protected:
            std::string _name;
            bool _isCall;

        public:
            DeclRefExpr(const std::string& name, bool isCall = false) :
            _name(name), _isCall(isCall) { _isLValue = !isCall; };
            
            virtual json asJson() const override;

            const std::string name() const { return _name; };
        };

        // Binary operator type expression
        class BinaryOperator : public Expr
        {
        public:
            // https://en.cppreference.com/w/cpp/language/operator_precedence
            enum class Precedence
            {
                UNDEFINED = 0,
                COMMA = 1, // ,
                ASSIGNMENT = 2, // =, +=, -=, *=, /=, %=, <<=M >>=, &=, ^=, |=
                LOGICALOR = 3, // ||
                LOGICALAND = 4, // &&
                BITWISEOR = 5, // | 
                BITWISEXOR = 6, // ^
                BITWISEAND = 7, // &
                EQUALITY = 8, // == !=
                RELATIONAL = 9, // < <= > >=
                SPACESHIP = 10, // <=>
                BITWISESHIFT = 11, // << >> 
                ADDITIVE = 12, // + -
                MULTIPLICATIVE = 13 // * / %
            };

        protected:
            BinaryOpType _type;
            std::unique_ptr<Expr> _lhs, _rhs;

        public:
            BinaryOperator(BinaryOpType type, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs);
            
            virtual json asJson() const override;

            bool isAssignment() const;

            BinaryOpType type() const { return _type; };
        };

        // Unary operator type expression
        class UnaryOperator : public Expr
        {
        protected:
            UnaryOpType _type;
            std::unique_ptr<Expr> _body;

        public:
            UnaryOperator(UnaryOpType type, std::unique_ptr<Expr> body) :
            _type(type), _body(std::move(body)) 
            {};

            virtual json asJson() const override;

            UnaryOpType type() const { return _type; };
        };

        // This represents a parethesized expression
        class ParenExpr : public Expr
        {
        protected:
            std::unique_ptr<Expr> _subExpr;

        public:
            ParenExpr(std::unique_ptr<Expr> expr) :
            _subExpr(std::move(expr))
            {
                _isLValue = _subExpr->isLValue();
            };

            virtual json asJson() const override;
        };

        // Represents a function call (C99 6.5.2.2, C++ [expr.call]).
        class CallExpr : public Expr
        {
        protected:
            std::unique_ptr<DeclRefExpr> _functionExpr;
            std::vector<std::unique_ptr<Expr>> _params;
        
        public:
            CallExpr(std::unique_ptr<DeclRefExpr> function, std::vector<std::unique_ptr<Expr>>& params) :
            _functionExpr(std::move(function)), _params(std::move(params))
            {};

            virtual json asJson() const override;
        };

        // Base class for type casts, including both implicit casts (ImplicitCastExpr) and explicit casts 
        class CastExpr : public Expr
        {
        protected:
            std::string _kind;
            std::unique_ptr<Expr> _subExpr;

        public:
            CastExpr(std::unique_ptr<Expr> expr, const std::string type):
                _subExpr(std::move(expr)), _kind(type){};

            virtual json asJson() const override;
        };

        // Allows us to explicitly represent implicit type conversions
        class ImplicitCastExpr : public CastExpr
        {
        public:
            ImplicitCastExpr(std::unique_ptr<Expr> expr, const std::string& type): 
                CastExpr(std::move(expr), type){};
                
            virtual json asJson() const override;
        };
    } // Expr end

    // Statements
    namespace AST
    {
        // Stmt base class
        class Stmt : public ASTNode
        {

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
        protected:
            std::unique_ptr<Expr> _expr;
        
        public:
            ValueStmt(std::unique_ptr<Expr> expr) :
            _expr(std::move(expr)){};

            virtual json asJson() const override;
        };

        // This represents an if/then/else.
        class IfStmt : public Stmt
        {
        protected:
            std::unique_ptr<Expr> _condition;
            std::unique_ptr<Stmt> _body;
            std::unique_ptr<Stmt> _elseBody;

        public:
            IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, std::unique_ptr<Stmt> elseBody = nullptr) :
            _condition(std::move(condition)), _body(std::move(body)), _elseBody(std::move(elseBody)){};

            virtual json asJson() const override;
        };

        // This represents a 'while' stmt.
        class WhileStmt : public Stmt
        {
        protected:
            std::unique_ptr<Expr> _condition;
            std::unique_ptr<Stmt> _body;

        public:
            WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body) :
            _condition(std::move(condition)), _body(std::move(body)) {};

            virtual json asJson() const override;
        };

        // Adaptor class for mixing declarations with statements and expressions.
        class DeclStmt : public Stmt
        {
        protected:
            std::vector<std::unique_ptr<Decl>> _decls;

        public:
            DeclStmt(std::vector<std::unique_ptr<Decl>>& decls) : 
            _decls(std::move(decls)) {};

            virtual json asJson() const override;
        };

        // This represents a group of statements like { stmt stmt }.
        class CompoundStmt : public Stmt
        {
        protected:
            std::vector<std::unique_ptr<Stmt>> _body;

        public:
            CompoundStmt(std::vector<std::unique_ptr<Stmt>>& body) :
            _body(std::move(body)) {};

            virtual json asJson() const override;
        };

        // This represents a return, optionally of an expression: return; return 4;. 
        class ReturnStmt : public Stmt
        {
        protected:
            std::unique_ptr<Expr> _value;

        public:
            ReturnStmt(std::unique_ptr<Expr> value = nullptr) :
            _value(std::move(value)) {};

            virtual json asJson() const override;
        };
    }
} // AST end

namespace cc
{
    // A simple vec2 struct for marking position
    typedef struct _Position
    {
        int line;
        int column;
    }Position;

    // File class
    class File
    {
    public:
        File(std::string path);
        ~File();

        const bool fail() const;
    public:
        void nextLine();
        const char nextChar();
        void retractChar();
        const char peekChar();
    
        // getters
        const Position getPosition() const { return _pos; };
        const std::string path() const { return _path; };

    private:
        std::stringstream _ss;
        std::ifstream _ifs;
        std::string _path;
        Position _pos;
        int _curIdx{0};
    };
    
    enum class TokenType
    {
        TOKEN_WHITESPACE,
        TOKEN_NEWLINE,
        TOKEN_IDENTIFIER,
        TOKEN_NUMBER,
        TOKEN_CHAR,
        TOKEN_STRING,
        TOKEN_EOF,
        TOKEN_INVALID,
        #define keyword(name, disc) name,
        #define punctuator(name, disc) name,
        #include "TokenType.inc"
        #undef punctuator
        #undef keyword    
    };

    // Token
    typedef struct _Token
    {
        TokenType type;
        File* file;
        Position pos; // token position in file
        unsigned int count;   // token number in a file
        std::string content; // content of this token

    } Token;

    // Lexer class
    class Lexer
    {
    public:
        ~Lexer() = default;
        Lexer(const Lexer&) = delete;
        Lexer& operator= (const Lexer&) = delete;

    private:
        Lexer();

    public:
        static Lexer* getInstance() {
            if(_inst.get() == nullptr) _inst.reset(new Lexer);

            return _inst.get();
        }

        std::vector<std::shared_ptr<Token>> run(File* file);
    
    private:
        static std::unique_ptr<Lexer> _inst;

    private:
        bool ignoreSpaces();
        void ignoreComments();

        // readers
        std::shared_ptr<Token> nextToken();
        std::shared_ptr<Token> readIdentifier(char c);
        std::shared_ptr<Token> readString();
        std::shared_ptr<Token> readNumber(char c);
        std::shared_ptr<Token> readChar();

        // token makers
        std::shared_ptr<Token> makeGeneralToken(const Token& token) const;
        std::shared_ptr<Token> makeSpaceToken() const;
        std::shared_ptr<Token> makeEOFToken() const;
        std::shared_ptr<Token> makeNewlineToken() const;
        std::shared_ptr<Token> makeInvalidToken() const;
        std::shared_ptr<Token> makeIdentifierToken(std::string& buffer) const;
        std::shared_ptr<Token> makeKeywordToken(TokenType keywordType, std::string& buffer) const;
        std::shared_ptr<Token> makePunctuatorToken(TokenType punctuatorType) const;
        std::shared_ptr<Token> makeStringToken(std::string& buffer) const;
        std::shared_ptr<Token> makeNumberToken(std::string& buffer) const;
        std::shared_ptr<Token> makeCharToken(std::string& buffer);

        // wrapper for file methods
        void nextLine();
        const char nextChar();
        void retractChar();
        const char peekChar();

        // functional
        bool isNextChar(const char c);
        std::shared_ptr<Token> forwardSearch(const char possibleCh, TokenType possibleType, TokenType defaultType);
        std::shared_ptr<Token> forwardSearch(const char possibleCh1, TokenType possibleType1, const char possibleCh2, TokenType possibleType2, TokenType defaultType);

    private:
        File* _file;
        Position _curTokenPos;
        unsigned int _tokenCnt;
        std::unordered_map<std::string, TokenType> _keywordMap;
    };

    // Parser class
    class Parser
    {
    private:
        Parser() = default;
        Parser(const Parser&) = delete;
        Parser& operator=(const Parser&) = delete;

    public:    
        static Parser* getInstance() {
            if(_inst.get() == nullptr) _inst.reset(new Parser);

            return _inst.get();
        }

    private:
        static std::unique_ptr<Parser> _inst;

    public:
        std::unique_ptr<AST::Decl> run(const std::vector<std::shared_ptr<Token>>& tokens);

    private:
        void nextToken();  

    private:
        // decl parsers
        std::unique_ptr<AST::Decl> nextTopLevelDecl(); 
        std::unique_ptr<AST::Decl> nextFunctionDecl(const std::string name, const std::string type);
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
        int _curTokenIdx{ 0 };
        std::shared_ptr<Token> _pCurToken{ nullptr };
    };

    // LR1 Parser class
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
        bool operator==(const Symbol& symbol) const{
            return (this->type() == symbol.type()) && (this->name() == symbol.name());
        }

        bool operator<(const Symbol& symbol) const{
            if(*this == symbol) return false;

            if(this->type() != symbol.type()) return this->type() < symbol.type();

            return this->name() < symbol.name();
        }
    };

    class Terminal : public Symbol
    {
    private:
        std::string _name;
        std::shared_ptr<Token> _token;
    public:
        virtual SymbolType type() const override { return SymbolType::Terminal; };
        virtual std::string name() const override { return _name; };
        Terminal(const std::string& name, std::shared_ptr<Token> token = nullptr) : _name(name), _token(token) {};
    };

    class NonTerminal : public Symbol
    {
    private:
        std::string _name;
        std::unique_ptr<AST::ASTNode> _node;
    public:
        virtual SymbolType type() const override { return SymbolType::NonTerminal; };
        virtual std::string name() const override { return _name; };
        NonTerminal(const std::string& name, std::unique_ptr<AST::ASTNode> node = nullptr) : _name(name), _node(std::move(node)) {};
    };

    typedef struct
    {
        template<typename T>
        bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const
        {
            if(*lhs == *rhs) return false; 

            return *lhs < *rhs;
        };
    }SharedPtrComp; // this is implemented so as to do pointer comparisons
    
    // production is an expression which looks like this: lhs -> rhs[0] rhs[1] ... rhs[n]
    typedef struct _Production
    {
        int id;
        std::shared_ptr<NonTerminal> lhs;
        std::vector<std::shared_ptr<Symbol>> rhs;
        _Production& operator=(const _Production& p) {
            if(this != &p) {
                this->id = p.id;
                this->lhs = p.lhs;
                this->rhs = p.rhs;
            }
            return *this;
        }

        bool operator==(const _Production& p) const {
            if(!(this->id == p.id)) return false;
            if(!(*lhs == *p.lhs)) return false;
            if(rhs.size() != p.rhs.size()) return false;
            for(int i = 0; i < rhs.size(); i++)
                if(!(*rhs[i] == *p.rhs[i])) return false;

            return true;
        }

        bool operator<(const _Production& p) const {
            if(*this == p) return false;
            if(!(this->id == p.id)) return this->id < p.id;
            if(!(*lhs == *p.lhs)) return *lhs < *p.lhs;
            if(rhs.size() != p.rhs.size()) return rhs.size() < p.rhs.size();
            for(int i = 0; i < rhs.size(); i++)
                if(!(*rhs[i] == *p.rhs[i])) return *rhs[i] < *p.rhs[i];
            
            return false;
        }
    }Production;

    // LR(1) item
    typedef struct _LR1Item
    {
        Production production;
        int dotPos;
        std::shared_ptr<Symbol> lookahead;

        bool operator==(const _LR1Item& item) const
        {
            return (production == item.production) && (dotPos == item.dotPos) && (*lookahead == *item.lookahead);
        }

        bool operator <(const _LR1Item& item) const
        {
            if(*this == item) return false;

            if(!(production == item.production)) return production < item.production;
            if(!(dotPos == item.dotPos)) return dotPos < item.dotPos;
            
            return *lookahead < *item.lookahead;
        }
    }LR1Item;

    // LR(1) item set
    typedef struct _LR1ItemSet
    {
        int id;
        std::set<LR1Item> items;
        std::map<std::shared_ptr<Symbol>, int> transitions;
        bool operator==(const _LR1ItemSet& itemSet) const
        {
            if(items.size() != itemSet.items.size()) return false;
            for(auto i = items.begin(), j = itemSet.items.begin(); i != items.end() && j != itemSet.items.end();i++, j++)
            {
                if(!(*i == *j)) return false;
            }

            return true;
        }

        bool operator <(const _LR1ItemSet& itemSet) const
        {
            if(*this == itemSet) return false;
            if(this->items.size() != itemSet.items.size()) return this->items.size() < itemSet.items.size();
            for(auto i = items.begin(), j = itemSet.items.begin(); i != items.end() && j != itemSet.items.end();i++, j++)
            {
                if(!(*i == *j)) return *i < *j;
            }

            return false;
        }
    }LR1ItemSet;

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
    }Action;
    
    
    private:
        LR1Parser() = default;
        LR1Parser(const LR1Parser&) = delete;
        LR1Parser& operator=(const LR1Parser&) = delete;

    public:    
        static LR1Parser* getInstance() {
            if(_inst.get() == nullptr) _inst.reset(new LR1Parser);

            return _inst.get();
        }

    private:
        static std::unique_ptr<LR1Parser> _inst;

    public:
        std::unique_ptr<AST::Decl> run(const std::vector<std::shared_ptr<Token>>& tokens, const std::string& productionFilePath);

    private:
        void parseProductionsFromJson(const std::string& productionFilePath);

        void findFirstSetForSymbols();

        void constructCanonicalCollections();

        void constructLR1ParseTable();

        void closure(LR1ItemSet& itemSet);

        LR1ItemSet go(LR1ItemSet& itemSet, std::shared_ptr<Symbol> symbol);

        std::unique_ptr<AST::Decl> parse(const std::vector<std::shared_ptr<Token>>& tokens);

        // some helpers
        bool isTerminal(const std::shared_ptr<Symbol>& symbol) const {
            return symbol->type() == SymbolType::Terminal;
        }

        bool isNonTerminal(const std::shared_ptr<Symbol>& symbol) const {
            return symbol->type() == SymbolType::NonTerminal;
        }

        bool isEpsilon(const std::shared_ptr<Symbol>& symbol) {
            return symbol->name() == "$";
        }

        template<typename T, typename _Pr=std::less<T>>
        bool doInsert(std::set<T, _Pr>& s, T elem) // if insertion is successful return true
        {
            auto lastSize = s.size();
            s.insert(elem);
            return lastSize != s.size();
        }

        void printItemSet(LR1ItemSet& itemSet) const;
        void printProduction(const Production& production, bool shouldAddDot = false, int dotPos = 0) const;
        void printActionAndGotoTable() const;

    private:
        std::shared_ptr<Terminal> _endSymbol; // #
        std::shared_ptr<Terminal> _epsilonSymbol; // $
        std::shared_ptr<NonTerminal> _extensiveStartSymbol; // S'
        std::set<std::shared_ptr<Terminal>, SharedPtrComp> _terminals;
        std::set<std::shared_ptr<NonTerminal>, SharedPtrComp> _nonTerminals;
        std::map<std::string, std::vector<Production>> _productions;
        std::map<std::string, std::set<std::shared_ptr<Symbol>, SharedPtrComp>> _first; // FIRST
        std::vector<LR1ItemSet> _canonicalCollections;

        std::vector<std::map<std::string, Action>> _actionTable; // ACTION
        std::vector<std::map<std::string, int>> _gotoTable; // GOTO 
    };

    // some util funcs
    bool isSpace(const char c);
    json jsonifyTokens(const std::vector<std::shared_ptr<Token>>& tokens);
    bool dumpJson(const json& j, const std::string outPath);
}