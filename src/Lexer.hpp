#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>

#include "File.hpp"

namespace lcc
{
    enum class TokenType
    {
        TOKEN_WHITESPACE,
        TOKEN_NEWLINE,
        TOKEN_IDENTIFIER,
        TOKEN_INTEGER,
        TOKEN_FLOAT,
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
        std::shared_ptr<File> file;
        Position pos;        // token position in file
        unsigned int count;  // token number in a file
        std::string content; // content of this token

    } Token;

    // Lexer class(Lexer.cpp)
    class Lexer
    {
    public:
        ~Lexer() = default;
        Lexer(const Lexer &) = delete;
        Lexer &operator=(const Lexer &) = delete;

    private:
        Lexer();

    public:
        static Lexer *getInstance()
        {
            if (_inst.get() == nullptr)
                _inst.reset(new Lexer);

            return _inst.get();
        }

        std::vector<std::shared_ptr<Token>> run(std::shared_ptr<File> file);

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
        std::shared_ptr<Token> makeGeneralToken(const Token &token) const;
        std::shared_ptr<Token> makeSpaceToken() const;
        std::shared_ptr<Token> makeEOFToken() const;
        std::shared_ptr<Token> makeNewlineToken() const;
        std::shared_ptr<Token> makeInvalidToken() const;
        std::shared_ptr<Token> makeIdentifierToken(std::string &buffer) const;
        std::shared_ptr<Token> makeKeywordToken(TokenType keywordType, std::string &buffer) const;
        std::shared_ptr<Token> makePunctuatorToken(TokenType punctuatorType) const;
        std::shared_ptr<Token> makeStringToken(std::string &buffer) const;
        std::shared_ptr<Token> makeNumberToken(TokenType type, std::string &buffer) const;
        std::shared_ptr<Token> makeCharToken(std::string &buffer);

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
        std::shared_ptr<File> _file;
        Position _curTokenPos;
        unsigned int _tokenCnt;
        std::unordered_map<std::string, TokenType> _keywordMap;
    };
}