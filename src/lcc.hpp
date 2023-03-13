#pragma once

#include <rang.hpp>

#define INFO(msg)                                                                                                \
    do                                                                                                           \
    {                                                                                                            \
        std::cout << rang::style::bold << rang::fg::green << "Info: " << rang::style::reset << msg << std::endl; \
    } while (0)

#define FATAL_ERROR(msg)                                                                                              \
    do                                                                                                                \
    {                                                                                                                 \
        std::cout << rang::style::bold << rang::fg::red << "Fatal error: " << rang::style::reset << msg << std::endl; \
    } while (0)

#define WARNING(msg)                                                                                                 \
    do                                                                                                               \
    {                                                                                                                \
        std::cout << rang::style::bold << rang::fg::yellow << "Warning: " << rang::style::reset << msg << std::endl; \
    } while (0)

#include "Options.hpp"
#include "AST.hpp"
#include "File.hpp"
#include "IRGenerator.hpp"
#include "Lexer.hpp"
#include "LR1Parser.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "Codegen.hpp"
