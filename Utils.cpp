#include "lcc.hpp"

namespace cc
{
    bool isSpace(const char c)
    {
        return (c == ' ' || c == '\t' || c == '\f' || c == '\v');    
    }
}