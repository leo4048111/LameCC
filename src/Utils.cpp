#include "lcc.hpp"
#include <cctype>

namespace lcc
{
    bool isSpace(const char c)
    {
        return (c == ' ' || c == '\t' || c == '\f' || c == '\v');
    }

    bool isLetter(const char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    bool isDigit(const char c)
    {
        return std::isdigit(c);
    }

    char charToEscapedChar(char c)
    {
        switch (c)
        {
        case 'a':
            c = '\a';
            break;
        case 'b':
            c = '\b';
            break;
        case 'f':
            c = '\f';
            break;
        case 'n':
            c = '\n';
            break;
        case 'r':
            c = '\r';
            break;
        case 't':
            c = '\t';
            break;
        case 'v':
            c = '\v';
            break;
        default:
            break;
        }

        return c;
    }

    bool dumpJson(const json &j, const std::string outPath)
    {
        if (outPath.empty())
        {
            FATAL_ERROR("Dump file path not specified.");
            return false;
        }

        std::ofstream ofs(outPath);
        ofs << j.dump(2) << std::endl;
        ofs.close();
        return true;
    }
}