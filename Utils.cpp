#include "lcc.hpp"

namespace cc
{    
    bool isSpace(const char c)
    {
        return (c == ' ' || c == '\t' || c == '\f' || c == '\v');    
    }

    json jsonifyToken(const Token& token)
    {
        json j;
        switch (token.type)
        {
        case TokenType::TOKEN_NEWLINE:
            break;
        case TokenType::TOKEN_IDENTIFIER:
            j["id"] = token.count;
            j["type"] = "TOKEN_IDENTIFIER";
            j["content"] = token.pchar;
            j["position"] = {token.pos.line, token.pos.column};
            break;
        case TokenType::TOKEN_EOF:
            break;
        case TokenType::TOKEN_WHITESPACE:
            break;
        case TokenType::TOKEN_INVALID:
            break;
        case TokenType::TOKEN_KEYWORD:
            j["id"] = token.count;
            j["type"] = "TOKEN_KEYWORD";
            j["content"] = (std::string("") + (char)token.id).c_str();
            j["position"] = {token.pos.line, token.pos.column};
            break;
        case TokenType::TOKEN_STRING:
        case TokenType::TOKEN_NUMBER:
            j["id"] = token.count;
            j["type"] = "TOKEN_NUMBER";
            j["content"] = token.pchar;
            j["position"] = {token.pos.line, token.pos.column};
            break;
        case TokenType::TOKEN_CHAR:
            j["id"] = token.count;
            j["type"] = "TOKEN_CHAR";
            j["content"] = token.pchar;
            j["position"] = {token.pos.line, token.pos.column};
            break;
        #define keyword(name, disc) \
        case TokenType::name: \
            j["id"] = token.count; \
            j["type"] = #name; \
            j["content"] = disc; \
            j["position"] = {token.pos.line, token.pos.column}; \
            break;
        #define operator(name, disc) keyword(name, disc)
        #include "TokenType.inc"
        #undef operator
        #undef keyword

        default:
            break;
        }

        return j;
    }
}