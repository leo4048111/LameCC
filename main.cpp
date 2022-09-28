#include "lcc.hpp"

#include <string>

const char* g_path;

static void parseOpt(int argc, char** argv)
{
    for(int i = 0; i < argc; i++)
    {
        printf("%s\n", argv[i]);
        std::string s(argv[i]);
        if(s.find(".cpp") != std::string::npos)
            g_path = argv[i];
    }
}

int main(int argc, char** argv)
{
    parseOpt(argc, argv);
    cc::File* file = new cc::File(g_path);
    cc::Lexer* lexer = new cc::Lexer(file);
    lexer->run();
}

// int main()
// {
//     // unit test
//     cc::CharBuffer buffer;
//     buffer.append('i');
//     buffer.append('n');
//     buffer.append('t');

//     printf("%s\n", (buffer == "int" ? "True" : "False"));

//     buffer.append('e');

//     printf("%s\n", (buffer == "int" ? "True" : "False"));

//     buffer.append('g');
//     buffer.append('e');
//     buffer.append('r');

//     buffer.reserve(200);

//     printf("%s\n", (buffer == "inte" ? "True" : "False"));
//     printf("%s\n", (buffer == "integer" ? "True" : "False"));
// }

// int main()
// {
//     std::string test = "abcd";
//     printf("%d\n", test.size());
//     printf("%d\n", (int)test[test.size() + 1]);
// }