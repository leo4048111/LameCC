#include "lcc.hpp"

namespace cc
{    
    bool isSpace(const char c)
    {
        return (c == ' ' || c == '\t' || c == '\f' || c == '\v');    
    }

    void freeToken(Token*& token)
    {
        if(token->pContent != nullptr) free((void*)token->pContent);
        free(token);
        token = nullptr;
    }

    json jsonifyTokens(const std::vector<Token*>& tokens)
    {
        json arr = json::array();
        for(auto& token : tokens)
        {
            json j;
            switch (token->type)
            {
                case TokenType::TOKEN_NEWLINE:
                    break;
                case TokenType::TOKEN_IDENTIFIER:
                    j["id"] = token->count;
                    j["type"] = "TOKEN_IDENTIFIER";
                    j["content"] = token->pContent;
                    j["position"] = {token->pos.line, token->pos.column};
                    break;
                case TokenType::TOKEN_EOF:
                    j["id"] = token->count;
                    j["type"] = "TOKEN_EOF";
                    j["content"] = "EOF";
                    j["position"] = {token->pos.line, token->pos.column};
                    break;
                case TokenType::TOKEN_WHITESPACE:
                    break;
                case TokenType::TOKEN_INVALID:
                    break;
                case TokenType::TOKEN_STRING:
                    j["id"] = token->count;
                    j["type"] = "TOKEN_STRING";
                    j["content"] = token->pContent;
                    j["position"] = {token->pos.line, token->pos.column};
                    break;
                case TokenType::TOKEN_NUMBER:
                    j["id"] = token->count;
                    j["type"] = "TOKEN_NUMBER";
                    j["content"] = token->pContent;
                    j["position"] = {token->pos.line, token->pos.column};
                    break;
                case TokenType::TOKEN_CHAR:
                    j["id"] = token->count;
                    j["type"] = "TOKEN_CHAR";
                    j["content"] = token->pContent;
                    j["position"] = {token->pos.line, token->pos.column};
                    break;
                #define keyword(name, disc) \
                case TokenType::name: \
                    j["id"] = token->count; \
                    j["type"] = #name; \
                    j["content"] = disc; \
                    j["position"] = {token->pos.line, token->pos.column}; \
                    break;
                #define punctuator(name, disc) keyword(name, disc)
                #include "TokenType.inc"
                #undef punctuator
                #undef keyword

                default:
                    break;
            }
            if(!j.empty()) arr.emplace_back(j);
        }
        return arr;
    }

    bool dumpJson(const json& j, const std::string outPath)
    {
        if(outPath.empty())
        {
            FATAL_ERROR("Dump file path not specified.");
            return false;
        }
            
        std::ofstream ofs(outPath);
        ofs << j.dump(4) << std::endl;
        ofs.close();
        return true;
    }
}