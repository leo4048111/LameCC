#include "lcc.hpp"

// int main(int argc, char** argv)
// {
//     cc::File* file = new cc::File("D:/Projects/CPP/Homework/LameCC/tests/test.cpp");
//     cc::Lexer* lexer = new cc::Lexer(file);
//     lexer->run();
// }

int main()
{
    // unit test
    cc::CharBuffer buffer;
    buffer.append('i');
    buffer.append('n');
    buffer.append('t');

    printf("%s\n", (buffer == "int" ? "True" : "False"));

    buffer.append('e');

    printf("%s\n", (buffer == "int" ? "True" : "False"));

    buffer.append('g');
    buffer.append('e');
    buffer.append('r');

    printf("%s\n", (buffer == "inte" ? "True" : "False"));
    printf("%s\n", (buffer == "integer" ? "True" : "False"));
}