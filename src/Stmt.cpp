#include "lcc.hpp"

namespace lcc
{
    namespace AST
    {
        json NullStmt::asJson() const
        {
            json j;
            j["type"] = "NullStmt";
            return j;
        }

        json ValueStmt::asJson() const
        {
            json j;
            j["type"] = "ValueStmt";
            j["expr"] = json::array({_expr->asJson()});
            return j;
        }

        json IfStmt::asJson() const
        {
            json j;
            j["type"] = "IfStmt";
            j["condition"] = json::array({_condition->asJson()});
            j["body"] = json::array({_body->asJson()});
            if (_elseBody == nullptr)
                j["elseBody"] = "Empty";
            else
                j["elseBody"] = json::array({_elseBody->asJson()});
            return j;
        }

        json WhileStmt::asJson() const
        {
            json j;
            j["type"] = "WhileStmt";
            j["condition"] = json::array({_condition->asJson()});
            j["body"] = json::array({_body->asJson()});
            return j;
        }

        json DeclStmt::asJson() const
        {
            json j;
            j["type"] = "DeclStmt";
            j["children"] = json::array();
            for (const auto &decl : _decls)
                j["children"].emplace_back(decl->asJson());
            return j;
        }

        json CompoundStmt::asJson() const
        {
            json j;
            j["type"] = "CompoundStmt";
            j["children"] = json::array();
            for (const auto &stmt : _body)
                j["children"].emplace_back(stmt->asJson());
            return j;
        }

        json ReturnStmt::asJson() const
        {
            json j;
            j["type"] = "ReturnStmt";
            if (_value == nullptr)
                j["value"] = "VOID";
            else
                j["value"] = json::array({_value->asJson()});
            return j;
        }

        json AsmStmt::asJson() const
        {
            // TODO: Content json print
            json j;
            j["type"] = "AsmStmt";
            j["AsmStmt"] = _asmString;
            return j;
        }

        bool CompoundStmt::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool DeclStmt::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool IfStmt::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool WhileStmt::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool ValueStmt::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool ReturnStmt::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool AsmStmt::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        char AsmStmt::AsmStringPiece::getModifier() const
        {
            return isLetter(Str[0]) ? Str[0] : '\0';
        };
    }
}