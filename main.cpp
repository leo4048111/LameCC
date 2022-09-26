#include "cc.hpp"

int main(int argc, char** argv)
{
    cc::File* file = new cc::File("D:/Projects/CPP/Homework/CLexer/tests/test.cpp");
    cc::Lexer* lexer = new cc::Lexer(file);
    lexer->run();
}