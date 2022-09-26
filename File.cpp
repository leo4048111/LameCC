#include "lcc.hpp"

namespace cc
{
    File::File(std::string path):
    _pos({1, 1})
    {
        _ifs = std::ifstream(path);
        std::string line;
        while(std::getline(_ifs, line))
        {
            _ss << line << '\n';
        }
        _ss << (char)EOF;
        printf("%s\n", _ss.str().c_str());
    }

    const char File::nextChar()
    {
        if(_curIdx >= _ss.str().size()) return EOF;

        char c = _ss.str()[_curIdx++];
        if(c == '\n')
        {
            _pos.line++;
            _pos.column = 1;
        } 
        else _pos.column++;

        return c;
    }

    void File::retractChar()
    {
        _curIdx = (_curIdx <= 0) ? 0 : _curIdx - 1;
        char c = _ss.str()[_curIdx];
        if(c == '\n')
        {
            int i = _curIdx - 1;
            for(; i >= 0 && _ss.str()[i] != '\n'; i--);
            _pos.line--;
            _pos.column = _curIdx - i + (_ss.str()[i] != '\n');
        }
        else _pos.column--;
    }

    const char File::peekChar()
    {
        if(_curIdx < _ss.str().size())
            return _ss.str()[_curIdx];
        
        return EOF;
    }
}