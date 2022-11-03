#include "lcc.hpp"

namespace cc
{
    std::unique_ptr<LR1Parser> LR1Parser::_inst;

    std::unique_ptr<AST::Decl> LR1Parser::run(const std::vector<Token*>& tokens, const std::string& productionFilePath)
    {
        parseProductionsFromJson(productionFilePath);
        findFirstSetForSymbols();
        constructCanonicalCollections();

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

        _endSymbol = std::make_shared<Terminal>("#");

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


        for(auto& symbol : _terminals)
        {
            std::cout << symbol->name() << ": ";
            auto& first = _first[symbol->name()];
            for(auto & s : first) std::cout << s->name() << " ";
            std::cout << std::endl;
        }

        for(auto& symbol : _nonTerminals)
        {
            std::cout << symbol->name() << ": ";
            auto& first = _first[symbol->name()];
            for(auto & s : first) std::cout << s->name() << " ";
            std::cout << std::endl;
        }
    }

    void LR1Parser::constructCanonicalCollections()
    {
        LR1Item firstItem = {_productions["S'"][0], 0, _endSymbol}; // S' -> .S

        LR1ItemSet I0;
        I0.id = 0;
        I0.items.push_back(firstItem);

        closure(I0); // calculate CLOSURE(I0)
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

                }
            }
        } while (!shouldBail);
        
    }
}