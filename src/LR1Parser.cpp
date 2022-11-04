#include "lcc.hpp"

namespace cc
{
    std::unique_ptr<LR1Parser> LR1Parser::_inst;

    std::unique_ptr<AST::Decl> LR1Parser::run(const std::vector<Token*>& tokens, const std::string& productionFilePath)
    {
        parseProductionsFromJson(productionFilePath);
        findFirstSetForSymbols();
        constructCanonicalCollections();
        constructLR1ParseTable();
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
        _epsilonSymbol = std::make_shared<Terminal>("$");
        _terminals.insert(_endSymbol);
        _extensiveStartSymbol = std::make_shared<NonTerminal>("S'");

        for(auto& production : j["Productions"])
        {
            Production p;
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
                    In.transitions.insert(std::make_pair(symbol->name(), newItemSet.id));
                }
            }
            _canonicalCollections[InID] = In;
        }

        for(auto& In : _canonicalCollections) printItemSet(In);
    }

    void LR1Parser::constructLR1ParseTable()
    {
        _actionTable.resize(_canonicalCollections.size());
        _gotoTable.resize(_canonicalCollections.size());

        for(auto& In : _canonicalCollections) // for every item set
        {
            Action initialAction = {ActionType::INVALID, -1};
            for(auto& terminal : _terminals) _actionTable[In.id].insert(std::make_pair(terminal->name(), initialAction));
            for(auto& nonTerminal : _nonTerminals) _gotoTable[In.id].insert(std::make_pair(nonTerminal->name(), -1));
        }

        printActionAndGotoTable();
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
        printf("------------------------I%d------------------------\n", itemSet.id);
        for(auto& item : itemSet.items)
        {
            printProduction(item.production, true, item.dotPos);
            printf("    %s", item.lookahead->name().c_str());
            printf("\n");
        }

        for(auto& transition : itemSet.transitions)
            printf("I%d takes %s goto I%d\n", itemSet.id, transition.first.c_str(), transition.second);
        printf("--------------------------------------------------\n");
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