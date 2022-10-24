#include "lcc.hpp"

#define DEFAULT_TOKEN_DUMP_PATH "tokens.json"
#define DEFAULT_AST_DUMP_PATH "ast.json"
#define DEFAULT_OUT_FILE "a.out"

std::string g_in_path;
std::string g_out_path;
std::string g_dump_token_out_path;
std::string g_dump_ast_out_path;
bool g_shouldDumpTokens = false;
bool g_shouldDumpAST = false;

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
        .description("Dump tokens in json format")
        .type(po::string)
        .single();

    auto& A = parser["dump-ast"]
        .abbreviation('A')
        .description("Dump AST Nodes in json format")
        .type(po::string)
        .single();

    if(!parser(argc, argv)) return false;

    if(help.was_set()) return false;

    if(file.size() < 1)
    {
        FATAL_ERROR("No input file specified");
        return false;
    }

    if(O.was_set())
    {
        g_out_path = O.get().string;
        if(g_out_path.empty())
        {
            WARNING("Out file path not specified, assuming " << DEFAULT_OUT_FILE);
            g_out_path = DEFAULT_OUT_FILE;
        }
    }
    else {
        WARNING("Out file path not specified, assuming " << DEFAULT_OUT_FILE);
        g_out_path = DEFAULT_OUT_FILE;
    }

    if(T.was_set())
    {
        g_shouldDumpTokens = true;
        g_dump_token_out_path = T.get().string;

        if(g_dump_token_out_path.empty())
        {
            WARNING("Token dump file path not specified, assuming " << DEFAULT_TOKEN_DUMP_PATH);
            g_dump_token_out_path = DEFAULT_TOKEN_DUMP_PATH;
        }
    }

    if(A.was_set())
    {
        g_shouldDumpAST = true;
        g_dump_ast_out_path = T.get().string;

        if(g_dump_ast_out_path.empty())
        {
            WARNING("AST dump file path not specified, assuming " << DEFAULT_AST_DUMP_PATH);
            g_dump_ast_out_path = DEFAULT_AST_DUMP_PATH;
        }
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

    auto tokens = cc::Lexer::getInstance()->run(file);

    if(g_shouldDumpTokens)
        cc::dumpJson(cc::jsonifyTokens(tokens), g_dump_token_out_path);
    
    cc::Parser::getInstance()->run(tokens);

    // cleanup
    for(auto& token: tokens) cc::freeToken(token);
}