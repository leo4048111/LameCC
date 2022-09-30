#include "lcc.hpp"

#define DEFAULT_DUMP_PATH "a.dump"

std::string g_in_path;
std::string g_out_path;
bool g_shouldDumpTokens = false;

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
            g_in_path = std::move(x);
            });

    auto& O = parser["out"]
        .abbreviation('o')
        .description("set output file path")
        .type(po::string)
        .single();
    
    auto& T = parser["dump-tokens"]
        .abbreviation('T')
        .description("Dump tokens in json format");

    if(!parser(argc, argv)) return false;

    if(help.was_set()) return false;

    if(file.size() < 1)
    {
        FATAL_ERROR("No input file specified");
        return false;
    }

    if(O.was_set())
        g_out_path = O.get().string;

    if(T.was_set())
    {
        g_shouldDumpTokens = true;
        if(g_out_path.empty())
        {
            WARNING("Dump file path not specified, assuming " << DEFAULT_DUMP_PATH);
            g_out_path = DEFAULT_DUMP_PATH;
        }
        return true;
    }

    return true;
}

int main(int argc, char** argv)
{
    if(!parseOpt(argc, argv)) return 0;
    cc::File* file = new cc::File(g_in_path);
    if(file->fail()) 
    {
        FATAL_ERROR(g_in_path << ": No such file or directory");
        return 0;
    }
    cc::Lexer* lexer = new cc::Lexer(file);
    lexer->run(g_shouldDumpTokens, g_out_path);
}