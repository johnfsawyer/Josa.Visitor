#include "shapes.hpp"
#include "colors.hpp"
#include <catch2/catch_test_macros.hpp>
#include <josa/visitor.hpp>
#include <memory>
#include <vector>

namespace jv = josa::visitor;
using namespace std::string_literals;

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