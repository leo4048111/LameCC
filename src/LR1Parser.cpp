#include "lcc.hpp"

namespace cc
{
    std::unique_ptr<LR1Parser> LR1Parser::_inst;

    std::unique_ptr<AST::Decl> LR1Parser::run(const std::vector<Token*>& tokens, const std::string& productionFilePath)
    {
        parseProductionsFromJson(productionFilePath);

        // TODO
        return nullptr;
    }

    void LR1Parser::parseProductionsFromJson(const std::string& productionFilePath)
    {
        std::ifstream i(productionFilePath);
        json j;
        i >> j;
        
        for(auto& symbol : j["NonTerminals"]) _nonTerminals.insert(std::make_shared<NonTerminal>(symbol.get<std::string>()));
        for(auto& symbol : j["Terminals"]) _terminals.insert(std::make_shared<Terminal>(symbol.get<std::string>()));

        for(auto& production : j["Productions"])
        {
            Production p;
            std::string lhs = production["lhs"].get<std::string>();
            for(auto& nonTerminal : _nonTerminals)
            {
                if(nonTerminal->name() == lhs) {
                    p.lhs = nonTerminal;
                    break;
                }
            }

            for(auto& rhsSymbol : production["rhs"])
            {
                std::string rhs = rhsSymbol.get<std::string>();
                bool hasFound = false;
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

            _productions.push_back(p);
        }
    }

    void LR1Parser::findFirstSetForSymbols()
    {

    }
}