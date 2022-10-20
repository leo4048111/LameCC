#include "lcc.hpp"

namespace cc
{
    std::unique_ptr<Lexer> Lexer::_inst;

    Lexer::Lexer()
    {
        #define keyword(name, disc) _keywordMap.insert(std::make_pair(disc, TokenType::name));
        #define operator(name, disc) 
        #define punctuator(name, disc)
        #include "TokenType.inc"
        #undef punctuator
        #undef operator
        #undef keyword
    }

    void Lexer::nextLine()
    {
        _file->nextLine();
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

        do
        {
            nextChar();
        } while (isSpace(peekChar()));
        
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
                if(c == '\n') nextLine();
                if(c == '/' && isLastStar) break;
                isLastStar = (c == '*');
            } while (c != EOF);
        }
        else retractChar();
    }

    std::vector<Token*> Lexer::run(File* file, const bool shouldDumpTokens, const std::string outPath)
    {
        _file = file;
        _tokenCnt = 0;
        _curTokenPos = _file->getPosition();
        nextLine();

        std::vector<Token*> tokens;
        Token* token = nullptr;

        do
        {
            token = nextToken();
            if(token->type == TokenType::TOKEN_INVALID)
                WARNING("Find invalid token at " << token->pos.line << ", " << token->pos.column);
            else if(token->type != TokenType::TOKEN_WHITESPACE && token->type != TokenType::TOKEN_NEWLINE)            
                tokens.push_back(token);
        } while (token->type != TokenType::TOKEN_EOF);

        if(shouldDumpTokens)
        {
            dumpJson(jsonifyTokens(tokens), outPath);
        }

        return tokens;
    }

    Token* Lexer::makeGeneralToken(const Token& token) const
    {
        Token* pToken = (Token*)malloc(sizeof(Token));
        ZeroMemory(pToken, sizeof(Token));
        memcpy(pToken, &token, sizeof(Token));
        pToken->pos = _curTokenPos;
        pToken->file = _file;
        pToken->count = _tokenCnt;
        return pToken;
    }

    Token* Lexer::makeSpaceToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_WHITESPACE;
        token.pContent = nullptr;
        return makeGeneralToken(token);
    }

    Token* Lexer::makeEOFToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_EOF;
        token.pContent = nullptr;
        return makeGeneralToken(token);
    }

    Token* Lexer::makeNewlineToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_NEWLINE;
        token.pContent = nullptr;
        return makeGeneralToken(token);
    }

    Token* Lexer::makeInvalidToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_INVALID;
        token.pContent = nullptr;
        return makeGeneralToken(token);
    }

    Token* Lexer::makeIdentifierToken(CharBuffer& buffer) const
    {
        Token token;
        token.type = TokenType::TOKEN_IDENTIFIER;
        token.pContent = buffer.new_c_str();
        return makeGeneralToken(token);
    }

    Token* Lexer::makeCharToken(CharBuffer& buffer)
    {
        Token token;
        token.type = TokenType::TOKEN_CHAR;
        token.pContent = buffer.new_c_str();
        return makeGeneralToken(token);
    }

    Token* Lexer::forwardSearch(const char possibleCh, TokenType possibleType, TokenType defaultType)
    {
        if(isNextChar(possibleCh)) return makePunctuatorToken(possibleType);

        return makePunctuatorToken(defaultType);
    }

    Token* Lexer::forwardSearch(const char possibleCh1, TokenType possibleType1, const char possibleCh2, TokenType possibleType2, TokenType defaultType)
    {
        if(isNextChar(possibleCh1)) return makePunctuatorToken(possibleType1);
        else if(isNextChar(possibleCh2)) return makePunctuatorToken(possibleType2);

        return makePunctuatorToken(defaultType);
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
                if(buffer == pair.first) return makeKeywordToken(pair.second, buffer);
            }

            return makeIdentifierToken(buffer);
        }
    }

    Token* Lexer::readString()
    {
        CharBuffer buffer;
        while(true)
        {
            char c = nextChar();
            if(c == '\"') break;
            else if(c == '\\') {
                c = nextChar();
            }
            buffer.append(c);
        }

        return makeStringToken(buffer);
    }

    Token* Lexer::readNumber(char c)
    {
        CharBuffer buffer;
        buffer.append(c);
        char last = c;
        while(true)
        {
            c = nextChar();
            bool isFloat = strchr("eEpP", last) && strchr("+-", c); // accepts scientific notation and p-notation(hexadecimal)
            if(!isalpha(c) && !isdigit(c) && !isFloat && c != '.')
            {
                retractChar();
                break;
            }
            buffer.append(c);
            last = c;
        }

        return makeNumberToken(buffer);
    }

    Token* Lexer::readChar()
    {
        CharBuffer buffer;
        while(true)
        {
            char c = nextChar();
            if(c == '\'') break;
            else if(c == '\\') c = nextChar();
            buffer.append(c);
        }

        return makeCharToken(buffer);
    }

    Token* Lexer::makeKeywordToken(TokenType keywordType, CharBuffer& buffer) const
    {
        Token token;
        token.type = keywordType;
        token.pContent = buffer.new_c_str();
        return makeGeneralToken(token);
    }

    Token* Lexer::makePunctuatorToken(TokenType punctuatorType) const
    {
        Token token;
        token.type = punctuatorType;
        token.pContent = nullptr;
        return makeGeneralToken(token);
    }

    Token* Lexer::makeStringToken(CharBuffer& buffer) const
    {
        Token token;
        token.type = TokenType::TOKEN_STRING;
        token.pContent = buffer.new_c_str();
        return makeGeneralToken(token);
    }

    Token* Lexer::makeNumberToken(CharBuffer& buffer) const
    {
        Token token;
        token.type = TokenType::TOKEN_NUMBER;
        token.pContent = buffer.new_c_str();
        return makeGeneralToken(token);
    }

    Token* Lexer::nextToken()
    {
        ignoreComments();
        _curTokenPos = _file->getPosition();
        if(ignoreSpaces()) return makeSpaceToken();
        char ch = nextChar();
        _tokenCnt++;
        switch(ch)
        {
        case '\n':
        {
            Token* token = makeNewlineToken();
            nextLine();
            return token;
        }
        case 'a' ... 'z': case 'A' ... 'Z': case '_': case '$':
        case 0x80 ... 0xFD:
            return readIdentifier(ch);
        case '0' ... '9':
            return readNumber(ch);
        case '.':
            if(isdigit(peekChar())) return readNumber(ch);
            return makePunctuatorToken(TokenType::TOKEN_PERIOD);
        case '=': return forwardSearch('=', TokenType::TOKEN_OPEQEQ, TokenType::TOKEN_OPEQ);
        case '<': return forwardSearch('=', TokenType::TOKEN_OPLEQ, TokenType::TOKEN_OPLESS);
        case '>': return forwardSearch('=', TokenType::TOKEN_OPGEQ, TokenType::TOKEN_OPGREATER);
        case '+': return makePunctuatorToken(TokenType::TOKEN_OPADD);
        case '-': return makePunctuatorToken(TokenType::TOKEN_OPMINUS);
        case '*': return makePunctuatorToken(TokenType::TOKEN_OPTIMES);
        case '/': return makePunctuatorToken(TokenType::TOKEN_OPDIV);
        case '(': return makePunctuatorToken(TokenType::TOKEN_LPAREN);
        case ')': return makePunctuatorToken(TokenType::TOKEN_RPAREN);
        case '{': return makePunctuatorToken(TokenType::TOKEN_LBRACE);
        case '}': return makePunctuatorToken(TokenType::TOKEN_RBRACE);
        case '[': return makePunctuatorToken(TokenType::TOKEN_LSQUARE);
        case ']': return makePunctuatorToken(TokenType::TOKEN_RSQUARE);
        case ';': return makePunctuatorToken(TokenType::TOKEN_SEMI);
        case '\"':
            return readString();
        case '\'':
            return readChar();
        case EOF: 
            return makeEOFToken();
        default:
            return makeInvalidToken();
        }
    }
}