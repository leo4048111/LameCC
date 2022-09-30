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

#define FATAL_ERROR(msg) \
    std::cout << po::red << "Fatal error: " << po::light_gray << msg << std::endl

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
        TOKEN_KEYWORD, // single ch keywords are marked by this enum
        TOKEN_NEWLINE,
        TOKEN_IDENTIFIER,
        TOKEN_NUMBER,
        TOKEN_CHAR,
        TOKEN_STRING,
        TOKEN_EOF,
        TOKEN_INVALID,
        #define keyword(name, disc) name,
        #define operator(name, disc) name,
        #include "TokenType.inc"
        #undef operator
        #undef keyword    
    };

    // Token
    typedef struct _Token
    {
        TokenType type;
        File* file;
        Position pos; // token position in file
        bool space;  // whether this token has a leading space
        bool bol;    // whether this token is at the beginning of a line
        unsigned int count;   // token number in a file

        union 
        {
            // token's keyword id if token is a keyword
            int id;
            // string, char, number and identifier name
            struct
            {
                const char* pchar;
                int length;
            };
        };
    } Token;

    // Lexer class
    class Lexer
    {
    public:
        Lexer(File* file);
        ~Lexer() = default;

        void run();

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
        Token* makeGeneralToken(Token token) const;
        Token* makeSpaceToken() const;
        Token* makeEOFToken() const;
        Token* makeNewlineToken() const;
        Token* makeInvalidToken() const;
        Token* makeIdentifierToken(CharBuffer& buffer) const;
        Token* makeKeywordToken(TokenType keywordType) const;
        Token* makeKeywordToken(int id) const;
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

    //utils
    bool isSpace(const char c);
}