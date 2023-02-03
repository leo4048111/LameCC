#include "lcc.hpp"

namespace lcc
{
    std::unique_ptr<Lexer> Lexer::_inst;

    Lexer::Lexer()
    {
#define keyword(name, disc) _keywordMap.insert(std::make_pair(disc, TokenType::name));
#define punctuator(name, disc)
#include "TokenType.inc"
#undef punctuator
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
        if (_file->peekChar() == c)
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
        if (!isSpace(peekChar()))
            return false;

        do
        {
            nextChar();
        } while (isSpace(peekChar()));

        return true;
    }

    void Lexer::ignoreComments()
    {
        if (peekChar() != '/')
            return;

        nextChar();
        if (isNextChar('/')) // ignore line comment
        {
            char c = nextChar();
            while (c != '\n' && c != EOF)
                c = nextChar();
            retractChar();
        }
        else if (isNextChar('*')) // ignore block comment
        {
            bool isLastStar = false;
            char c = 0;
            do
            {
                c = nextChar();
                if (c == '\n')
                    nextLine();
                if (c == '/' && isLastStar)
                    break;
                isLastStar = (c == '*');
            } while (c != EOF);
        }
        else
            retractChar();
    }

    std::vector<std::shared_ptr<Token>> Lexer::run(std::shared_ptr<File> file)
    {
        _file = file;
        _tokenCnt = 0;
        _curTokenPos = _file->getPosition();
        nextLine();

        std::vector<std::shared_ptr<Token>> tokens;
        std::shared_ptr<Token> token = nullptr;

        do
        {
            token = nextToken();
            if (token->type == TokenType::TOKEN_INVALID)
                WARNING("Find invalid token at " << token->pos.line << ", " << token->pos.column);
            else if (token->type != TokenType::TOKEN_WHITESPACE && token->type != TokenType::TOKEN_NEWLINE)
                tokens.push_back(token);
        } while (token->type != TokenType::TOKEN_EOF);

        return tokens;
    }

    std::shared_ptr<Token> Lexer::makeGeneralToken(const Token &token) const
    {
        std::shared_ptr<Token> pToken = std::make_shared<Token>(token);
        pToken->pos = _curTokenPos;
        pToken->file = _file;
        pToken->count = _tokenCnt;
        return pToken;
    }

    std::shared_ptr<Token> Lexer::makeSpaceToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_WHITESPACE;
        token.content = "";
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::makeEOFToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_EOF;
        token.content = "";
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::makeNewlineToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_NEWLINE;
        token.content = "";
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::makeInvalidToken() const
    {
        Token token;
        token.type = TokenType::TOKEN_INVALID;
        token.content = "";
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::makeIdentifierToken(std::string &buffer) const
    {
        Token token;
        token.type = TokenType::TOKEN_IDENTIFIER;
        token.content = buffer;
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::makeCharToken(std::string &buffer)
    {
        Token token;
        token.type = TokenType::TOKEN_CHAR;
        token.content = buffer;
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::forwardSearch(const char possibleCh, TokenType possibleType, TokenType defaultType)
    {
        if (isNextChar(possibleCh))
            return makePunctuatorToken(possibleType);

        return makePunctuatorToken(defaultType);
    }

    std::shared_ptr<Token> Lexer::forwardSearch(const char possibleCh1, TokenType possibleType1, const char possibleCh2, TokenType possibleType2, TokenType defaultType)
    {
        if (isNextChar(possibleCh1))
            return makePunctuatorToken(possibleType1);
        else if (isNextChar(possibleCh2))
            return makePunctuatorToken(possibleType2);

        return makePunctuatorToken(defaultType);
    }

    std::shared_ptr<Token> Lexer::readIdentifier(char c)
    {
        std::string buffer;
        buffer += c;
        while (true)
        {
            c = nextChar();
            if (isalnum(c) || (c & 0x80) || c == '_' || c == '$') // numbders and ascii letters are accepted
            {
                buffer += c;
                continue;
            }
            retractChar();

            // determine whether this identifier is a keyword
            for (auto &pair : _keywordMap)
            {
                if (buffer == pair.first)
                    return makeKeywordToken(pair.second, buffer);
            }

            return makeIdentifierToken(buffer);
        }
    }

    std::shared_ptr<Token> Lexer::readString()
    {
        std::string buffer;
        while (true)
        {
            char c = nextChar();
            if (c == '\"')
                break;
            else if (c == '\\')
            {
                c = nextChar();
            }
            buffer += c;
        }

        return makeStringToken(buffer);
    }

    std::shared_ptr<Token> Lexer::readNumber(char c)
    {
        std::string buffer;
        buffer += c;
        char last = c;
        float mightBeFloat = false;
        while (true)
        {
            c = nextChar();
            bool isFloat = strchr("eEpP", last) && strchr("+-", c); // accepts scientific notation and p-notation(hexadecimal)

            if (isFloat)
                mightBeFloat = true;

            if (!isalpha(c) && !isdigit(c) && !isFloat && c != '.')
            {
                retractChar();
                break;
            }

            if (c == '.')
                mightBeFloat = true;

            buffer += c;
            last = c;
        }

        if (!mightBeFloat)
            return makeNumberToken(TokenType::TOKEN_INTEGER, buffer);
        else
            return makeNumberToken(TokenType::TOKEN_FLOAT, buffer);
    }

    std::shared_ptr<Token> Lexer::readChar()
    {
        std::string buffer;
        while (true)
        {
            char c = nextChar();
            if (c == '\'')
                break;
            else if (c == '\\')
                c = nextChar();
            buffer += c;
        }

        return makeCharToken(buffer);
    }

    std::shared_ptr<Token> Lexer::makeKeywordToken(TokenType keywordType, std::string &buffer) const
    {
        Token token;
        token.type = keywordType;
        token.content = buffer;
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::makePunctuatorToken(TokenType punctuatorType) const
    {
        Token token;
        token.type = punctuatorType;
        token.content = "";
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::makeStringToken(std::string &buffer) const
    {
        Token token;
        token.type = TokenType::TOKEN_STRING;
        token.content = buffer;
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::makeNumberToken(TokenType type, std::string &buffer) const
    {
        Token token;
        token.type = type;
        token.content = buffer;
        return makeGeneralToken(token);
    }

    std::shared_ptr<Token> Lexer::nextToken()
    {
        // preprocess
        ignoreComments();
        _curTokenPos = _file->getPosition();
        if (ignoreSpaces())
            return makeSpaceToken();
        char ch = nextChar();
        _tokenCnt++;
        switch (ch)
        {
        case '\n':
        {
            _tokenCnt--; // make sure newline isn't counted
            std::shared_ptr<Token> token = makeNewlineToken();
            nextLine();
            return token;
        }
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '_':
        case '$':
            return readIdentifier(ch);
        case '0' ... '9':
            return readNumber(ch);
        case '.':
            if (isdigit(peekChar()))
                return readNumber(ch);
            return makePunctuatorToken(TokenType::TOKEN_PERIOD);
        case '=':
            return forwardSearch('=', TokenType::TOKEN_EQEQ, TokenType::TOKEN_EQ);
        case '<':
        {
            if (isNextChar('<'))
                return forwardSearch('=', TokenType::TOKEN_LESSLESSEQ, TokenType::TOKEN_LESSLESS);

            return forwardSearch('=', TokenType::TOKEN_LESSEQ, TokenType::TOKEN_LESS);
        }
        case '>':
        {
            if (isNextChar('>'))
                return forwardSearch('=', TokenType::TOKEN_GREATERGREATEREQ, TokenType::TOKEN_GREATERGREATER);

            return forwardSearch('=', TokenType::TOKEN_GREATEREQ, TokenType::TOKEN_GREATER);
        }
        case '+':
            return forwardSearch('=', TokenType::TOKEN_PLUSEQ, '+', TokenType::TOKEN_PLUSPLUS, TokenType::TOKEN_PLUS);
        case '-':
            return forwardSearch('=', TokenType::TOKEN_MINUSEQ, '-', TokenType::TOKEN_MINUSMINUS, TokenType::TOKEN_MINUS);
        case '*':
            return forwardSearch('=', TokenType::TOKEN_STAREQ, TokenType::TOKEN_STAR);
        case '/':
            return forwardSearch('=', TokenType::TOKEN_SLASHEQ, TokenType::TOKEN_SLASH);
        case '%':
            return forwardSearch('=', TokenType::TOKEN_PERCENTEQ, TokenType::TOKEN_PERCENT);
        case '^':
            return forwardSearch('=', TokenType::TOKEN_CARETEQ, TokenType::TOKEN_CARET);
        case '|':
            return forwardSearch('=', TokenType::TOKEN_PIPEEQ, TokenType::TOKEN_PIPE);
        case '&':
            return forwardSearch('=', TokenType::TOKEN_AMPEQ, TokenType::TOKEN_AMP);
        case '(':
            return makePunctuatorToken(TokenType::TOKEN_LPAREN);
        case ')':
            return makePunctuatorToken(TokenType::TOKEN_RPAREN);
        case '{':
            return makePunctuatorToken(TokenType::TOKEN_LBRACE);
        case '}':
            return makePunctuatorToken(TokenType::TOKEN_RBRACE);
        case '[':
            return makePunctuatorToken(TokenType::TOKEN_LSQUARE);
        case ']':
            return makePunctuatorToken(TokenType::TOKEN_RSQUARE);
        case ';':
            return makePunctuatorToken(TokenType::TOKEN_SEMI);
        case ',':
            return makePunctuatorToken(TokenType::TOKEN_COMMA);
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