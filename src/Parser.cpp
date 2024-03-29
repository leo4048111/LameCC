#include "lcc.hpp"

#define TOKEN_INFO(pTok) pTok->file->path() << ' ' << pTok->pos.line << ", " << pTok->pos.column << ": "

namespace lcc
{
    static AST::UnaryOpType TokenTypeToUnaryOpType(TokenType tokenType)
    {
        switch (tokenType)
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

    static AST::BinaryOpType TokenTypeToBinaryOpType(TokenType tokenType)
    {
        switch (tokenType)
        {
        case TokenType::TOKEN_PLUS:
            return AST::BinaryOpType::BO_Add;
        case TokenType::TOKEN_MINUS:
            return AST::BinaryOpType::BO_Sub;
        case TokenType::TOKEN_PLUSEQ:
            return AST::BinaryOpType::BO_AddAssign;
        case TokenType::TOKEN_MINUSEQ:
            return AST::BinaryOpType::BO_SubAssign;
        case TokenType::TOKEN_EXCLAIMEQ:
            return AST::BinaryOpType::BO_NE;
        case TokenType::TOKEN_EQEQ:
            return AST::BinaryOpType::BO_EQ;
        case TokenType::TOKEN_SLASH:
            return AST::BinaryOpType::BO_Div;
        case TokenType::TOKEN_SLASHEQ:
            return AST::BinaryOpType::BO_DivAssign;
        case TokenType::TOKEN_PERCENT:
            return AST::BinaryOpType::BO_Rem;
        case TokenType::TOKEN_PERCENTEQ:
            return AST::BinaryOpType::BO_RemAssign;
        case TokenType::TOKEN_STAR:
            return AST::BinaryOpType::BO_Mul;
        case TokenType::TOKEN_STAREQ:
            return AST::BinaryOpType::BO_MulAssign;
        case TokenType::TOKEN_LESS:
            return AST::BinaryOpType::BO_LT;
        case TokenType::TOKEN_LESSEQ:
            return AST::BinaryOpType::BO_LE;
        case TokenType::TOKEN_LESSLESS:
            return AST::BinaryOpType::BO_Shl;
        case TokenType::TOKEN_LESSLESSEQ:
            return AST::BinaryOpType::BO_ShlAssign;
        case TokenType::TOKEN_GREATER:
            return AST::BinaryOpType::BO_GT;
        case TokenType::TOKEN_GREATEREQ:
            return AST::BinaryOpType::BO_GE;
        case TokenType::TOKEN_GREATERGREATER:
            return AST::BinaryOpType::BO_Shr;
        case TokenType::TOKEN_GREATERGREATEREQ:
            return AST::BinaryOpType::BO_ShrAssign;
        case TokenType::TOKEN_EQ:
            return AST::BinaryOpType::BO_Assign;
        case TokenType::TOKEN_AMP:
            return AST::BinaryOpType::BO_And;
        case TokenType::TOKEN_AMPAMP:
            return AST::BinaryOpType::BO_LAnd;
        case TokenType::TOKEN_AMPEQ:
            return AST::BinaryOpType::BO_AndAssign;
        case TokenType::TOKEN_PIPE:
            return AST::BinaryOpType::BO_Or;
        case TokenType::TOKEN_PIPEPIPE:
            return AST::BinaryOpType::BO_LOr;
        case TokenType::TOKEN_PIPEEQ:
            return AST::BinaryOpType::BO_OrAssign;
        case TokenType::TOKEN_CARET:
            return AST::BinaryOpType::BO_Xor;
        case TokenType::TOKEN_CARETEQ:
            return AST::BinaryOpType::BO_XorAssign;
        default:
            return AST::BinaryOpType::BO_UNDEFINED;
        }
    };

    static AST::BinaryOperator::Precedence TokenTypeToBinaryOpPrecedence(TokenType tokenType)
    {
        switch (tokenType)
        {
        case TokenType::TOKEN_EQ:
        case TokenType::TOKEN_PLUSEQ:
        case TokenType::TOKEN_MINUSEQ:
        case TokenType::TOKEN_STAREQ:
        case TokenType::TOKEN_SLASHEQ:
        case TokenType::TOKEN_PERCENTEQ:
        case TokenType::TOKEN_LESSLESSEQ:
        case TokenType::TOKEN_GREATERGREATEREQ:
        case TokenType::TOKEN_AMPEQ:
        case TokenType::TOKEN_CARETEQ:
        case TokenType::TOKEN_PIPEEQ:
            return AST::BinaryOperator::Precedence::ASSIGNMENT;
        case TokenType::TOKEN_PIPEPIPE:
            return AST::BinaryOperator::Precedence::LOGICALOR;
        case TokenType::TOKEN_AMPAMP:
            return AST::BinaryOperator::Precedence::LOGICALAND;
        case TokenType::TOKEN_PIPE:
            return AST::BinaryOperator::Precedence::BITWISEOR;
        case TokenType::TOKEN_CARET:
            return AST::BinaryOperator::Precedence::BITWISEXOR;
        case TokenType::TOKEN_AMP:
            return AST::BinaryOperator::Precedence::BITWISEAND;
        case TokenType::TOKEN_EXCLAIMEQ:
        case TokenType::TOKEN_EQEQ:
            return AST::BinaryOperator::Precedence::EQUALITY;
        case TokenType::TOKEN_LESS:
        case TokenType::TOKEN_LESSEQ:
        case TokenType::TOKEN_GREATER:
        case TokenType::TOKEN_GREATEREQ:
            return AST::BinaryOperator::Precedence::RELATIONAL;
        case TokenType::TOKEN_LESSLESS:
        case TokenType::TOKEN_GREATERGREATER:
            return AST::BinaryOperator::Precedence::BITWISESHIFT;
        case TokenType::TOKEN_PLUS:
        case TokenType::TOKEN_MINUS:
            return AST::BinaryOperator::Precedence::ADDITIVE;
        case TokenType::TOKEN_SLASH:
        case TokenType::TOKEN_STAR:
        case TokenType::TOKEN_PERCENT:
            return AST::BinaryOperator::Precedence::MULTIPLICATIVE;
        default:
            return AST::BinaryOperator::Precedence::UNDEFINED;
        }
    };

    std::unique_ptr<Parser> Parser::_inst;

    void Parser::nextToken()
    {
        if (_curTokenIdx + 1 < _tokens.size())
            _curTokenIdx++;
        _pCurToken = _tokens[_curTokenIdx];
    }

    std::unique_ptr<AST::Decl> Parser::run(const std::vector<std::shared_ptr<Token>> &tokens)
    {
        _tokens = tokens;
        _curTokenIdx = 0;
        _pCurToken = _tokens[_curTokenIdx];

        std::vector<std::unique_ptr<AST::Decl>> topLevelDecls;

        while (_curTokenIdx < _tokens.size())
        {
            switch (_pCurToken->type)
            {
            case TokenType::TOKEN_EOF:
                return std::make_unique<AST::TranslationUnitDecl>(topLevelDecls);
            case TokenType::TOKEN_KWINT:
            case TokenType::TOKEN_KWVOID:
            case TokenType::TOKEN_KWFLOAT:
            case TokenType::TOKEN_KWCHAR:
            case TokenType::TOKEN_KWEXTERN:
                topLevelDecls.push_back(nextTopLevelDecl());
                break;

            default:
                FATAL_ERROR("Parsing failed, abort...");
                return nullptr;
            }
        }

        return std::make_unique<AST::TranslationUnitDecl>(topLevelDecls);
    }

    // FunctionDecl
    // ::= type name '(' params ')'
    // ::= type name '(' params ')' '{' CompoundStmt '}'
    // params
    // ::= ParmVarDecl, params
    std::unique_ptr<AST::Decl> Parser::nextFunctionDecl(const std::string name, const std::string type, const bool isExtern)
    {
        std::shared_ptr<Token> pLParen = _pCurToken;
        nextToken(); // eat '('

        // parse all function params
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;
        while (_pCurToken->type != TokenType::TOKEN_RPAREN)
        {
            switch (_pCurToken->type) // function return type check
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

            std::string paramType = _pCurToken->content;
            std::string paramName;
            nextToken(); // eat type
            
            // pointer type
            if(_pCurToken->type == TokenType::TOKEN_STAR)
            {
                paramType += '*'; 
                nextToken(); // eat *
            }

            // next token should be identifier
            if (_pCurToken->type == TokenType::TOKEN_IDENTIFIER)
            {
                paramName = _pCurToken->content;
                nextToken(); // eat name
            }
            else
            {
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected parameter name");
                return nullptr;
            }

            // next token is either comma(another param) or rbaren(end decl)
            if (_pCurToken->type == TokenType::TOKEN_COMMA)
                nextToken(); // eat ','
            else if (_pCurToken->type != TokenType::TOKEN_RPAREN)
            {
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "No matching rparen found for lparen at " << pLParen->pos.line << ", " << pLParen->pos.column);
                return nullptr;
            }

            params.push_back(std::make_unique<AST::ParmVarDecl>(paramName, paramType));
        }

        nextToken(); // eat ')'
        // parse top level function body
        std::unique_ptr<AST::Stmt> body = nullptr;
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_LBRACE: // function definition
            body = nextCompoundStmt();
            if (body == nullptr)
                return nullptr;
            if (_pCurToken->type == TokenType::TOKEN_SEMI)
            {
                WARNING(TOKEN_INFO(_pCurToken) << "Ignored ; after function definition");
                nextToken(); // eat ';'
            }
            break;
        case TokenType::TOKEN_SEMI: // function declaration
            nextToken();            // eat ';'
            break;
        default:
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ;");
            return nullptr;
        }

        return std::make_unique<AST::FunctionDecl>(name, type, params, std::move(body), isExtern);
    }

    // VarDecl
    // ::= '=' BinaryOperator ';'
    std::unique_ptr<AST::Decl> Parser::nextVarDecl(const std::string name, const std::string type)
    {
        nextToken(); // eat '='
        std::unique_ptr<AST::Expr> val = nextExpression();
        if (val == nullptr)
            return nullptr;
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_SEMI:
        {
            nextToken(); // eat ';'
            if (val->isLValue())
                val = std::make_unique<AST::ImplicitCastExpr>(std::move(val), AST::CastExpr::CastType::LValueToRValue);
            break;
        }
        default:
            return nullptr;
        }

        return std::make_unique<AST::VarDecl>(name, type, true, std::move(val));
    }

    std::unique_ptr<AST::Decl> Parser::nextTopLevelDecl()
    {
        auto possibleExtern = _pCurToken; // check if is extern decl
        bool isExternC = false;
        bool isExtern = false;
        if (possibleExtern->type == TokenType::TOKEN_KWEXTERN)
        {
            nextToken(); // eat kwextern
            isExtern = true;
            auto possibleExternCStr = _pCurToken;

            if (possibleExternCStr->type == TokenType::TOKEN_STRING)
            {
                nextToken(); // eat string
                if (possibleExternCStr->content == "C")
                    isExternC = true;
                else
                {
                    FATAL_ERROR("Unknown extern function type, expected \"C\"");
                    return nullptr;
                }
            }
        }

        std::string type = _pCurToken->content; // function return value type or var type
        nextToken();                            // eat type
        
        if(_pCurToken->type == TokenType::TOKEN_STAR) // pointer type
        {
            type += '*';
            nextToken(); // eat *
        }

        if (_pCurToken->type != TokenType::TOKEN_IDENTIFIER)
        {
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected identifier");
        }

        std::string name = _pCurToken->content; // function or var name
        nextToken();                            // eat name

        std::unique_ptr<AST::Decl> topLevelDecl = nullptr;

        // parse top level function decl
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_LPAREN:
            return nextFunctionDecl(name, type, isExtern);
        case TokenType::TOKEN_EQ:
            return nextVarDecl(name, type);
        case TokenType::TOKEN_SEMI: // uninitialized varDecl
            nextToken();            // eat ';'
            return std::make_unique<AST::VarDecl>(name, type);
        default:
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Unexpected top level declaration");
            return nullptr;
        }

        return nullptr;
    }

    std::unique_ptr<AST::Stmt> Parser::nextCompoundStmt()
    {
        std::shared_ptr<Token> pLBrace = _pCurToken;
        nextToken(); // eat '{'

        std::vector<std::unique_ptr<AST::Stmt>> body;
        while (_pCurToken->type != TokenType::TOKEN_RBRACE)
        {
            std::unique_ptr<AST::Stmt> stmt = nextStmt();
            if (stmt == nullptr)
                break;
            body.push_back(std::move(stmt));
        }

        if (_pCurToken->type != TokenType::TOKEN_RBRACE)
        {
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "No matching rbrace found for lbrace at " << pLBrace->pos.line << ", " << pLBrace->pos.column);
            return nullptr;
        }
        nextToken(); // eat '}'
        return std::make_unique<AST::CompoundStmt>(body);
    }

    std::unique_ptr<AST::Stmt> Parser::nextDeclStmt()
    {
        auto possibleExtern = _pCurToken; // check if is extern decl
        bool isExternC = false;
        bool isExtern = false;
        if (possibleExtern->type == TokenType::TOKEN_KWEXTERN)
        {
            nextToken(); // eat kwextern
            isExtern = true;
            auto possibleExternCStr = _pCurToken;

            if (possibleExternCStr->type == TokenType::TOKEN_STRING)
            {
                nextToken(); // eat string
                if (possibleExternCStr->content == "C")
                    isExternC = true;
                else
                {
                    FATAL_ERROR("Unknown extern function type, expected \"C\"");
                    return nullptr;
                }
            }
        }

        std::string type = _pCurToken->content; // function return value type or var type
        nextToken();                            // eat type

        if(_pCurToken->type == TokenType::TOKEN_STAR) // pointer type
        {
            type += '*';
            nextToken(); // eat *
        }

        if (_pCurToken->type != TokenType::TOKEN_IDENTIFIER)
        {
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected identifier");
        }

        std::vector<std::unique_ptr<AST::Decl>> decls;

        std::string name = _pCurToken->content; // function or var name
        nextToken();                            // eat name

        if(_pCurToken->type == TokenType::TOKEN_LSQUARE) // array type
        {
            nextToken(); // eat '['
            if(_pCurToken->type != TokenType::TOKEN_INTEGER)
            {
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected array size");
                return nullptr;
            }
            type += '[' + _pCurToken->content + ']';
            nextToken(); // eat arr size integer
            if(_pCurToken->type != TokenType::TOKEN_RSQUARE)
            {
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ]");
                return nullptr;
            }
            nextToken(); // eat ]
            nextToken(); // eat ;
            std::unique_ptr<AST::Decl> varDecl = std::make_unique<AST::VarDecl>(name, type);
            decls.push_back(std::move(varDecl));
            return std::make_unique<AST::DeclStmt>(decls); // array doesn't support {} initialization
        }

        if (isExtern && !isExternC)
            name = name; // FIXME convertion to cpp symbol name

        // TODO support comma declaration
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_LPAREN:
        {
            std::unique_ptr<AST::Decl> funcDecl = nextFunctionDecl(name, type, isExtern);
            decls.push_back(std::move(funcDecl));
            if (_pCurToken->type == TokenType::TOKEN_COMMA)
                nextToken(); // eat ','
            break;
        }
        case TokenType::TOKEN_EQ:
        {
            std::unique_ptr<AST::Decl> varDecl = nextVarDecl(name, type);
            decls.push_back(std::move(varDecl));
            if (_pCurToken->type == TokenType::TOKEN_COMMA)
                nextToken(); // eat ','
            break;
        }
        case TokenType::TOKEN_SEMI: // uninitialized varDecl
        case TokenType::TOKEN_COMMA:
        {
            if (_pCurToken->type == TokenType::TOKEN_COMMA)
                nextToken(); // eat ','
            std::unique_ptr<AST::Decl> varDecl = std::make_unique<AST::VarDecl>(name, type);
            decls.push_back(std::move(varDecl));
            break;
        }
        default:
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ;");
            return nullptr;
        }

        // nextToken(); // eat ';'
        return std::make_unique<AST::DeclStmt>(decls);
    }

    std::unique_ptr<AST::Stmt> Parser::nextReturnStmt()
    {
        nextToken(); // eat 'return'

        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_SEMI:
            nextToken(); // eat ';'
            return std::make_unique<AST::ReturnStmt>();
        default:
        {
            std::unique_ptr<AST::Expr> val = nextRValue();
            if (val == nullptr)
                return nullptr;
            if (_pCurToken->type != TokenType::TOKEN_SEMI)
            {
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ;");
                return nullptr;
            }
            nextToken(); // eat ';'
            return std::make_unique<AST::ReturnStmt>(std::move(val));
        }
        }
    }

    // ValueStmt
    // ::= Expr ';'
    std::unique_ptr<AST::Stmt> Parser::nextValueStmt()
    {
        std::unique_ptr<AST::Expr> expr = nextExpression();
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_SEMI:
            nextToken(); // eat ';'
            return std::make_unique<AST::ValueStmt>(std::move(expr));
        default:
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ;");
            return nullptr;
        }
    }

    std::unique_ptr<AST::Stmt> Parser::nextStmt()
    {
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_LBRACE:
            return nextCompoundStmt();
        case TokenType::TOKEN_KWWHILE:
            return nextWhileStmt();
        case TokenType::TOKEN_KWIF:
            return nextIfStmt();
        case TokenType::TOKEN_KWRETURN:
            return nextReturnStmt();
        case TokenType::TOKEN_SEMI:
            return nextNullStmt();
        case TokenType::TOKEN_KWINT:
        case TokenType::TOKEN_KWVOID:
        case TokenType::TOKEN_KWFLOAT:
        case TokenType::TOKEN_KWCHAR:
            return nextDeclStmt();
        case TokenType::TOKEN_INTEGER:
        case TokenType::TOKEN_FLOAT:
        case TokenType::TOKEN_LPAREN:
        case TokenType::TOKEN_IDENTIFIER:
            return nextValueStmt();
        case TokenType::TOKEN_KWASM: // only GCC asm dialect syntax is supported
            return nextAsmStmt();
        default:
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Unexpected statement");
            return nullptr;
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
        std::shared_ptr<Token> pLParen = _pCurToken;
        switch (_pCurToken->type) // lparen check
        {
        case TokenType::TOKEN_LPAREN:
            nextToken(); // eat '('
            break;
        default:
            FATAL_ERROR(TOKEN_INFO(pLParen) << "Unexpected token after if");
            return nullptr;
        }

        std::unique_ptr<AST::Expr> condition = nextRValue();
        if (condition == nullptr)
            return nullptr;

        switch (_pCurToken->type) // rparen check
        {
        case TokenType::TOKEN_RPAREN:
            nextToken(); // eat ')'
            break;
        default:
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "No matching ) for ( at " << pLParen->pos.line << ", " << pLParen->pos.column);
            return nullptr;
        }

        std::unique_ptr<AST::Stmt> body = nextStmt();
        if (body == nullptr)
            return nullptr;

        std::unique_ptr<AST::Stmt> elseBody = nullptr;
        if (_pCurToken->type == TokenType::TOKEN_KWELSE) // parse else body
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
        nextToken(); // eat 'while'
        std::shared_ptr<Token> pLParen = _pCurToken;
        switch (_pCurToken->type) // lparen check
        {
        case TokenType::TOKEN_LPAREN:
            nextToken(); // eat '('
            break;
        default:
            FATAL_ERROR(TOKEN_INFO(pLParen) << "Unexpected token after if");
            return nullptr;
        }
        std::unique_ptr<AST::Expr> condition = nextRValue();
        switch (_pCurToken->type) // rparen check
        {
        case TokenType::TOKEN_RPAREN:
            nextToken(); // eat ')'
            break;
        default:
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "No matching ) for ( at " << pLParen->pos.line << ", " << pLParen->pos.column);
            return nullptr;
        }
        std::unique_ptr<AST::Stmt> body = nextStmt();
        if (body == nullptr)
            return nullptr;

        return std::make_unique<AST::WhileStmt>(std::move(condition), std::move(body));
    }

    // VarRefOrFuncCall
    // ::= CallExpr '(' params ')'
    // ::= DeclRefExpr
    // ::= DeclRefExpr '[' Expr ']'
    // params
    // ::= Expr
    // ::= Expr ',' params
    std::unique_ptr<AST::Expr> Parser::nextVarRefOrFuncCall()
    {
        std::string name = _pCurToken->content;
        nextToken(); // eat name
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_LPAREN: // function call
        {
            std::shared_ptr<Token> pLParen = _pCurToken;
            nextToken(); // eat '('
            std::vector<std::unique_ptr<AST::Expr>> params;
            while (_pCurToken->type != TokenType::TOKEN_RPAREN)
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
            }

            nextToken(); // eat ')'
            return std::make_unique<AST::CallExpr>(std::make_unique<AST::DeclRefExpr>(name, true), params);
        }
        case TokenType::TOKEN_LSQUARE:
        {
            nextToken(); // eat '['
            std::unique_ptr<AST::Expr> index = nextRValue();
            switch (_pCurToken->type)
            {
            case TokenType::TOKEN_RSQUARE:
                nextToken(); // eat ']'
                return std::make_unique<AST::ArraySubscriptExpr>(name, std::make_unique<AST::DeclRefExpr>(name), std::move(index));
            default:
                FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Expected ] to match [ at " << _pCurToken->pos.line << ", " << _pCurToken->pos.column);
                return nullptr;
            }
        }
        default:
            return std::make_unique<AST::DeclRefExpr>(name);
        }
    }

    std::unique_ptr<AST::Expr> Parser::nextNumber()
    {
        std::string content = _pCurToken->content;
        if (_pCurToken->type == TokenType::TOKEN_INTEGER)
        {
            nextToken(); // eat integer
            return std::make_unique<AST::IntegerLiteral>(std::stoi(content));
        }
        else if(_pCurToken->type == TokenType::TOKEN_FLOAT)
        {
            nextToken(); // eat floating literal
            return std::make_unique<AST::FloatingLiteral>(std::stof(content));
        }
        else if(_pCurToken->type == TokenType::TOKEN_CHAR)
        {
            nextToken(); // eat char
            return std::make_unique<AST::CharacterLiteral>(content[0]);
        }
        else if(_pCurToken->type == TokenType::TOKEN_STRING)
        {
            nextToken(); // eat string
            return std::make_unique<AST::StringLiteral>(content);
        }
        else
        {
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Unexpected token");
            return nullptr;
        }
    }

    std::unique_ptr<AST::Expr> Parser::nextParenExpr()
    {
        std::shared_ptr<Token> pLParen = _pCurToken;
        nextToken(); // eat '('
        std::unique_ptr<AST::Expr> subExpr = nextExpression();
        if (subExpr == nullptr)
            return nullptr;
        switch (_pCurToken->type)
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
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_IDENTIFIER:
            return nextVarRefOrFuncCall();
        case TokenType::TOKEN_INTEGER:
        case TokenType::TOKEN_FLOAT:
        case TokenType::TOKEN_CHAR:
        case TokenType::TOKEN_STRING:
            return nextNumber();
        case TokenType::TOKEN_LPAREN:
            return nextParenExpr();
        case TokenType::TOKEN_MINUS:
        case TokenType::TOKEN_PLUS:
        case TokenType::TOKEN_EXCLAIM:
        case TokenType::TOKEN_TILDE:
        case TokenType::TOKEN_PLUSPLUS:
        case TokenType::TOKEN_MINUSMINUS:
            return nextUnaryOperator();

        default:
            FATAL_ERROR(TOKEN_INFO(_pCurToken) << "Unsupported expression");
            return nullptr;
        }
    }

    std::unique_ptr<AST::Expr> Parser::nextUnaryOperator()
    {
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_EXCLAIM:
        case TokenType::TOKEN_TILDE:
        case TokenType::TOKEN_PLUS:
        case TokenType::TOKEN_MINUS:
        case TokenType::TOKEN_PLUSPLUS:
        case TokenType::TOKEN_MINUSMINUS:
        {
            AST::UnaryOpType opType;
            switch (_pCurToken->type)
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

            if (opType == AST::UnaryOpType::UO_UNDEFINED) // unary op type check
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

    std::unique_ptr<AST::Expr> Parser::nextRHSExpr(std::unique_ptr<AST::Expr> lhs, AST::BinaryOperator::Precedence lastBiOpPrec)
    {
        do
        {
            AST::BinaryOperator::Precedence curBiOpPrec = TokenTypeToBinaryOpPrecedence(_pCurToken->type);
            if (curBiOpPrec < lastBiOpPrec)
                return lhs;

            AST::BinaryOpType opType = TokenTypeToBinaryOpType(_pCurToken->type);
            nextToken(); // eat current biOp
            std::unique_ptr<AST::Expr> rhs = nextPrimaryExpr();
            if (rhs == nullptr)
                return nullptr;

            AST::BinaryOperator::Precedence nextBiOpPrec = TokenTypeToBinaryOpPrecedence(_pCurToken->type);

            if (curBiOpPrec < nextBiOpPrec || (curBiOpPrec == AST::BinaryOperator::Precedence::ASSIGNMENT && curBiOpPrec == nextBiOpPrec))
                rhs = nextRHSExpr(std::move(rhs), (AST::BinaryOperator::Precedence)((int)curBiOpPrec + !(curBiOpPrec == AST::BinaryOperator::Precedence::ASSIGNMENT)));
            lhs = std::make_unique<AST::BinaryOperator>(opType, std::move(lhs), std::move(rhs));
        } while (true);

        return nullptr;
    }

    std::unique_ptr<AST::Expr> Parser::nextBinaryOperator()
    {
        std::unique_ptr<AST::Expr> lhs = nextUnaryOperator();
        if (lhs == nullptr)
            return nullptr;

        return nextRHSExpr(std::move(lhs), AST::BinaryOperator::Precedence::COMMA);
    }

    std::unique_ptr<AST::Expr> Parser::nextExpression()
    {
        return nextBinaryOperator();
    }

    std::unique_ptr<AST::Expr> Parser::nextRValue()
    {
        std::unique_ptr<AST::Expr> expr = nextExpression();
        if (expr->isLValue())
            expr = std::make_unique<AST::ImplicitCastExpr>(std::move(expr), AST::CastExpr::CastType::LValueToRValue);

        return expr;
    }

    //    asm ( assembler template
    //        : output operands                  /* optional */
    //        : input operands                   /* optional */
    //        : list of clobbered registers      /* optional */
    //        );
    std::unique_ptr<AST::Stmt> Parser::nextAsmStmt()
    {
        std::string asmStr = "";
        std::vector<std::pair<std::string, std::unique_ptr<AST::DeclRefExpr>>> outputConstraints;
        std::vector<std::pair<std::string, std::unique_ptr<AST::Expr>>> inputConstraints;
        std::vector<std::string> clbRegs;

        auto kwasm = _pCurToken;

        if (kwasm->type != TokenType::TOKEN_KWASM)
        {
            FATAL_ERROR(kwasm->pos.line << ", " << kwasm->pos.column << " unsupported statement.");
            return nullptr;
        }

        nextToken(); // eat TOKEN_KWASM

        auto lparen = _pCurToken;
        if (lparen->type != TokenType::TOKEN_LPAREN)
        {
            FATAL_ERROR(kwasm->pos.line << ", " << kwasm->pos.column << " expected (");
            return nullptr;
        }

        nextToken(); // eat (

        do
        {
            auto asmOpStrToken = _pCurToken;

            if (asmOpStrToken->type != TokenType::TOKEN_STRING)
            {
                FATAL_ERROR("Expected string literal in \'__asm__\'");
                return nullptr;
            }

            asmStr.append(asmOpStrToken->content);
            nextToken(); // eat asm string literal
        } while (_pCurToken->type == TokenType::TOKEN_STRING);

        if (_pCurToken->type == TokenType::TOKEN_COLON)
            nextToken(); // eat :
        else if (_pCurToken->type == TokenType::TOKEN_RPAREN)
        {
            nextToken(); // eat )
            goto ret;
        }

        // parse output operand
        while (_pCurToken->type != TokenType::TOKEN_COLON)
        {
            auto asmConstraintStr = _pCurToken;
            if (asmConstraintStr->type != TokenType::TOKEN_STRING)
            {
                FATAL_ERROR("Expected constraint string");
                return nullptr;
            }

            nextToken(); // eat constraint string

            auto lparen = _pCurToken;

            if (lparen->type != TokenType::TOKEN_LPAREN)
            {
                FATAL_ERROR("Expected (");
                return nullptr;
            }

            nextToken(); // eat (

            if (_pCurToken->type != TokenType::TOKEN_IDENTIFIER)
            {
                FATAL_ERROR(_pCurToken->pos.line << ", " << _pCurToken->pos.column << "Expected LValue");
                return nullptr;
            }

            auto tmpLValue = nextVarRefOrFuncCall();
            auto referencedLValue = dynamic_pointer_cast<AST::DeclRefExpr>(std::move(tmpLValue));

            auto rparen = _pCurToken;

            if (rparen->type != TokenType::TOKEN_RPAREN)
            {
                FATAL_ERROR("No matching rparen found for lparen at " << rparen->pos.line << ", " << rparen->pos.column);
                return nullptr;
            }

            nextToken(); // eat )

            if (_pCurToken->type == TokenType::TOKEN_COMMA)
                nextToken(); // eat ,

            outputConstraints.push_back(std::make_pair(asmConstraintStr->content, std::move(referencedLValue)));
        }

        if (_pCurToken->type == TokenType::TOKEN_COLON)
            nextToken(); // eat :
        else if (_pCurToken->type == TokenType::TOKEN_RPAREN)
        {
            nextToken(); // eat )
            goto ret;
        }

        // parse input operand
        while (_pCurToken->type != TokenType::TOKEN_COLON)
        {
            auto asmConstraintStr = _pCurToken;
            if (asmConstraintStr->type != TokenType::TOKEN_STRING)
            {
                FATAL_ERROR("Expected constraint string");
                return nullptr;
            }

            nextToken(); // eat constraint string

            auto lparen = _pCurToken;

            if (lparen->type != TokenType::TOKEN_LPAREN)
            {
                FATAL_ERROR("Expected (");
                return nullptr;
            }

            nextToken(); // eat (

            auto inputVal = nextRValue();

            auto rparen = _pCurToken;

            if (rparen->type != TokenType::TOKEN_RPAREN)
            {
                FATAL_ERROR("No matching rparen found for lparen at " << rparen->pos.line << ", " << rparen->pos.column);
                return nullptr;
            }

            nextToken(); // eat )

            if (_pCurToken->type == TokenType::TOKEN_COMMA)
                nextToken(); // eat ,

            inputConstraints.push_back(std::make_pair(asmConstraintStr->content, std::move(inputVal)));
        }

        if (_pCurToken->type == TokenType::TOKEN_COLON)
            nextToken(); // eat :
        else if (_pCurToken->type == TokenType::TOKEN_RPAREN)
        {
            nextToken(); // eat )
            goto ret;
        }

        while (_pCurToken->type != TokenType::TOKEN_RPAREN)
        {
            auto clobberedRegStr = _pCurToken;

            if (clobberedRegStr->type != TokenType::TOKEN_STRING)
            {
                FATAL_ERROR("Expected string literal to describe clobbered register");
                return nullptr;
            }

            nextToken(); // eat string literal

            if (_pCurToken->type == TokenType::TOKEN_COMMA)
                nextToken(); // eat ,

            clbRegs.push_back(clobberedRegStr->content);
        }

        if (_pCurToken->type != TokenType::TOKEN_RPAREN)
        {
            FATAL_ERROR("No matching rparen found for lparen at " << _pCurToken->pos.line << ", " << _pCurToken->pos.column);
            return nullptr;
        }
        nextToken(); // eat )

    ret:
        auto semi = _pCurToken;
        if (semi->type != TokenType::TOKEN_SEMI)
        {
            FATAL_ERROR("Missing ; at the end of asm statement");
            return nullptr;
        }

        nextToken(); // eat ;

        return std::make_unique<AST::AsmStmt>(asmStr, outputConstraints, inputConstraints, clbRegs);
    }

}