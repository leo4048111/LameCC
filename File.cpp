#include "lcc.hpp"

namespace cc
{
    File::File(std::string path):
    _pos({0, 1})
    {
        _ifs = std::ifstream(path);
    }

    void File::nextLine()
    {
        std::string line;

        // flush stringstream first
        _ss.str("");

        // update positions
        _pos.line++;
        _pos.column = 1;
        _curIdx = 0;
        if(std::getline(_ifs, line)) 
            _ss << line << '\n';
        else 
            _ss << (char)EOF;
    }

    const char File::nextChar()
    {
        if(_curIdx >= _ss.str().size()) return EOF;

        char c = _ss.str()[_curIdx++];
        _pos.column++;

        return c;
    }

    void File::retractChar()
    {
        _curIdx = (_curIdx <= 0) ? 0 : _curIdx - 1;
        char c = _ss.str()[_curIdx];
        // if(c == '\n')
        // {
        //     int i = _curIdx - 1;
        //     for(; i >= 0 && _ss.str()[i] != '\n'; i--);
        //     _pos.line--;
        //     _pos.column = _curIdx - i + (_ss.str()[i] != '\n');
        // }
        // else _pos.column--;
        _pos.column = _curIdx + 1;
    }

    const char File::peekChar()
    {
        if(_curIdx >= _ss.str().size()) return EOF;
        
        return _ss.str()[_curIdx];
    }
}