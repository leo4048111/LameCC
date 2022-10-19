#include "lcc.hpp"

namespace cc
{
    namespace AST
    {
        BinaryOperator::BinaryOperator(BinaryOpType type, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs):
        _type(type)
        {
            _lhs = std::move(lhs);
            _rhs = std::move(rhs);
        }
    }
}