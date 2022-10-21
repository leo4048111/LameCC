#include "lcc.hpp"

#define TOKEN_INFO(pTok) pTok->file->path() << ' ' <<  pTok->pos.line << ", " << pTok->pos.column << ": "

namespace cc
{
    std::unique_ptr<Parser> Parser::_inst;

    void Parser::nextToken()
    {
        if(_curTokenIdx + 1 < _tokens.size()) _curTokenIdx++;
        _pCurToken = _tokens[_curTokenIdx];
    }

    std::unique_ptr<AST::TranslationUnitDecl> Parser::run(const std::vector<Token*>& tokens)
    {
        _tokens = tokens;
        _curTokenIdx = 0;
        _pCurToken = _tokens[_curTokenIdx];

        std::vector<std::unique_ptr<AST::Decl>> topLevelDecls;

        while(_curTokenIdx < _tokens.size())
        {
            switch(_pCurToken->type)
            {
                case TokenType::TOKEN_EOF:
                    return std::make_unique<AST::TranslationUnitDecl>(topLevelDecls);
                case TokenType::TOKEN_KWINT:
                case TokenType::TOKEN_KWVOID:
                case TokenType::TOKEN_KWFLOAT:
                case TokenType::TOKEN_KWCHAR:
                    topLevelDecls.push_back(genTopLevelDecl());
                    break;

                default:
                    FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Invalid top level token");
                    return nullptr;
            }
        }

        return std::make_unique<AST::TranslationUnitDecl>(topLevelDecls);
    }

    std::unique_ptr<AST::Decl> Parser::genTopLevelDecl()
    {
        std::string type = _pCurToken->pContent; // function return value type or var type
        nextToken();
        
        if(_pCurToken->type != TokenType::TOKEN_IDENTIFIER)
        {
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected identifier");
        }

        std::string name = _pCurToken->pContent; // function or var name
        nextToken();

        // parse top level function decl
        if(_pCurToken->type == TokenType::TOKEN_LPAREN)
        {
            Token* pLParen = _pCurToken;
            nextToken(); // eat '('

            // parse all funciton params
            std::vector<std::unique_ptr<AST::ParmVarDecl>> params;
            while(_pCurToken->type != TokenType::TOKEN_RPAREN)
            {
                switch(_pCurToken->type) // function return type check
                {
                    case TokenType::TOKEN_KWINT:
                    case TokenType::TOKEN_KWVOID:
                    case TokenType::TOKEN_KWFLOAT:
                    case TokenType::TOKEN_KWCHAR:
                    break;
                    default:
                        FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Unsupported return type.");
                        return nullptr;
                }

                std::string paramType = _pCurToken->pContent;
                std::string paramName;
                nextToken();
                // next token should be identifier
                if(_pCurToken->type == TokenType::TOKEN_IDENTIFIER)
                {
                    paramName = _pCurToken->pContent;
                    nextToken();
                }
                else 
                {
                    FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected parameter name");
                    return nullptr;
                }

                // next token is either comma(another param) or rbaren(end decl)
                if(_pCurToken->type == TokenType::TOKEN_COMMA) nextToken(); // eat ','
                else if(_pCurToken->type != TokenType::TOKEN_RPAREN)
                {
                    FATAL_ERROR(TOKEN_INFO(_pCurToken) << "No matching rparen found for lparen at " << pLParen->pos.line << ", " << pLParen->pos.column);
                    return nullptr;
                }

                params.push_back(std::make_unique<AST::ParmVarDecl>(paramName, paramType));
            }

            nextToken(); // eat '('

            // parse top level funciton body
            
        }

        return nullptr;
    }
}