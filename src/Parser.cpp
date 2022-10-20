#include "lcc.hpp"

#define CURTOKEN_INFO _pCurToken->file->path() << ' ' <<  _pCurToken->pos.line << ", " << _pCurToken->pos.column << ": "

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
                    FATAL_ERROR(CURTOKEN_INFO << "Invalid top level token.");
                    return nullptr;
            }
        }

        return std::make_unique<AST::TranslationUnitDecl>(topLevelDecls);
    }

    std::unique_ptr<AST::Decl> Parser::genTopLevelDecl()
    {
        std::string type(_pCurToken->pContent);
        nextToken();
        
        if(_pCurToken->type != TokenType::TOKEN_IDENTIFIER)
        {
            FATAL_ERROR(CURTOKEN_INFO << "Expected identifier.");
        }

        return nullptr;
    }   
}