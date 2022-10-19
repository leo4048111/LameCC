#pragma once

#include <iostream>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include <json.hpp>

using json = nlohmann::json;

#include <ProgramOptions.hpp>

#define INFO(msg) \
    std::cout << po::green << "Info: " << po::light_gray << msg << std::endl

#define FATAL_ERROR(msg) \
    std::cout << po::red << "Fatal error: " << po::light_gray << msg << std::endl

#define WARNING(msg) \
    std::cout << po::yellow << "Warning: " << po::light_gray << msg << std::endl

namespace cc
{
    // im using this instead of std::string
    class CharBuffer
    {
    public:
        CharBuffer();
        ~CharBuffer();

    public:
        const char charAt(unsigned int idx);
        void append(const char c);
        void pop();
        void reserve(size_t newSize);
        const size_t size() const { return _size; };
        const char* c_str();
        // note that this method is different from cc::CharBuffer::c_str()
        // because it allocates memory on heap
        const char* new_c_str() const;
        char* begin(){ return &_buffer[0]; };
        char* end() {return &_buffer[_size - 1];};

        // implement this method here to make things easier
        bool operator == (const char* s);
    private:
        char* _buffer;
        size_t _size;
        size_t _capacity;
    };

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

    private:
        std::stringstream _ss;
        std::ifstream _ifs;

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
        #define operator(name, disc) name,
        #define punctuator(name, disc) name,
        #include "TokenType.inc"
        #undef punctuator
        #undef operator
        #undef keyword    
    };

    // Token
    typedef struct _Token
    {
        TokenType type;
        File* file;
        Position pos; // token position in file
        unsigned int count;   // token number in a file
        const char* pContent { nullptr }; // content of this token 
        
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

        void install(File* file);
        std::vector<Token*> run(const bool shouldDumpTokens, const std::string outPath);
    
    private:
        static std::unique_ptr<Lexer> _inst;

    private:
        bool ignoreSpaces();
        void ignoreComments();

        // readers
        Token* nextToken();
        Token* readIdentifier(char c);
        Token* readString();
        Token* readNumber(char c);
        Token* readChar();

        // token makers
        Token* makeGeneralToken(const Token& token) const;
        Token* makeSpaceToken() const;
        Token* makeEOFToken() const;
        Token* makeNewlineToken() const;
        Token* makeInvalidToken() const;
        Token* makeIdentifierToken(CharBuffer& buffer) const;
        Token* makeKeywordToken(TokenType keywordType) const;
        Token* makeStringToken(CharBuffer& buffer) const;
        Token* makeNumberToken(CharBuffer& buffer) const;
        Token* makeCharToken(CharBuffer& buffer);

        // wrapper for file methods
        void nextLine();
        const char nextChar();
        void retractChar();
        const char peekChar();

        // functional
        bool isNextChar(const char c);
        Token* forwardSearch(const char possibleCh, TokenType possibleType, TokenType defaultType);
        Token* forwardSearch(const char possibleCh1, TokenType possibleType1, const char possibleCh2, TokenType possibleType2, TokenType defaultType);

    private:
        File* _file;
        Position _curTokenPos;
        unsigned int _tokenCnt;
        std::unordered_map<const char*, TokenType> _keywordMap;
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
        void setup(const std::vector<Token*>& tokens); 

        void run(const std::vector<Token*>& tokens);

    private:
        void nextToken();   

    private:
        std::vector<Token*> _tokens;
        std::vector<Token*>::iterator _pCurToken;
    };

    //utils
    bool isSpace(const char c);
    json jsonifyTokens(const std::vector<Token*>& tokens);
    void freeToken(Token*& token);
    bool dumpJson(const json& j, const std::string outPath);
}

// #define NONCOPYABLE(classname) \
// public: \
//     classname(const classname&) = delete; \
//     classname& operator=(const classname&) = delete;

// AST
namespace cc
{
    namespace AST
    {
        // AST node base class
        class ASTNode
        {
        };

        class Decl;
        class TranslationUnitDecl;
        class NamedDecl;
        class VarDecl;

        class Expr;
        class IntegerLiteral;
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
            TranslationUnitDecl(std::vector<std::unique_ptr<Decl>> decls) :
            _decls(std::move(decls)) {};
            ~TranslationUnitDecl() = default;
        };

        // This represents a decl that may have a name
        class NamedDecl : public Decl
        {
        protected:
            std::string _name;

        public:
            NamedDecl(const std::string name) : _name(name) {};
            ~NamedDecl() = default;

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
            VarDecl(const std::string name, const std::string type, 
            bool isInitialized = false, std::unique_ptr<Expr> value = nullptr) :
            NamedDecl(name), _type(type), _isInitialized(isInitialized), _value(std::move(value))
            {};
        };
    }

    // Expressions
    namespace AST
    {
        // Expr base class
        class Expr : public ASTNode
        {
        protected:
            bool _isConstant{ false };
            bool _isLValue{ false };

        public:
            bool isConstant() const { return _isConstant; };
            bool isLValue() const { return _isLValue; };
        };

        // Integer literal value
        class IntegerLiteral : public Expr
        {
        protected:
            int _value;
        
        public: 
            IntegerLiteral(int value) : _value(value) {};
            int value() const { return _value; };
        };

        // A reference to a declared variable, function, enum, etc.
        class DeclRefExpr : public Expr
        {
        protected:
            std::string _name;
            bool _isCall;

        public:
            DeclRefExpr(const std::string name, bool isCall = false) :
            _name(name), _isCall(isCall)
            {};

            const std::string name() const { return _name; };
        };
    }

}