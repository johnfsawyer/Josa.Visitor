#pragma once
#include <josa/visitor/hierarchy.hpp>
#include <memory>

struct Color
{
    virtual ~Color() {}
    auto isConst() const -> bool { return true; }
    auto isConst() -> bool { return false; }
};

struct Red final : Color {};
struct Blue final : Color {};

using ColorHierarchy = josa::visitor::hierarchy
<
    josa::visitor::base_type<Color>,
    josa::visitor::concrete_types<Red, Blue>
>;

//--------------------------------------------------------------------------------------------------

struct Shape { virtual ~Shape() {} };
struct Square final : Shape {};
struct Circle final : Shape {};

//  Deliberately ommitted from ShapeHierarchy
struct BadShape final : Shape {};

using ShapeHierarchy = josa::visitor::hierarchy
<
    josa::visitor::base_type<Shape>,
    josa::visitor::concrete_types<Square, Circle>
>;

//--------------------------------------------------------------------------------------------------

namespace MathAst
{
    class Expr
    {
    public:

        virtual ~Expr() = default;

    protected:

        Expr() = default;
    };

    using ExprPtr = std::unique_ptr<Expr>;

    class Value final : public Expr
    {
    public:

        explicit Value(const int i)
            :   value_{i}
        {}

        auto value() const -> int
        {
            return value_;
        }

        void setValue(const int i)
        {
            value_ = i;
        }

    private:

        int value_;
    };

    class Negate final : public Expr
    {
    public:

        explicit Negate(ExprPtr pExpr)
            :   pExpr_{std::move(pExpr)}
        {}

        auto expr() const -> const Expr& { return *pExpr_; }
        auto expr() -> Expr& { return *pExpr_; }

    private:

        ExprPtr pExpr_;
    };

    class BinaryOp : public Expr
    {
    public:

        auto expr1() const -> const Expr&  { return *pExpr1_; }
        auto expr2() const -> const Expr&  { return *pExpr2_; }
        auto expr1() -> Expr&  { return *pExpr1_; }
        auto expr2() -> Expr&  { return *pExpr2_; }

    protected:

        BinaryOp(ExprPtr pExpr1, ExprPtr pExpr2)
            :   pExpr1_{std::move(pExpr1)}, pExpr2_{std::move(pExpr2)}
        {}


    private:

        ExprPtr pExpr1_;
        ExprPtr pExpr2_;
    };

    class Plus : public BinaryOp
    {
    public:

        Plus(ExprPtr pExpr1, ExprPtr pExpr2)
            :   BinaryOp(std::move(pExpr1), std::move(pExpr2))
        {}

    };

    class Times : public BinaryOp
    {
    public:

        Times(ExprPtr pExpr1, ExprPtr pExpr2)
            :   BinaryOp(std::move(pExpr1), std::move(pExpr2))
        {}

    };

    inline auto value(const int i) -> ExprPtr
    {
        return std::make_unique<Value>(i);
    }

    inline auto negate(ExprPtr pExpr) -> ExprPtr
    {
        return std::make_unique<Negate>(std::move(pExpr));
    }

    inline auto plus(ExprPtr pExpr1, ExprPtr pExpr2) -> ExprPtr
    {
        return std::make_unique<Plus>(std::move(pExpr1), std::move(pExpr2));
    }

    inline auto times(ExprPtr pExpr1, ExprPtr pExpr2) -> ExprPtr
    {
        return std::make_unique<Times>(std::move(pExpr1), std::move(pExpr2));
    }

    using Hierarchy = josa::visitor::hierarchy
    <
        josa::visitor::base_type<Expr>,
        josa::visitor::concrete_types<Value, Negate, Plus, Times>
    >;
}

//--------------------------------------------------------------------------------------------------

struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    auto operator = (const NonCopyable&) -> NonCopyable& = delete;
    auto operator = (NonCopyable&&) -> NonCopyable& = default;
    ~NonCopyable() = default;
};

struct NonCopyableNonMoveable
{
    NonCopyableNonMoveable() = default;
    NonCopyableNonMoveable(const NonCopyableNonMoveable&) = delete;
    NonCopyableNonMoveable(NonCopyableNonMoveable&&) = delete;
    auto operator = (const NonCopyableNonMoveable&) -> NonCopyableNonMoveable& = delete;
    auto operator = (NonCopyableNonMoveable&&) -> NonCopyableNonMoveable& = delete;
    ~NonCopyableNonMoveable() = default;
};
