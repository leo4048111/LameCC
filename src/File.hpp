#pragma once

#include <fstream>
#include <sstream>
#include <string>

namespace lcc
{
    // A simple vec2 struct for marking position
    typedef struct _Position
    {
        int line;
        int column;
    } Position;

    // File class(File.cpp)
    class File
    {
    public:
        File(std::string path);
        ~File();

        const bool fail() const;

    public:
        void nextLine();
        const char nextChar();
        void retractChar();
        const char peekChar();
        const std::string curLine();

        // getters
        const Position getPosition() const { return _pos; };
        const std::string path() const { return _path; };

    private:
        std::stringstream _ss;
        std::ifstream _ifs;
        std::string _path;
        Position _pos;
        int _curIdx{0};
    };
}