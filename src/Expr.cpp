#include "lcc.hpp"

namespace cc
{
    namespace AST
    {
        BinaryOperator::BinaryOperator(BinaryOpType type, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs):
        _type(type)
        {
            if(!isAssignment() && lhs->isLValue())
                lhs = std::make_unique<ImplicitCastExpr>(std::move(lhs), "LValueToRValue");
            if(rhs->isLValue())
                rhs = std::make_unique<ImplicitCastExpr>(std::move(rhs), "LValueToRValue");
            
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
    }
}