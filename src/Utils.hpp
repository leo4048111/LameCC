#pragma once

#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

#include "Lexer.hpp"

namespace lcc
{
    // some util funcs(Utils.cpp)
    bool isSpace(const char c);
    bool isLetter(const char c);
    bool isSpace(const char c);
    json jsonifyTokens(const std::vector<std::shared_ptr<Token>> &tokens);
    bool dumpJson(const json &j, const std::string outPath);

    // cast helpers
    template <class T, class U>
    std::unique_ptr<T> dynamic_pointer_cast(std::unique_ptr<U> &&r)
    {
        (void)dynamic_cast<T *>(static_cast<U *>(0));

        T *p = dynamic_cast<T *>(r.get());
        if (p)
            r.release();
        return std::unique_ptr<T>(p);
    }
}