#include "lcc.hpp"
#include "ProgramOptions.hpp"

std::string g_path;

#define FATAL_ERROR(msg) \
    std::cout << po::red << "Fatal error: " << po::light_gray << msg << std::endl

static bool parseOpt(int argc, char** argv)
{
    po::parser parser;

    auto& help = parser["help"]
        .abbreviation('?')
        .description("show all available options")
        .callback([&]{ 
            std::cout << parser << '\n';
            });

    auto& file = parser[""]
        .single()
        .callback([&](std::string const& x){
            g_path = std::move(x);
            });
    
    if(!parser(argc, argv)) return false;

    if(help.was_set()) return false;

    if(file.size() < 1)
    {
        FATAL_ERROR("No input file specified");
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    if(!parseOpt(argc, argv)) return 0;
    cc::File* file = new cc::File(g_path);
    if(file->fail()) 
    {
        FATAL_ERROR(g_path << ": No such file or directory");
        return 0;
    }
    cc::Lexer* lexer = new cc::Lexer(file);
    lexer->run();
}