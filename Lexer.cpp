#include "cc.hpp"

namespace cc
{
    Lexer::Lexer(File* file):
        _file(file)
    {
        // TODO
    }

    const char Lexer::nextChar()
    {
        return _file->nextChar();
    }

    bool Lexer::isNextChar(const char c) const
    {
        return _file->peekChar() == c;
    }

    const char Lexer::peekChar()
    {
        return _file->peekChar();
    }

    void Lexer::retractChar()
    {
        _file->retractChar();
    }

    bool Lexer::ignoreSpaces()
    {
        if(!isSpace(peekChar()))
            return false;

        while(isSpace(nextChar()));
        retractChar();
        return true;
    }

    void Lexer::ignoreComments()
    {
        if(peekChar() != '/') return;

        nextChar();
        if(isNextChar('/')) // ignore line comment
        {
            char c = nextChar();
            while(c != '\n' && c != EOF) c = nextChar();
            retractChar();
        }
        else if(isNextChar('*')) // ignore block comment
        {
            char c = nextChar();
            bool isLastStar = false;
            do
            {
                c = nextChar();
                if(c == '/' && isLastStar) break;
                isLastStar = (c == '*');
            } while (c != EOF);
        }
        else retractChar();
    }

    void Lexer::run()
    {
        while(true)
        {
            Token* token = nextToken();
            switch (token->type)
            {
            case TokenType::TOKEN_NEWLINE:
                printf("row:%d, col:%d, newline\n", token->pos.line, token->pos.column);
                break;
            case TokenType::TOKEN_IDENTIFIER:
                printf("row:%d, col:%d, %s\n", token->pos.line, token->pos.column, token->pchar);
                break;
            case TokenType::TOKEN_EOF:
                printf("row:%d, col:%d, END\n", token->pos.line, token->pos.column);
                break;
            case TokenType::TOKEN_WHITESPACE:
                printf("row:%d, col:%d, WS\n", token->pos.line, token->pos.column);
                break;
            case TokenType::TOKEN_INVALID:
                printf("row:%d, col:%d, ?\n", token->pos.line, token->pos.column);
                break;
            default:
                break;
            }

            if(token->type == TokenType::TOKEN_EOF) break;
        }

        // while(true)
        // {
        //     Token* token = nextToken();
        //     switch (token->type)
        //     {
        //     case TokenType::TOKEN_NEWLINE:
        //         printf("\n");
        //         break;
        //     case TokenType::TOKEN_IDENTIFIER:
        //         printf("%s", token->pchar);
        //         break;
        //     case TokenType::TOKEN_EOF:
        //         printf("END\n");
        //         break;
        //     case TokenType::TOKEN_WHITESPACE:
        //         printf(" ");
        //         break;
        //     case TokenType::TOKEN_INVALID:
        //         printf("?");
        //         break;
        //     default:
        //         break;
        //     }

        //     if(token->type == TokenType::TOKEN_EOF) break;
        // }
    }

    Token* Lexer::makeGeneralToken(Token *token) const
    {
        Token* pToken = new Token(*token);
        pToken->pos = _curTokenPos;
        pToken->file = _file;
        pToken->count = _file->getTokenCnt();
        return pToken;
    }

    Token* Lexer::makeSpaceToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_WHITESPACE;
        return makeGeneralToken(&token);
    }

    Token* Lexer::makeEOFToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_EOF;
        return makeGeneralToken(&token);
    }

    Token* Lexer::makeNewlineToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_NEWLINE;
        return makeGeneralToken(&token);
    }

    Token* Lexer::makeInvalidToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_INVALID;
        return makeGeneralToken(&token);
    }

    Token* Lexer::makeIdentifierToken(CharBuffer& buffer) const
    {
        Token token;
        token.type = TokenType::TOKEN_IDENTIFIER;
        token.pchar = buffer.asCBuffer();
        token.length = buffer.size();
        return makeGeneralToken(&token);
    }

    Token* Lexer::readIdentifier(char c)
    {
        CharBuffer buffer;
        buffer.append(c);
        while(true)
        {
            c = nextChar();
            if(isalnum(c) || (c & 0x80) || c == '_' || c == '$') // numbders and ascii letters are accepted
            {
                buffer.append(c);
                continue;
            }
            retractChar();
            buffer.append('\0');
            return makeIdentifierToken(buffer);
        }
    }

    Token* Lexer::nextToken()
    {
        ignoreComments();
        _curTokenPos = _file->getPosition();
        if(ignoreSpaces()) return makeSpaceToken();
        char ch = nextChar();
        switch(ch)
        {
        case '\n':
            return makeNewlineToken();
        case 'a' ... 'z': case 'A' ... 'Z': case '_': case '$':
        case 0x80 ... 0xFD:
            return readIdentifier(ch);
        case EOF: 
            return makeEOFToken();
        default:
            return makeInvalidToken();
        }
    }
}