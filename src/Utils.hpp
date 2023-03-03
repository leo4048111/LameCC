/*
 * @Author: leo4048111
 * @Date: 2023-01-13 18:24:14
 * @LastEditTime: 2023-03-03 11:01:29
 * @LastEditors: ***
 * @Description: 
 * @FilePath: \LameCC\src\Utils.hpp
 */
#pragma once

#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

namespace lcc
{
    // some util funcs(Utils.cpp)
    bool isSpace(const char c);
    bool isLetter(const char c);
    bool isDigit(const char c);
    char charToEscapedChar(char c);
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