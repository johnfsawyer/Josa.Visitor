#include "types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <josa/visitor.hpp>
#include <memory>
#include <vector>

namespace jv = josa::visitor;
using namespace std::string_literals;

TEST_CASE("test single-dispatch visitor class")
{

}

TEST_CASE("single-dispatch visit function with overload set")
{
    std::vector<std::unique_ptr<Shape>> shapeVec;

    shapeVec.push_back(std::make_unique<Square>());
    shapeVec.push_back(std::make_unique<Circle>());

    std::vector<std::string> shapeNameVec;

    const auto getShapeName = jv::overload
    (
        [](const Square&) { return "square"s; },
        [](const Circle&) { return "circle"s; }
    );

    using ShapeDispatcher = jv::dispatcher<ShapeHierarchy>;

    for (const auto& pShape : shapeVec)
    {
        shapeNameVec.push_back(ShapeDispatcher::visit(getShapeName, *pShape));
    }

    REQUIRE(shapeNameVec.size() == shapeVec.size());
    CHECK(shapeNameVec[0] == "square");
    CHECK(shapeNameVec[1] == "circle");
}

TEST_CASE("single-dispatch visitation with match function")
{
    std::vector<std::unique_ptr<Shape>> shapeVec;

    shapeVec.push_back(std::make_unique<Square>());
    shapeVec.push_back(std::make_unique<Circle>());

    std::vector<std::string> shapeNameVec;

    using ShapeDispatcher = jv::dispatcher<ShapeHierarchy>;

    for (const auto& pShape : shapeVec)
    {
        auto s = ShapeDispatcher::match(*pShape)
        (
            [](const Square&) { return "square"s; },
            [](const Circle&) { return "circle"s; }
        );

        shapeNameVec.push_back(std::move(s));
    }

    REQUIRE(shapeNameVec.size() == shapeVec.size());
    CHECK(shapeNameVec[0] == "square");
    CHECK(shapeNameVec[1] == "circle");
}


struct Evaluator : jv::enable_dispatch<Evaluator, MathAst::Hierarchy>
{
    auto operator()(const MathAst::Value& node) const -> int
    {
        return node.value();
    }

    auto operator()(const MathAst::Negate& node) const -> int
    {
        return -visit(node.expr());
    }

    auto operator()(const MathAst::Plus& node) const -> int
    {
        return visit(node.expr1()) + visit(node.expr2());
    }

    auto operator()(const MathAst::Times& node) const -> int
    {
        return visit(node.expr1()) * visit(node.expr2());
    }
};

auto evaluate(const MathAst::ExprPtr& pExpr) -> int
{
    return Evaluator{}.visit(*pExpr);
}

TEST_CASE("visitor struct using enable_dispatch")
{
    using namespace MathAst;
    const auto pExpr = negate(times(value(2), plus(value(3), value(4))));
    CHECK(evaluate(pExpr) == -14);
}

TEST_CASE("visitor struct using enable_dispatch, non-const objects, extra argument, and no return value")
{
    using namespace MathAst;

    //  Adds x to every value in an expression
    //
    struct Modify : jv::enable_dispatch<Modify, Hierarchy>
    {
        void operator()(Value& node, const int x) const
        {
            node.setValue(node.value() + x);
        }

        void operator()(Negate& node, const int x) const
        {
            visit(node.expr(), x);
        }

        void operator()(BinaryOp& node, const int x) const
        {
            visit(node.expr1(), x);
            visit(node.expr2(), x); 
        }
    };

    auto pExpr = negate(times(value(2), plus(value(3), value(4))));
    Modify{}.visit(*pExpr, -5);
    CHECK(evaluate(pExpr) == -9);
}

TEST_CASE("visitor struct with a non-copyable argument - requiring move")
{
    struct MoveSemantics : jv::enable_dispatch<MoveSemantics, ColorHierarchy>
    {
        auto operator()(const Red&, NonCopyable) const -> void {}
        auto operator()(const Blue&, NonCopyable) const -> void {}
    };
    
    Red red;
    MoveSemantics vis;

    vis.visit(red, NonCopyable{});

    NonCopyable nc;
    vis.visit(red, std::move(nc));
}

TEST_CASE("visitor non-copyable non-moveable temporary handler")
{
    struct Handler : NonCopyableNonMoveable
    {
        auto operator()(const Red&) const -> void {}
        auto operator()(const Blue&) const -> void {}
    };
    
    Red red;
    jv::dispatcher<ColorHierarchy>::visit(Handler{}, red);
}
