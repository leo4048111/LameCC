#pragma once

#include <rang.hpp>

#define INFO(msg) \
    std::cout << rang::style::bold << rang::fg::green << "Info: " << rang::style::reset << msg << std::endl;

#define FATAL_ERROR(msg) \
    std::cout << rang::style::bold << rang::fg::red << "Fatal error: " << rang::style::reset << msg << std::endl

#define WARNING(msg) \
    std::cout << rang::style::bold << rang::fg::yellow << "Warning: " << rang::style::reset << msg << std::endl

#include "AST.hpp"
#include "File.hpp"
#include "IRGenerator.hpp"
#include "Lexer.hpp"
#include "LR1Parser.hpp"
#include "Parser.hpp"
#include "Utils.hpp"