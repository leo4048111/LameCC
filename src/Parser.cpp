#include "lcc.hpp"

namespace cc
{
    std::unique_ptr<Parser> Parser::_inst;

    void Parser::nextToken()
    {
        if(_pCurToken != _tokens.end()) _pCurToken++;
    }

     void Parser::setup(const std::vector<Token*>& tokens)
     {
        _tokens = tokens;
        _pCurToken = _tokens.begin();
     }

     void Parser::run(const std::vector<Token*>& tokens)
     {
        _pCurToken = _tokens.begin();
        
        while(_pCurToken != _tokens.end())
        {
            switch((*_pCurToken)->type)
            {
            }
        }
     }

}