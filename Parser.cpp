#include "lcc.hpp"

namespace cc
{
    void Parser::nextToken()
    {
        if(_pCurToken != _tokens.end()) _pCurToken++;
    }

     void Parser::setup(const std::vector<Token*> tokens)
     {
        _tokens = tokens;
        _pCurToken = _tokens.begin();
     }

     void Parser::run()
     {
        if(_tokens.empty())
            FATAL_ERROR("No tokens specified.");
     }

}