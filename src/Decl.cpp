#include "lcc.hpp"

namespace lcc
{
    namespace AST
    {
        json TranslationUnitDecl::asJson() const
        {
            json j;
            j["type"] = "TranslationUnitDecl";
            j["children"] = json::array();
            for (const auto &decl : _decls)
                j["children"].emplace_back(decl->asJson());
            return j;
        }

        json NamedDecl::asJson() const
        {
            json j;
            j["type"] = "NamedDecl";
            j["name"] = _name;
            return j;
        }

        json VarDecl::asJson() const
        {
            json j;
            j["type"] = "VarDecl";
            j["name"] = _name;
            if (_isInitialized)
                j["init"] = json::array({_value->asJson()});
            else
                j["init"] = false;
            return j;
        }

        json ParmVarDecl::asJson() const
        {
            json j;
            j["type"] = "ParmVarDecl";
            j["name"] = _name;
            return j;
        }

        json FunctionDecl::asJson() const
        {
            json j;
            j["type"] = "FunctionDecl";
            std::string functionType;
            functionType += _type; // ret value type
            functionType += '(';
            bool isBegin = true;
            for (const auto &param : _params)
            {
                if (!isBegin)
                    functionType += ", ";
                functionType += param->type();
                isBegin = false;
            }
            functionType += ')';
            j["functionType"] = functionType;
            j["name"] = name();
            j["params"] = json::array();
            for (const auto &param : _params)
                j["params"].emplace_back(param->asJson());
            if (_body != nullptr)
                j["body"] = json::array({_body->asJson()});
            else
                j["body"] = "empty";
            return j;
        }

        bool TranslationUnitDecl::gen(lcc::IRGeneratorBase* generator)
        {
            return generator->gen(this);
        }

        bool VarDecl::gen(lcc::IRGeneratorBase* generator)
        {
            return generator->gen(this);
        }

        bool FunctionDecl::gen(lcc::IRGeneratorBase* generator)
        {
            return generator->gen(this);
        }
    }
}