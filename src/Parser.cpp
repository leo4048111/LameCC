#include "lcc.hpp"

#define TOKEN_INFO(pTok) pTok->file->path() << ' ' <<  pTok->pos.line << ", " << pTok->pos.column << ": "

namespace cc
{
    static AST::UnaryOpType TokenTypeToUnaryOpType(TokenType tokenType)
    {
        switch(tokenType)
        {
            case TokenType::TOKEN_PLUS:
                return AST::UnaryOpType::UO_Plus;
            case TokenType::TOKEN_MINUS:
                return AST::UnaryOpType::UO_Minus;
            case TokenType::TOKEN_TILDE:
                return AST::UnaryOpType::UO_Not;
            case TokenType::TOKEN_EXCLAIM:
                return AST::UnaryOpType::UO_LNot;
            default:
                return AST::UnaryOpType::UO_UNDEFINED;
        }
    }

    std::unique_ptr<Parser> Parser::_inst;

    void Parser::nextToken()
    {
        if(_curTokenIdx + 1 < _tokens.size()) _curTokenIdx++;
        _pCurToken = _tokens[_curTokenIdx];
    }

    std::unique_ptr<AST::Decl> Parser::run(const std::vector<Token*>& tokens)
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
                    topLevelDecls.push_back(nextTopLevelDecl());
                    break;

                default:
                    FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Invalid top level token");
                    return nullptr;
            }
        }

        return std::make_unique<AST::TranslationUnitDecl>(topLevelDecls);
    }

    std::unique_ptr<AST::Decl> Parser::nextTopLevelDecl()
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

            nextToken(); // eat ')'
            // parse top level funciton body
            std::unique_ptr<AST::Stmt> body;
            switch (_pCurToken->type)
            {
            case TokenType::TOKEN_LBRACE: // function definition
                body = nextCompoundStmt();
                break;
            case TokenType::TOKEN_SEMI: // function declaration
                nextToken(); // eat ';'
                break;
            default:
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ; or function body");
                return nullptr;
            }

            return std::make_unique<AST::FunctionDecl>(name, type, params, std::move(body));
        }

        return nullptr;
    }

    std::unique_ptr<AST::Stmt> Parser::nextCompoundStmt()
    {
        Token* pLBrace = _pCurToken;
        nextToken(); // eat '{'

        std::vector<std::unique_ptr<AST::Stmt>> body;
        while(_pCurToken->type != TokenType::TOKEN_RBRACE)
        {
            std::unique_ptr<AST::Stmt> stmt = nextStmt();
            if(stmt == nullptr) break;
            body.push_back(std::move(stmt));
        }

        return std::make_unique<AST::CompoundStmt>(body);
    }

    std::unique_ptr<AST::Stmt> Parser::nextStmt()
    {
        switch(_pCurToken->type)
        {
            case TokenType::TOKEN_LBRACE:
                return nextCompoundStmt();
            case TokenType::TOKEN_KWWHILE:
                return nextWhileStmt();
            case TokenType::TOKEN_KWIF:
                return nextIfStmt();
            case TokenType::TOKEN_SEMI:
                return nextNullStmt();
                
        }
    }
    
    // NullStmt
    // ::= ';'
    std::unique_ptr<AST::Stmt> Parser::nextNullStmt()
    {
        nextToken(); // eat ';'
        return std::make_unique<AST::NullStmt>();
    }

    // IfStmt
    // ::= 'if' '(' Expr ')' Stmt
    // ::= 'if' '(' Expr ')' Stmt 'else' Stmt
    std::unique_ptr<AST::Stmt> Parser::nextIfStmt()
    {
        nextToken(); // eat 'if'
        Token* pLParen = _pCurToken;
        switch(_pCurToken->type) // lparen check
        {
            case TokenType::TOKEN_LPAREN:
                nextToken(); // eat '('
                break;
            default:
                FATAL_ERROR(TOKEN_INFO(pLParen) << "Unexpected token after if");
                return nullptr;
        }

        std::unique_ptr<AST::Expr> condition = nextRValue();
        if(condition == nullptr) return nullptr;

        switch(_pCurToken->type) // rparen check
        {
            case TokenType::TOKEN_RPAREN:
                nextToken(); // eat ')'
                break;
            default:
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "No matching ) for ( at " << pLParen->pos.line << ", " << pLParen->pos.column);
                return nullptr;
        }

        std::unique_ptr<AST::Stmt> body = nextStmt();
        if(body == nullptr) return nullptr;

        std::unique_ptr<AST::Stmt> elseBody = nullptr;
        if(_pCurToken->type == TokenType::TOKEN_KWELSE) // parse else body
        {
            nextToken(); // eat 'else'
            elseBody = nextStmt();
        }

        return std::make_unique<AST::IfStmt>(std::move(condition), std::move(body), std::move(elseBody));
    }

    // WhileStmt
    // ::= 'while' '(' Expr ')' Stmt
    std::unique_ptr<AST::Stmt> Parser::nextWhileStmt()
    {

    }

    // VarRefOrFuncCall
    // ::= CallExpr '(' params ')'
    // ::= DeclRefExpr
    // params
    // ::= Expr
    // ::= Expr ',' params
    std::unique_ptr<AST::Expr> Parser::nextVarRefOrFuncCall()
    {
        std::string name = _pCurToken->pContent;
        nextToken(); // eat name
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_LPAREN:
        {
            Token* pLParen = _pCurToken;
            nextToken(); // eat '('
            std::vector<std::unique_ptr<AST::Expr>> params;
            do
            {
                params.push_back(nextRValue());
                switch (_pCurToken->type)
                {
                case TokenType::TOKEN_COMMA:
                    nextToken(); // eat ','
                case TokenType::TOKEN_RPAREN:
                    break;
                default:
                    FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ) to match ( at " << pLParen->pos.line << ", " << pLParen->pos.column);
                    return nullptr;
                }
            }while(_pCurToken->type != TokenType::TOKEN_RPAREN);

            nextToken(); // eat ')'
            return std::make_unique<AST::CallExpr>(std::make_unique<AST::DeclRefExpr>(name), params);
        }
        default:
            return std::make_unique<AST::DeclRefExpr>(name);
        }
    }

    std::unique_ptr<AST::Expr> Parser::nextNumber()
    {
        // TODO float literal
        std::string number = _pCurToken->pContent;
        nextToken(); // eat number
        return std::make_unique<AST::IntegerLiteral>(std::stoi(number));
    }

    std::unique_ptr<AST::Expr> Parser::nextParenExpr()
    {
        Token* pLParen = _pCurToken;
        nextToken(); // eat '('
        std::unique_ptr<AST::Expr> subExpr = nextExpression();
        if(subExpr == nullptr) return nullptr;
        switch(_pCurToken->type)
        {
            case TokenType::TOKEN_RPAREN:
                nextToken(); // eat ')'
                return std::make_unique<AST::ParenExpr>(std::move(subExpr));
            default:
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ) to match ( at " << pLParen->pos.line << ", " << pLParen->pos.column);
                return nullptr;
        }
    }

    // PrimaryExpr
    // ::= VarRefOrFuncCall
    // ::= Number
    // ::= ParenExpr
    std::unique_ptr<AST::Expr> Parser::nextPrimaryExpr()
    {
        switch(_pCurToken->type)
        {
            case TokenType::TOKEN_IDENTIFIER:
                return nextVarRefOrFuncCall();
            case TokenType::TOKEN_NUMBER:
                return nextNumber();
            case TokenType::TOKEN_LPAREN:
                return nextParenExpr();
            default:
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Unsupported expression");
                return nullptr;
        }
    }

    std::unique_ptr<AST::Expr> Parser::nextUnaryOperator()
    {
        switch(_pCurToken->type)
        {
            case TokenType::TOKEN_EXCLAIM:
            case TokenType::TOKEN_TILDE:
            case TokenType::TOKEN_PLUS:
            case TokenType::TOKEN_MINUS:
            case TokenType::TOKEN_PLUSPLUS:
            case TokenType::TOKEN_MINUSMINUS:
            {
                AST::UnaryOpType opType;
                switch(_pCurToken->type)
                {
                    case TokenType::TOKEN_PLUSPLUS:
                        opType = AST::UnaryOpType::UO_PreInc;
                        break;
                    case TokenType::TOKEN_MINUSMINUS:
                        opType = AST::UnaryOpType::UO_PreDec;
                        break;
                    default:
                        opType = TokenTypeToUnaryOpType(_pCurToken->type);
                        break;
                }

                if(opType == AST::UnaryOpType::UO_UNDEFINED) // unary op type check
                {
                    FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Undefined unary operator");
                    return nullptr;
                }

                nextToken(); // eat current operator
                return std::make_unique<AST::UnaryOperator>(opType, nextUnaryOperator());
            }
            default:
            {
                std::unique_ptr<AST::Expr> body = nextPrimaryExpr();
                switch (_pCurToken->type)
                {
                case TokenType::TOKEN_PLUSPLUS:
                    nextToken(); // eat '++'
                    return std::make_unique<AST::UnaryOperator>(AST::UnaryOpType::UO_PostInc, std::move(body));
                case TokenType::TOKEN_MINUSMINUS:
                    nextToken(); // eat '--'
                    return std::make_unique<AST::UnaryOperator>(AST::UnaryOpType::UO_PostDec, std::move(body));
                default:
                    return std::move(body);
                }
            }
        }
    }

    std::unique_ptr<AST::Expr> Parser::nextBinaryOperator()
    {
        std::unique_ptr<AST::Expr> lhs = nextUnaryOperator();
        if(lhs == nullptr) return nullptr;


    }

    std::unique_ptr<AST::Expr> Parser::nextExpression()
    {
        return nextBinaryOperator();
    }

    std::unique_ptr<AST::Expr> Parser::nextRValue()
    {
        std::unique_ptr<AST::Expr> expr = nextExpression();
        if(expr->isLValue())
            expr = std::make_unique<AST::ImplicitCastExpr>(std::move(expr), "LValueToRValue");

        return expr;
    }
}