#include "lcc.hpp"

namespace cc
{
    std::unique_ptr<LR1Parser> LR1Parser::_inst;

    static std::string TokenTypeToSymbolName(TokenType type)
    {
        switch(type)
        {
        #define keyword(name, disc)
        #define punctuator(name, disc) case TokenType::name: return disc;
        #include "TokenType.inc"
        #undef punctuator
        #undef keyword
        case TokenType::TOKEN_KWINT:
        case TokenType::TOKEN_KWFLOAT:
        case TokenType::TOKEN_KWCHAR:
            return "TOKEN_VARTYPE";
        case TokenType::TOKEN_KWVOID: return "TOKEN_KWVOID";
        case TokenType::TOKEN_KWIF: return "TOKEN_KWIF";
        case TokenType::TOKEN_KWELSE: return "TOKEN_KWELSE";
        case TokenType::TOKEN_KWWHILE: return "TOKEN_KWWHILE";
        case TokenType::TOKEN_KWRETURN: return "TOKEN_KWRETURN";
        case TokenType::TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TokenType::TOKEN_NUMBER: return "TOKEN_NUMBER";
        case TokenType::TOKEN_CHAR: return "TOKEN_CHAR";
        case TokenType::TOKEN_STRING: return "TOKEN_STRING";
        case TokenType::TOKEN_EOF: return "#";
        default: return "";
        }

        return "";
    }

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

    static AST::BinaryOpType TokenTypeToBinaryOpType(TokenType tokenType)
    {
        switch(tokenType)
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
        switch(tokenType)
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

    LR1Parser::LR1Parser()
    {
        // for rn item, use function _productionFuncMap[n] to reduce
        _productionFuncMap.insert(std::make_pair(1, &nextStartSymbolR1));
        _productionFuncMap.insert(std::make_pair(2, &nextTranslationUnitDeclR2));
        _productionFuncMap.insert(std::make_pair(3, &nextTranslationUnitDeclR3));
        _productionFuncMap.insert(std::make_pair(4, &nextDecl));
        _productionFuncMap.insert(std::make_pair(5, &nextFunctionDeclR5));
        _productionFuncMap.insert(std::make_pair(6, &nextFunctionDeclR6));
        _productionFuncMap.insert(std::make_pair(7, &nextParmVarDeclR7));
        _productionFuncMap.insert(std::make_pair(8, &nextParmVarDeclR8));
        _productionFuncMap.insert(std::make_pair(9, &nextFunctionDeclR9));
        _productionFuncMap.insert(std::make_pair(10, &nextCompoundStmtR10));
        _productionFuncMap.insert(std::make_pair(11, &nextFunctionDeclR11));
        _productionFuncMap.insert(std::make_pair(12, &nextFunctionDeclR12));
        _productionFuncMap.insert(std::make_pair(13, &nextFunctionDeclR13));
        _productionFuncMap.insert(std::make_pair(14, &nextFunctionDeclR14));
        _productionFuncMap.insert(std::make_pair(15, &nextFunctionDeclR15));
        _productionFuncMap.insert(std::make_pair(16, &nextDecl));
        _productionFuncMap.insert(std::make_pair(17, &nextVarDeclR17));
        _productionFuncMap.insert(std::make_pair(18, &nextVarDeclR18));
    }

    std::unique_ptr<AST::Decl> LR1Parser::run(const std::vector<std::shared_ptr<Token>>& tokens, const std::string& productionFilePath)
    {
        parseProductionsFromJson(productionFilePath);
        findFirstSetForSymbols();
        constructCanonicalCollections();
        constructLR1ParseTable();

        auto root = parse(tokens);
        return root;
    }

    void LR1Parser::nextToken()
    {
        if(_curTokenIdx + 1 < _tokens.size()) _curTokenIdx++;
            _pCurToken = _tokens[_curTokenIdx];
    }

    std::unique_ptr<AST::Decl> LR1Parser::parse(const std::vector<std::shared_ptr<Token>>& tokens)
    {
        _tokens = tokens;
        _curTokenIdx = 0;
        _pCurToken = _tokens[_curTokenIdx];

        std::stack<int> stateStack;
        std::stack<std::shared_ptr<Symbol>> symbolStack;
        symbolStack.push(_endSymbol); // push '#'
        stateStack.push(0); // initial state 0

        do
        {
            auto actionTableRow = _actionTable[stateStack.top()];
            std::string symbolName = TokenTypeToSymbolName(_pCurToken->type);
            if(actionTableRow.find(symbolName) == actionTableRow.end()) {
                FATAL_ERROR("LR1Parser can't recognize " << symbolName);
                return nullptr;
            }
            Action action = actionTableRow[symbolName];
            
            switch (action.type)
            {
            case ActionType::INVALID:
                // check if there should be an expression, if positive then call our OperatorPrecedence Parser
                if(actionTableRow["Expr"].type != ActionType::INVALID) {
                    auto expr = nextExpr(); // parse next expression
                    if(expr != nullptr) { 
                        symbolStack.push(expr);
                        stateStack.push(actionTableRow["Expr"].id);
                        break;
                    }
                }
                // invalid action, abort asap
                FATAL_ERROR("Parsing failed at " << _pCurToken->pos.line << ", " << _pCurToken->pos.column);
                return nullptr;
            case ActionType::SHIFT:
                symbolStack.push(std::make_shared<Terminal>(symbolName, _pCurToken)); // push symbol
                stateStack.push(action.id); // push state
                nextToken();
                break;
            case ActionType::REDUCE:
            {
                auto nonTerminal = _productionFuncMap[action.id](stateStack, symbolStack);
                if(nonTerminal == nullptr) {
                    FATAL_ERROR("Internal LR1 parser error");
                    return nullptr;
                }
                symbolStack.push(nonTerminal); // push reduced nonterminal
                stateStack.push(_gotoTable[stateStack.top()][nonTerminal->name()]); // push new state
                break;
            }
            case ActionType::ACC:
            {
                auto result = dynamic_pointer_cast<NonTerminal>(symbolStack.top());
                return std::move(dynamic_pointer_cast<AST::Decl>(std::move(result->_node)));
            }
            default:
                return nullptr;
            }

        } while (symbolStack.top()->name() != "#");
        
        return nullptr;
    }

    // S -> TranslationUnitDecl
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextStartSymbolR1(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        stateStack.pop(); // pop state

        auto translationUnitDecl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce TranslationUnitDecl 
        symbolStack.pop();

        if(translationUnitDecl->name() != "TranslationUnitDecl") return nullptr;

        return std::make_shared<NonTerminal>("S", std::move(translationUnitDecl->_node));
    }
    
    // TranslationUnitDecl -> Decl
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextTranslationUnitDeclR2(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        stateStack.pop(); // pop state

        auto decl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce Decl
        symbolStack.pop();

        if(decl->name() != "Decl") return nullptr;

        std::vector<std::unique_ptr<AST::Decl>> decls;
        decls.push_back(dynamic_pointer_cast<AST::Decl>(std::move(decl->_node))); 
        auto translationUnitDecl = std::make_unique<AST::TranslationUnitDecl>(decls);
        return std::make_shared<NonTerminal>("TranslationUnitDecl", std::move(translationUnitDecl));
    }

    // TranslationUnitDecl -> Decl TranslationUnitDecl
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextTranslationUnitDeclR3(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 2; i++) stateStack.pop(); // pop 2 states

        auto translationUnitDecl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce TranslationUnitDecl
        symbolStack.pop();
        auto decl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce Decl
        symbolStack.pop();

        if(translationUnitDecl->name() != "TranslationUnitDecl" || decl->name() != "Decl") return nullptr;

        auto lastTranslationUnitDecl = dynamic_pointer_cast<AST::TranslationUnitDecl>(std::move(translationUnitDecl->_node));
        std::vector<std::unique_ptr<AST::Decl>> decls(std::move(lastTranslationUnitDecl->_decls));
        decls.push_back(dynamic_pointer_cast<AST::Decl>(std::move(decl->_node))); // push new decl
        auto curTranslationUnitDeclNode = std::make_unique<AST::TranslationUnitDecl>(decls);
        return std::make_shared<NonTerminal>("TranslationUnitDecl", std::move(curTranslationUnitDeclNode));
    }

    // Decl -> FunctionDecl
    // Decl -> VarDecl
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextDecl(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        stateStack.pop(); // pop state

        auto decl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce FunctionDecl
        symbolStack.pop();

        if(decl->name() != "FunctionDecl" && decl->name() != "VarDecl") return nullptr;

        return std::make_shared<NonTerminal>("Decl", std::move(decl->_node));
    }

    // FunctionDecl -> TOKEN_VARTYPE TOKEN_IDENTIFIER ( ) ;
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextFunctionDeclR5(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 5; i++) stateStack.pop(); // pop 5 states
        
        auto semi = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce ;
        symbolStack.pop();
        auto rparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce )
        symbolStack.pop();
        auto lparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce (
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvartype = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_VARTYPE
        symbolStack.pop();

        if(semi->name() != ";" || rparen->name() != ")" || lparen->name() != "(" || identifier->name() != "TOKEN_IDENTIFIER" || kwvartype->name() != "TOKEN_VARTYPE") return nullptr;

        std::string type = kwvartype->_token->content;
        std::string name = identifier->_token->content;
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;

        auto functionDecl = std::make_unique<AST::FunctionDecl>(name, type, params, nullptr);
        return std::make_shared<NonTerminal>("FunctionDecl", std::move(functionDecl));
    }

    // FunctionDecl -> TOKEN_VARTYPE TOKEN_IDENTIFIER ( ParmVarDecl ) ;
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextFunctionDeclR6(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 6; i++) stateStack.pop(); // pop 6 states

        auto semi = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce ;
        symbolStack.pop();
        auto rparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce )
        symbolStack.pop();
        auto parmVarDecl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce ParmVarDecl
        symbolStack.pop();
        auto lparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce (
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvartype = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_VARTYPE
        symbolStack.pop();

        if(semi->name() != ";" || rparen->name() != ")" || parmVarDecl->name() != "ParmVarDecl" ||lparen->name() != "(" || identifier->name() != "TOKEN_IDENTIFIER" || kwvartype->name() != "TOKEN_VARTYPE") return nullptr;

        std::string type = kwvartype->_token->content;
        std::string name = identifier->_token->content;
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;

        auto curParmVarDeclNode = dynamic_pointer_cast<AST::ParmVarDecl>(std::move(parmVarDecl->_node));

        while(curParmVarDeclNode != nullptr) {
            params.push_back(std::move(curParmVarDeclNode));
            curParmVarDeclNode = std::move(params.back()->_nextParmVarDecl);
        }

        auto functionDecl = std::make_unique<AST::FunctionDecl>(name, type, params, nullptr);
        return std::make_shared<NonTerminal>("FunctionDecl", std::move(functionDecl));
    }

    // ParmVarDecl -> TOKEN_VARTYPE TOKEN_IDENTIFIER
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextParmVarDeclR7(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 2; i++) stateStack.pop(); // pop 2 states

        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvartype = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_VARTYPE
        symbolStack.pop();

        if(identifier->name() != "TOKEN_IDENTIFIER" || kwvartype->name() != "TOKEN_VARTYPE") return nullptr;

        std::string type = kwvartype->_token->content;
        std::string name = identifier->_token->content;

        auto parmVarDecl = std::make_unique<AST::ParmVarDecl>(name, type);
        return std::make_shared<NonTerminal>("ParmVarDecl", std::move(parmVarDecl));
    }

    // ParmVarDecl -> TOKEN_VARTYPE TOKEN_IDENTIFIER , ParmVarDecl
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextParmVarDeclR8(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 4; i++) stateStack.pop(); // pop 4 states

        auto nextParmVarDecl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce ParmVarDecl
        symbolStack.pop();
        auto comma = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce ,
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvartype = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_VARTYPE
        symbolStack.pop();

        if(identifier->name() != "TOKEN_IDENTIFIER" || kwvartype->name() != "TOKEN_VARTYPE" || comma->name() != "," || nextParmVarDecl->name() != "ParmVarDecl") return nullptr;

        std::string type = kwvartype->_token->content;
        std::string name = identifier->_token->content;
        auto nextParmVarDeclNode = dynamic_pointer_cast<AST::ParmVarDecl>(std::move(nextParmVarDecl->_node));

        auto parmVarDecl = std::make_unique<AST::ParmVarDecl>(name, type, std::move(nextParmVarDeclNode));
        return std::make_shared<NonTerminal>("ParmVarDecl", std::move(parmVarDecl));
    }

    // FunctionDecl -> TOKEN_VARTYPE TOKEN_IDENTIFIER ( ) CompoundStmt
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextFunctionDeclR9(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 5; i++) stateStack.pop(); // pop 5 states

        auto compoundStmt = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce CompoundStmt
        symbolStack.pop();
        auto rparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce )
        symbolStack.pop();
        auto lparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce (
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvartype = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_VARTYPE
        symbolStack.pop();

        if(compoundStmt->name() != "CompoundStmt" || rparen->name() != ")" || lparen->name() != "(" || identifier->name() != "TOKEN_IDENTIFIER" || kwvartype->name() != "TOKEN_VARTYPE") return nullptr;
    
        std::string type = kwvartype->_token->content;
        std::string name = identifier->_token->content;
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;
        auto body = dynamic_pointer_cast<AST::CompoundStmt>(std::move(compoundStmt->_node));
        auto functionDecl = std::make_unique<AST::FunctionDecl>(name, type, params, std::move(body));
        return std::make_shared<NonTerminal>("FunctionDecl", std::move(functionDecl));
    }

    // CompoundStmt -> { }
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextCompoundStmtR10(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 2; i++) stateStack.pop(); // pop 2 states

        auto rbrace = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce }
        symbolStack.pop();
        auto lbrace = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce {
        symbolStack.pop();

        if(rbrace->name() != "}" || lbrace->name() != "{") return nullptr;

        std::vector<std::unique_ptr<AST::Stmt>> body;
        auto CompoundStmt = std::make_unique<AST::CompoundStmt>(body);
        return std::make_shared<NonTerminal>("CompoundStmt", std::move(CompoundStmt));
    }

    // FunctionDecl -> TOKEN_VARTYPE TOKEN_IDENTIFIER ( ParmVarDecl ) CompoundStmt
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextFunctionDeclR11(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 6; i++) stateStack.pop(); // pop 6 states

        auto compoundStmt = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce CompoundStmt
        symbolStack.pop();
        auto rparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce )
        symbolStack.pop();
        auto parmVarDecl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce ParmVarDecl
        symbolStack.pop();
        auto lparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce (
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvartype = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_VARTYPE
        symbolStack.pop();

        if(parmVarDecl->name() != "ParmVarDecl" || rparen->name() != ")" || parmVarDecl->name() != "ParmVarDecl" ||lparen->name() != "(" || identifier->name() != "TOKEN_IDENTIFIER" || kwvartype->name() != "TOKEN_VARTYPE") return nullptr;
    
        std::string type = kwvartype->_token->content;
        std::string name = identifier->_token->content;
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;

        auto curParmVarDeclNode = dynamic_pointer_cast<AST::ParmVarDecl>(std::move(parmVarDecl->_node));

        while(curParmVarDeclNode != nullptr) {
            params.push_back(std::move(curParmVarDeclNode));
            curParmVarDeclNode = std::move(params.back()->_nextParmVarDecl);
        }
        auto body = dynamic_pointer_cast<AST::CompoundStmt>(std::move(compoundStmt->_node));
        auto functionDecl = std::make_unique<AST::FunctionDecl>(name, type, params, std::move(body));
        
        return std::make_shared<NonTerminal>("FunctionDecl", std::move(functionDecl));
    }

    // FunctionDecl -> TOKEN_KWVOID TOKEN_IDENTIFIER ( ) ;
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextFunctionDeclR12(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 5; i++) stateStack.pop(); // pop 5 states
        
        auto semi = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce ;
        symbolStack.pop();
        auto rparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce )
        symbolStack.pop();
        auto lparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce (
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvoid = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_KWVOID
        symbolStack.pop();

        if(semi->name() != ";" || rparen->name() != ")" || lparen->name() != "(" || identifier->name() != "TOKEN_IDENTIFIER" || kwvoid->name() != "TOKEN_KWVOID") return nullptr;

        std::string type = kwvoid->_token->content;
        std::string name = identifier->_token->content;
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;

        auto functionDecl = std::make_unique<AST::FunctionDecl>(name, type, params, nullptr);
        return std::make_shared<NonTerminal>("FunctionDecl", std::move(functionDecl));
    }

    // FunctionDecl -> TOKEN_KWVOID TOKEN_IDENTIFIER ( ParmVarDecl ) ;
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextFunctionDeclR13(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 6; i++) stateStack.pop(); // pop 6 states

        auto semi = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce ;
        symbolStack.pop();
        auto rparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce )
        symbolStack.pop();
        auto parmVarDecl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce ParmVarDecl
        symbolStack.pop();
        auto lparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce (
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvoid = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_KWVOID
        symbolStack.pop();

        if(semi->name() != ";" || rparen->name() != ")" || parmVarDecl->name() != "ParmVarDecl" ||lparen->name() != "(" || identifier->name() != "TOKEN_IDENTIFIER" || kwvoid->name() != "TOKEN_KWVOID") return nullptr;

        std::string type = kwvoid->_token->content;
        std::string name = identifier->_token->content;
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;

        auto curParmVarDeclNode = dynamic_pointer_cast<AST::ParmVarDecl>(std::move(parmVarDecl->_node));

        while(curParmVarDeclNode != nullptr) {
            params.push_back(std::move(curParmVarDeclNode));
            curParmVarDeclNode = std::move(params.back()->_nextParmVarDecl);
        }

        auto functionDecl = std::make_unique<AST::FunctionDecl>(name, type, params, nullptr);
        return std::make_shared<NonTerminal>("FunctionDecl", std::move(functionDecl));
    }

    // FunctionDecl -> TOKEN_KWVOID TOKEN_IDENTIFIER ( ) CompoundStmt
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextFunctionDeclR14(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 5; i++) stateStack.pop(); // pop 5 states

        auto compoundStmt = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce CompoundStmt
        symbolStack.pop();
        auto rparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce )
        symbolStack.pop();
        auto lparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce (
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvoid = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_KWVOID
        symbolStack.pop();

        if(compoundStmt->name() != "CompoundStmt" || rparen->name() != ")" || lparen->name() != "(" || identifier->name() != "TOKEN_IDENTIFIER" || kwvoid->name() != "TOKEN_KWVOID") return nullptr;
    
        std::string type = kwvoid->_token->content;
        std::string name = identifier->_token->content;
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;
        auto body = dynamic_pointer_cast<AST::CompoundStmt>(std::move(compoundStmt->_node));
        auto functionDecl = std::make_unique<AST::FunctionDecl>(name, type, params, std::move(body));
        return std::make_shared<NonTerminal>("FunctionDecl", std::move(functionDecl));
    }

    // FunctionDecl -> TOKEN_KWVOID TOKEN_IDENTIFIER ( ParmVarDecl ) CompoundStmt
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextFunctionDeclR15(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 6; i++) stateStack.pop(); // pop 6 states

        auto compoundStmt = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce CompoundStmt
        symbolStack.pop();
        auto rparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce )
        symbolStack.pop();
        auto parmVarDecl = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce ParmVarDecl
        symbolStack.pop();
        auto lparen = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce (
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvoid = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_KWVOID
        symbolStack.pop();

        if(parmVarDecl->name() != "ParmVarDecl" || rparen->name() != ")" || parmVarDecl->name() != "ParmVarDecl" ||lparen->name() != "(" || identifier->name() != "TOKEN_IDENTIFIER" || kwvoid->name() != "TOKEN_KWVOID") return nullptr;
    
        std::string type = kwvoid->_token->content;
        std::string name = identifier->_token->content;
        std::vector<std::unique_ptr<AST::ParmVarDecl>> params;

        auto curParmVarDeclNode = dynamic_pointer_cast<AST::ParmVarDecl>(std::move(parmVarDecl->_node));

        while(curParmVarDeclNode != nullptr) {
            params.push_back(std::move(curParmVarDeclNode));
            curParmVarDeclNode = std::move(params.back()->_nextParmVarDecl);
        }
        auto body = dynamic_pointer_cast<AST::CompoundStmt>(std::move(compoundStmt->_node));
        auto functionDecl = std::make_unique<AST::FunctionDecl>(name, type, params, std::move(body));
        
        return std::make_shared<NonTerminal>("FunctionDecl", std::move(functionDecl));
    }

    // VarDecl -> TOKEN_VARTYPE TOKEN_IDENTIFIER ;
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextVarDeclR17(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 3; i++) stateStack.pop(); // pop 3 states

        auto semi = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce ;
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvartype = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_VARTYPE
        symbolStack.pop();

        if(semi->name() != ";" || identifier->name() != "TOKEN_IDENTIFIER" || kwvartype->name() != "TOKEN_VARTYPE") return nullptr;

        std::string type = kwvartype->_token->content;
        std::string name = identifier->_token->content;

        auto varDecl = std::make_unique<AST::VarDecl>(name, type);
        return std::make_shared<NonTerminal>("VarDecl", std::move(varDecl));
    }

    // VarDecl -> TOKEN_VARTYPE TOKEN_IDENTIFIER = Expr ;
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextVarDeclR18(std::stack<int>& stateStack, std::stack<std::shared_ptr<Symbol>>& symbolStack)
    {
        for(int i = 0; i < 5; i++) stateStack.pop(); // pop 5 states

        auto semi = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce ;
        symbolStack.pop();
        auto expr = std::dynamic_pointer_cast<NonTerminal>(symbolStack.top()); // reduce Expr
        symbolStack.pop();
        auto eq = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce =
        symbolStack.pop();
        auto identifier = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_IDENTIFIER
        symbolStack.pop();
        auto kwvartype = std::dynamic_pointer_cast<Terminal>(symbolStack.top()); // reduce TOKEN_VARTYPE
        symbolStack.pop();

        if(semi->name() != ";" || expr->name() != "Expr" || eq->name() != "=" || identifier->name() != "TOKEN_IDENTIFIER" || kwvartype->name() != "TOKEN_VARTYPE") return nullptr;

        std::string type = kwvartype->_token->content;
        std::string name = identifier->_token->content;
        auto exprNode = dynamic_pointer_cast<AST::Expr>(std::move(expr->_node));

        auto varDecl = std::make_unique<AST::VarDecl>(name, type, true, std::move(exprNode));
        return std::make_shared<NonTerminal>("VarDecl", std::move(varDecl));
    }

    // Expression parser imeplemented with OperatorPrecedence Parse
    std::shared_ptr<LR1Parser::NonTerminal> LR1Parser::nextExpr()
    {
        auto exprNode = nextExpression();
        if(exprNode == nullptr) return nullptr;

        return std::make_shared<NonTerminal>("Expr", std::move(exprNode));
    }

    std::unique_ptr<AST::Expr> LR1Parser::nextExpression()
    {
        std::unique_ptr<AST::Expr> lhs = nextUnaryOperator();
        if(lhs == nullptr) return nullptr;

        return nextRHSExpr(std::move(lhs), AST::BinaryOperator::Precedence::COMMA); 
    }

    std::unique_ptr<AST::Expr> LR1Parser::nextRHSExpr(std::unique_ptr<AST::Expr> lhs, AST::BinaryOperator::Precedence lastBiOpPrec)
    {
        do
        {
            AST::BinaryOperator::Precedence curBiOpPrec = TokenTypeToBinaryOpPrecedence(_pCurToken->type);
            if(curBiOpPrec < lastBiOpPrec) return lhs;

            AST::BinaryOpType opType = TokenTypeToBinaryOpType(_pCurToken->type);
            nextToken(); // eat current biOp
            std::unique_ptr<AST::Expr> rhs = nextPrimaryExpr();
            if(rhs == nullptr) return nullptr;

            AST::BinaryOperator::Precedence nextBiOpPrec = TokenTypeToBinaryOpPrecedence(_pCurToken->type);

            if(curBiOpPrec < nextBiOpPrec || (curBiOpPrec == AST::BinaryOperator::Precedence::ASSIGNMENT && curBiOpPrec == nextBiOpPrec))
                rhs = nextRHSExpr(std::move(rhs), (AST::BinaryOperator::Precedence)((int)curBiOpPrec + !(curBiOpPrec == AST::BinaryOperator::Precedence::ASSIGNMENT)));
            lhs = std::make_unique<AST::BinaryOperator>(opType, std::move(lhs), std::move(rhs));
        }while(true);

        return nullptr;
    }

    std::unique_ptr<AST::Expr> LR1Parser::nextUnaryOperator()
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

    std::unique_ptr<AST::Expr> LR1Parser::nextRValue()
    {
        std::unique_ptr<AST::Expr> expr = nextExpression();
        if(expr->isLValue())
            expr = std::make_unique<AST::ImplicitCastExpr>(std::move(expr), "LValueToRValue");

        return expr;
    }

    // PrimaryExpr
    // ::= VarRefOrFuncCall
    // ::= Number
    // ::= ParenExpr
    std::unique_ptr<AST::Expr> LR1Parser::nextPrimaryExpr()
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
            return nullptr;
        }
    }

    // VarRefOrFuncCall
    // ::= CallExpr '(' params ')'
    // ::= DeclRefExpr
    // params
    // ::= Expr
    // ::= Expr ',' params
    std::unique_ptr<AST::Expr> LR1Parser::nextVarRefOrFuncCall()
    {
        std::string name = _pCurToken->content;
        nextToken(); // eat name
        switch (_pCurToken->type)
        {
        case TokenType::TOKEN_LPAREN:
        {
            std::shared_ptr<Token> pLParen = _pCurToken;
            nextToken(); // eat '('
            std::vector<std::unique_ptr<AST::Expr>> params;
            while(_pCurToken->type != TokenType::TOKEN_RPAREN)
            {
                params.push_back(nextRValue());
                switch (_pCurToken->type)
                {
                case TokenType::TOKEN_COMMA:
                    nextToken(); // eat ','
                case TokenType::TOKEN_RPAREN:
                    break;
                default:
                    return nullptr;
                }
            }

            nextToken(); // eat ')'
            return std::make_unique<AST::CallExpr>(std::make_unique<AST::DeclRefExpr>(name, true), params);
        }
        default:
            return std::make_unique<AST::DeclRefExpr>(name);
        }
    }

    std::unique_ptr<AST::Expr> LR1Parser::nextNumber()
    {
        // TODO float literal
        std::string number = _pCurToken->content;
        nextToken(); // eat number
        return std::make_unique<AST::IntegerLiteral>(std::stoi(number));
    }

    std::unique_ptr<AST::Expr> LR1Parser::nextParenExpr()
    {
        std::shared_ptr<Token> pLParen = _pCurToken;
        nextToken(); // eat '('
        std::unique_ptr<AST::Expr> subExpr = nextExpression();
        if(subExpr == nullptr) return nullptr;
        switch(_pCurToken->type)
        {
        case TokenType::TOKEN_RPAREN:
            nextToken(); // eat ')'
            return std::make_unique<AST::ParenExpr>(std::move(subExpr));
        default:
            return nullptr;
        }
    }

    void LR1Parser::parseProductionsFromJson(const std::string& productionFilePath)
    {
        std::ifstream i(productionFilePath);
        json j;
        i >> j;
        i.close();

        for(auto& symbol : j["NonTerminals"]) _nonTerminals.insert(std::make_shared<NonTerminal>(symbol.get<std::string>()));
        for(auto& symbol : j["Terminals"]) _terminals.insert(std::make_shared<Terminal>(symbol.get<std::string>()));

        _endSymbol = std::make_shared<Terminal>("#");
        _epsilonSymbol = std::make_shared<Terminal>("$");
        _terminals.insert(_endSymbol);
        _extensiveStartSymbol = std::make_shared<NonTerminal>("S'");

        for(auto& production : j["Productions"])
        {
            Production p;
            p.id = production["id"].get<int>();
            std::string lhs = production["lhs"].get<std::string>();
            if(lhs == "S'") p.lhs = _extensiveStartSymbol;
            else
            {
                for(auto& nonTerminal : _nonTerminals)
                {
                    if(nonTerminal->name() == lhs) {
                        p.lhs = nonTerminal;
                        break;
                    }
                }
            }

            for(auto& rhsSymbol : production["rhs"])
            {
                std::string rhs = rhsSymbol.get<std::string>();
                bool hasFound = false;
                if(rhs == "$")
                {
                    p.rhs.push_back(_epsilonSymbol);
                    hasFound = true;
                }

                if(!hasFound)
                    for(auto& nonTerminal : _nonTerminals)
                    {
                        if(nonTerminal->name() == rhs) {
                            p.rhs.push_back(nonTerminal);
                            hasFound = true;
                            break;
                        }
                    }

                if(!hasFound)
                    for(auto& terminal : _terminals)
                    {
                        if(terminal->name() == rhs) {
                            p.rhs.push_back(terminal);
                            break;
                        }
                    }
            }

            _productions[p.lhs->name()].push_back(p);
        }
    }

    void LR1Parser::findFirstSetForSymbols()
    {
        // for all terminals, their first sets are themselves
        for(auto& symbol : _terminals)
            _first[symbol->name()].insert(symbol);

        _first[_endSymbol->name()].insert(_endSymbol);

        bool shouldBail = true;
        do
        {
            shouldBail = true;
            // traverse every production, check whether first symbol is a terminal
            for(auto& pair : _productions)
            {
                for(auto& production : pair.second)
                {
                    auto& curLhsFirst = _first[production.lhs->name()];
                    // if the first symbol is a terminal, add to FIRST(lhs)
                    if(isTerminal(production.rhs[0]) || isEpsilon(production.rhs[0])) 
                    {
                        if(doInsert(curLhsFirst, production.rhs[0])) shouldBail = false;
                    }
                    else // if current symbol is a nonTerminal, and it produces epsilon, then add its FIRST set to FIRST(lhs)
                    {
                        int curRhsIdx = 0;
                        bool shouldTraverseNext = true;
                        while(shouldTraverseNext && curRhsIdx < production.rhs.size())
                        {
                            shouldTraverseNext = false;
                            std::shared_ptr<Symbol> curRhs = production.rhs[curRhsIdx];
                            auto& curRhsFirst = _first[curRhs->name()];

                            for(auto& symbol : curRhsFirst)
                            {
                                if(isEpsilon(symbol)) { // epsilon shouldn't be added to FIRST(lhs)
                                    shouldTraverseNext = true;
                                    curRhsIdx++;
                                }
                                else {
                                    if(doInsert(curLhsFirst, symbol)) shouldBail = false;
                                }
                            }
                        }
                    }
                }
            }
        } while (!shouldBail);


        // for(auto& symbol : _terminals)
        // {
        //     std::cout << symbol->name() << ": ";
        //     auto& first = _first[symbol->name()];
        //     for(auto & s : first) std::cout << s->name() << " ";
        //     std::cout << std::endl;
        // }

        // for(auto& symbol : _nonTerminals)
        // {
        //     std::cout << symbol->name() << ": ";
        //     auto& first = _first[symbol->name()];
        //     for(auto & s : first) std::cout << s->name() << " ";
        //     std::cout << std::endl;
        // }
    }

    void LR1Parser::constructCanonicalCollections()
    {
        LR1Item firstItem = {_productions["S'"][0], 0, _endSymbol}; // S' -> .S

        LR1ItemSet I0;
        I0.id = 0;
        I0.items.insert(firstItem);

        closure(I0); // calculate CLOSURE(I0)
        _canonicalCollections.push_back(I0); // add to canonical collections 

        std::queue<int> qSets;
        qSets.push(I0.id);
        while(!qSets.empty())
        {
            int InID = qSets.front();
            qSets.pop();
            LR1ItemSet In = _canonicalCollections[InID];
            std::map<std::string, bool> isVisited;
            for(auto& item : In.items)
            {
                if(item.dotPos < item.production.rhs.size()) // if there is a symbol after dot
                {
                    auto symbol = item.production.rhs[item.dotPos];
                    if(isEpsilon(symbol) || isVisited[symbol->name()]) continue;
                    else isVisited[symbol->name()] = true;
                    auto newItemSet = go(In, symbol);
                    newItemSet.id = _canonicalCollections.size();

                    bool isFound = false;
                    for(auto& I : _canonicalCollections) // duplication check
                    {
                        if(I == newItemSet) {
                            newItemSet.id = I.id;
                            isFound = true;
                            break;
                        }
                    }
                    if(!isFound) { // if this is not a duplicated itemset
                        if(_canonicalCollections.size() <= newItemSet.id)
                            _canonicalCollections.resize(newItemSet.id + 1);
                        _canonicalCollections[newItemSet.id] = newItemSet;
                        qSets.push(newItemSet.id);
                    }
                    In.transitions.insert(std::make_pair(symbol, newItemSet.id));
                }
            }
            _canonicalCollections[InID] = In;
        }

        // debug print
        //for(auto& In : _canonicalCollections) printItemSet(In);
    }

    void LR1Parser::constructLR1ParseTable()
    {
        _actionTable.resize(_canonicalCollections.size());
        _gotoTable.resize(_canonicalCollections.size());

        for(auto& In : _canonicalCollections) // for every item set
        {
            // init table rows
            Action initialAction = {ActionType::INVALID, -1};
            for(auto& terminal : _terminals) _actionTable[In.id].insert(std::make_pair(terminal->name(), initialAction));
            for(auto& nonTerminal : _nonTerminals) _gotoTable[In.id].insert(std::make_pair(nonTerminal->name(), -1));

            for(auto& transition : In.transitions) {
                if(isTerminal(transition.first)) // if symbol is terminal, update ACTION
                {
                    _actionTable[In.id][transition.first->name()].type = ActionType::SHIFT;
                    _actionTable[In.id][transition.first->name()].id = transition.second;
                }
                else // if symbol is nonterminal, update GOTO
                {
                    _gotoTable[In.id][transition.first->name()] = transition.second;
                }
            }

            for(auto& item : In.items)
            {
                if(item.dotPos >= item.production.rhs.size()) // if the item is a reduce item
                {
                    if(item.production.id != 1)
                    {
                        _actionTable[In.id][item.lookahead->name()].type = ActionType::REDUCE;
                        _actionTable[In.id][item.lookahead->name()].id = item.production.id;
                    }
                    else  _actionTable[In.id][item.lookahead->name()].type = ActionType::ACC;
                }
            }
        }

        // debug print
        //printActionAndGotoTable();
    }

    void LR1Parser::closure(LR1ItemSet& itemSet)
    {
        bool shouldBail = true;
        do
        {
            shouldBail = true;
            for(auto& item : itemSet.items)
            {
                if(item.dotPos < item.production.rhs.size()) // if current LR1Item is a shift item
                {
                    auto B = item.production.rhs[item.dotPos];
                    if(isNonTerminal(B)) // A -> a.Bb
                    {
                        std::set<std::shared_ptr<Symbol>, SharedPtrComp> followB;
                        int idx = item.dotPos + 1;
                        bool canNextSymbolProduceEpsilon = false;
                        do // find FOLLOW(B) from current production
                        {
                            canNextSymbolProduceEpsilon = false;
                            if(idx >= item.production.rhs.size()) followB.insert(item.lookahead); // No symbols after B
                            else {
                                auto& nextSymbol = item.production.rhs[idx];
                                if(isTerminal(nextSymbol) || isEpsilon(nextSymbol)) followB.insert(nextSymbol); // Next symbol is terminal
                                else // next symbol is NonTerminal
                                {
                                    for(auto& symbol : _first[nextSymbol->name()])
                                    {
                                        if(isEpsilon(symbol)) canNextSymbolProduceEpsilon = true; // next symbol can produce epsilon
                                        else followB.insert(symbol);
                                    }
                                }
                            }
                            idx++;
                        } while (canNextSymbolProduceEpsilon);
                        
                        for(auto& production : _productions[B->name()]) // for every production whose lhs is B, eg. B -> ...
                        {
                            for(auto& lookAheadSymbol : followB)
                            {
                                LR1Item newItem = {production, 0, lookAheadSymbol};
                                if(doInsert(itemSet.items, newItem)) shouldBail = false;
                            }
                        }
                    }
                }
            }
        } while (!shouldBail);
    }

    LR1Parser::LR1ItemSet LR1Parser::go(LR1ItemSet& itemSet, std::shared_ptr<Symbol> symbol)
    {
        LR1ItemSet newItemSet;
        for(auto& item : itemSet.items)
        {
            if(item.dotPos < item.production.rhs.size()) // if there is a symbol after dot
            {
                auto s = item.production.rhs[item.dotPos];
                if(s->name() == symbol->name())
                {
                    LR1Item newItem = item;
                    newItem.dotPos++;
                    newItemSet.items.insert(newItem);
                }
            }
        }

        closure(newItemSet); // calculate CLOSURE(In) before go returns
        return newItemSet;
    }

    void LR1Parser::printItemSet(LR1ItemSet& itemSet) const
    {
        printf("I%d: \n", itemSet.id);
        for(auto& item : itemSet.items)
        {
            printProduction(item.production, true, item.dotPos);
            printf("    %s", item.lookahead->name().c_str());
            printf("\n");
        }

        for(auto& transition : itemSet.transitions)
            printf("I%d takes %s goto I%d\n", itemSet.id, transition.first->name().c_str(), transition.second);
    }

    void LR1Parser::printProduction(const Production& production, bool shouldAddDot, int dotPos) const
    {
        printf("%s ->", production.lhs->name().c_str());
        for(auto& symbol : production.rhs)
        {
            if(shouldAddDot && (dotPos == 0)) printf(" .");
            printf(" %s", symbol->name().c_str());
            dotPos--;
        }
        if(shouldAddDot && (dotPos == 0)) printf(" .");
    }

    void LR1Parser::printActionAndGotoTable() const
    {
        const int spacing = 20;
        printf("%-*s", spacing, "ACTION");
        for(auto& terminal : _terminals)
            printf("%-*s", spacing, terminal->name().c_str());

        printf("%-*s", spacing, "GOTO");
        for(auto& nonTerminal : _nonTerminals)
            printf("%-*s", spacing, nonTerminal->name().c_str());
        printf("\n");
        for(auto& In : _canonicalCollections)
        {
            printf("%-*d", spacing, In.id);
            std::map<std::string, Action> firstTableLine = _actionTable[In.id];
            std::map<std::string, int> gotoTableLine = _gotoTable[In.id];
            for(auto& terminal : _terminals)
            {
                Action action = firstTableLine[terminal->name()];
                switch (action.type)
                {
                case ActionType::ACC:
                    printf("%-*s", spacing, "ACC");
                    break;
                case ActionType::REDUCE:
                    printf("r%-*d", spacing - 1, action.id);
                    break;
                case ActionType::SHIFT:
                    printf("s%-*d", spacing-1, action.id);
                    break;
                default:
                    printf("%-*c", spacing, ' ');
                    break;
                }
            }
            printf("%-*c", spacing, ' ');
            for(auto& nonTerminal : _nonTerminals)
            {
                int id = gotoTableLine[nonTerminal->name()];
                if(id == -1) printf("%-*c", spacing, ' ');
                else printf("%-*d", spacing, id);
            }
            printf("\n");
        }
    }
}