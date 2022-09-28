#include "lcc.hpp"

namespace cc
{
    Lexer::Lexer(File* file):
        _file(file)
    {
        #define keyword(name, disc) _keywordMap.insert(std::make_pair(disc, TokenType::name));
        #define operator(name, disc) 
        #include "TokenType.inc"
        #undef operator
        #undef keyword
    }

    const char Lexer::nextChar()
    {
        return _file->nextChar();
    }

    bool Lexer::isNextChar(const char c)
    {
        if(_file->peekChar() == c) 
        {
            nextChar();
            return true;
        }

        return false;
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
            bool isLastStar = false;
            char c = 0;
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
            case TokenType::TOKEN_KEYWORD:
                printf("row:%d, col:%d, %c\n", token->pos.line, token->pos.column, token->id);
                break;
            #define keyword(name, disc) \
            case TokenType::name: \
            printf("row:%d, col:%d, %s\n", token->pos.line, token->pos.column, disc); \
            break;
            #define operator(name, disc) keyword(name, disc)
            #include "TokenType.inc"
            #undef operator
            #undef keyword

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
        token.pchar = buffer.new_c_str();
        token.length = buffer.size();
        return makeGeneralToken(&token);
    }

    Token* Lexer::forwardSearch(const char possibleCh, TokenType possibleType, TokenType defaultType)
    {
        if(isNextChar(possibleCh)) return makeKeywordToken(possibleType);

        return makeKeywordToken(defaultType);
    }

    Token* Lexer::forwardSearch2(const char possibleCh1, TokenType possibleType1, const char possibleCh2, TokenType possibleType2, TokenType defaultType)
    {
        if(isNextChar(possibleCh1)) return makeKeywordToken(possibleType1);
        else if(isNextChar(possibleCh2)) return makeKeywordToken(possibleType2);

        return makeKeywordToken(defaultType);
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

            // determine whether this identifier is a keyword
            for(auto& pair: _keywordMap)
            {
                if(buffer == pair.first) return makeKeywordToken(pair.second);
            }

            return makeIdentifierToken(buffer);
        }
    }

    Token* Lexer::makeKeywordToken(TokenType keywordType) const
    {
        Token token;
        token.type = keywordType;
        return makeGeneralToken(&token);
    }

    Token* Lexer::makeKeywordToken(int id) const
    {
        Token token;
        token.type = TokenType::TOKEN_KEYWORD;
        token.id = id;
        return makeGeneralToken(&token);
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
        case '=': return forwardSearch('=', TokenType::TOKEN_OPEQ, TokenType::TOKEN_OPASSIGN);
        case '<': return forwardSearch('=', TokenType::TOKEN_OPLEQ, TokenType::TOKEN_OPLESS);
        case '>': return forwardSearch('=', TokenType::TOKEN_OPGEQ, TokenType::TOKEN_OPGREATER);
        case '+': return makeKeywordToken(TokenType::TOKEN_OPADD);
        case '-': return makeKeywordToken(TokenType::TOKEN_OPMINUS);
        case '*': return makeKeywordToken(TokenType::TOKEN_OPTIMES);
        case '/': return makeKeywordToken(TokenType::TOKEN_OPDIV);
        case '{': case '}': case '(': case ')':
            return makeKeywordToken((int)ch);
        case EOF: 
            return makeEOFToken();
        default:
            return makeInvalidToken();
        }
    }
}