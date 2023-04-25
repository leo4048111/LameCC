#include "lcc.hpp"

namespace lcc
{
    namespace AST
    {
        static std::string BinaryOpTypeToStringDisc(BinaryOpType type)
        {
            switch (type)
            {
#define BINARY_OPERATION(name, disc) \
    case BinaryOpType::BO_##name:    \
        return disc;
#define UNARY_OPERATION(name, disc)
#include "OperationType.inc"
#undef UNARY_OPERATION
#undef BINARY_OPERATION
            default:
                return "";
            }
        }

        static std::string UnaryOpTypeToStringDisc(UnaryOpType type)
        {
            switch (type)
            {
#define BINARY_OPERATION(name, disc)
#define UNARY_OPERATION(name, disc) \
    case UnaryOpType::UO_##name:    \
        return #name;
#include "OperationType.inc"
#undef UNARY_OPERATION
#undef BINARY_OPERATION
            default:
                return "";
            }
        }

        BinaryOperator::BinaryOperator(BinaryOpType type, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs) : _type(type)
        {
            if (!isAssignment() && lhs->isLValue())
                lhs = std::make_unique<ImplicitCastExpr>(std::move(lhs), AST::CastExpr::CastType::LValueToRValue);
            if (rhs->isLValue())
                rhs = std::make_unique<ImplicitCastExpr>(std::move(rhs), AST::CastExpr::CastType::LValueToRValue);

            _lhs = std::move(lhs);
            _rhs = std::move(rhs);
        }

        ArraySubscriptExpr::ArraySubscriptExpr(const std::string& name, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs) :
            DeclRefExpr(name, false, true)
        {
            if (rhs->isLValue())
                rhs = std::make_unique<ImplicitCastExpr>(std::move(rhs), AST::CastExpr::CastType::LValueToRValue);
            _lhs = std::move(lhs);
            _rhs = std::move(rhs);
        }

        bool BinaryOperator::isAssignment() const
        {
            switch (_type)
            {
            case AST::BinaryOpType::BO_Assign:
            case AST::BinaryOpType::BO_MulAssign:
            case AST::BinaryOpType::BO_DivAssign:
            case AST::BinaryOpType::BO_RemAssign:
            case AST::BinaryOpType::BO_AddAssign:
            case AST::BinaryOpType::BO_SubAssign:
            case AST::BinaryOpType::BO_ShlAssign:
            case AST::BinaryOpType::BO_ShrAssign:
            case AST::BinaryOpType::BO_AndAssign:
            case AST::BinaryOpType::BO_XorAssign:
            case AST::BinaryOpType::BO_OrAssign:
                return true;
            default:
                return false;
            }
        }

        json IntegerLiteral::asJson() const
        {
            json j;
            j["type"] = "IntegerLiteral";
            j["value"] = std::to_string(_value);
            return j;
        }

        json FloatingLiteral::asJson() const
        {
            json j;
            j["type"] = "FloatingLiteral";
            j["value"] = std::to_string(_value);
            return j;
        }

        json DeclRefExpr::asJson() const
        {
            json j;
            j["type"] = "DeclRefExpr";
            j["name"] = _name;
            j["isCall"] = _isCall;
            return j;
        }

        json BinaryOperator::asJson() const
        {
            json j;
            j["type"] = "BinaryOperator";
            j["opcode"] = BinaryOpTypeToStringDisc(_type);
            j["lhs"] = json::array({_lhs->asJson()});
            j["rhs"] = json::array({_rhs->asJson()});
            return j;
        }

        json UnaryOperator::asJson() const
        {
            json j;
            j["type"] = "UnaryOperator";
            j["opcode"] = UnaryOpTypeToStringDisc(_type);
            j["body"] = json::array({_body->asJson()});
            return j;
        }

        json ParenExpr::asJson() const
        {
            json j;
            j["type"] = "ParenExpr";
            j["expr"] = json::array({_subExpr->asJson()});
            return j;
        }

        json CallExpr::asJson() const
        {
            json j;
            j["type"] = "CallExpr";
            j["expr"] = json::array({_functionExpr->asJson()});
            j["params"] = json::array();
            for (const auto &param : _params)
                j["params"].emplace_back(param->asJson());
            return j;
        }

        json CastExpr::asJson() const
        {
            json j;
            j["type"] = "CastExpr";
            switch (_type)
            {
            case CastExpr::CastType::LValueToRValue:
                j["castType"] = "LValueToRValue";
                break;

            default:
                j["castType"] = "Undefined";
                break;
            }
            j["expr"] = json::array({_subExpr->asJson()});
            return j;
        }

        json ImplicitCastExpr::asJson() const
        {
            json j;
            j["type"] = "ImplicitCastExpr";
            switch (_type)
            {
            case CastExpr::CastType::LValueToRValue:
                j["castType"] = "LValueToRValue";
                break;

            default:
                j["castType"] = "Undefined";
                break;
            }
            j["expr"] = json::array({_subExpr->asJson()});
            return j;
        }

        json CharacterLiteral::asJson() const
        {
            json j;
            j["type"] = "CharacterLiteral";
            j["value"] = std::to_string(_value);
            return j;
        }

        json StringLiteral::asJson() const
        {
            json j;
            j["type"] = "StringLiteral";
            j["value"] = _value;
            return j;
        }

        json ArraySubscriptExpr::asJson() const
        {
            json j;
            j["type"] = "ArraySubscriptExpr";
            j["lhs"] = json::array({_lhs->asJson()});
            j["rhs"] = json::array({_rhs->asJson()});
            return j;
        }

        bool DeclRefExpr::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool IntegerLiteral::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool CharacterLiteral::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool FloatingLiteral::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool StringLiteral::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool CastExpr::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool ImplicitCastExpr::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool BinaryOperator::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool UnaryOperator::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool ParenExpr::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool CallExpr::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }

        bool ArraySubscriptExpr::gen(lcc::IRGeneratorBase *generator)
        {
            return generator->gen(this);
        }
    }
}