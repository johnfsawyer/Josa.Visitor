#include <josa/visitor.hpp>
#include "types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

namespace jv = josa::visitor;
using namespace std::string_literals;


TEST_CASE("double dispatch enabled visitor class")
{
    struct ColorShapeNamer : jv::enable_dispatch<ColorShapeNamer, ColorHierarchy, ShapeHierarchy>
    {
        auto operator()(const Red& c, const Square& s) const -> std::string
        { 
            return "red square"s;
        }

        auto operator()(const Red& c, const Circle& s) const -> std::string
        { 
            return "red circle"s;
        }

        auto operator()(const Blue& c, const Square& s) const -> std::string
        { 
            return "blue square"s;
        }

        auto operator()(const Blue& c, const Circle& s) const -> std::string
        { 
            return "blue circle"s;
        }
    };

    std::vector<std::tuple<std::unique_ptr<Color>, std::unique_ptr<Shape>>> colorShapeVec;

    colorShapeVec.emplace_back(std::make_unique<Red>(), std::make_unique<Circle>());
    colorShapeVec.emplace_back(std::make_unique<Blue>(), std::make_unique<Square>());
    colorShapeVec.emplace_back(std::make_unique<Red>(), std::make_unique<Square>());

    ColorShapeNamer visitor;
    std::vector<std::string> nameVec;

    for (const auto& [pColor, pShape] : colorShapeVec)
    {
        nameVec.push_back(visitor.visit(*pColor, *pShape));
    }

    REQUIRE(nameVec.size() == 3);
    CHECK(nameVec[0] == "red circle"s);
    CHECK(nameVec[1] == "blue square"s);
    CHECK(nameVec[2] == "red square"s);
}

TEST_CASE("double dispatch enabled visitor class with unhandled combination")
{
    struct ColorShapeNamer : jv::enable_dispatch<ColorShapeNamer, ColorHierarchy, ShapeHierarchy>
    {
        auto operator()(const Red& c, const Square& s) const -> std::string
        { 
            return "red square"s;
        }

        auto operator()(const Red& c, const Circle& s) const -> std::string
        { 
            return "red circle"s;
        }

        auto operator()(const Blue& c, const Square& s) const -> std::string
        { 
            return "blue square"s;
        }

        auto operator()(const Blue& c, const Circle& s) const -> std::string
        { 
            return "blue circle"s;
        }
    };

    ColorShapeNamer visitor;

    std::unique_ptr<Color> pColor;
    std::unique_ptr<Shape> pShape;

    pColor = std::make_unique<Red>();
    pShape = std::make_unique<Circle>();

    CHECK(visitor.visit(*pColor, *pShape) == "red circle"s);

    pShape = std::make_unique<BadShape>();

    CHECK_THROWS_AS(visitor.visit(*pColor, *pShape), jv::unhandled_type);
}

TEST_CASE("double dispatch enabled visitor class with base handlers")
{
    struct ColorShapeNamer : jv::enable_dispatch<ColorShapeNamer, ColorHierarchy, ShapeHierarchy>
    {
        auto operator()(const Red& c, const Square& s) const -> std::string
        { 
            return "red square"s;
        }

        auto operator()(const Red& c, const Circle& s) const -> std::string
        { 
            return "red circle"s;
        }

        auto operator()(const Blue& c, const Shape& s) const -> std::string
        { 
            return "blue shape"s;
        }
    };

    ColorShapeNamer visitor;

    std::unique_ptr<Color> pColor;
    std::unique_ptr<Shape> pShape;

    pColor = std::make_unique<Red>();
    pShape = std::make_unique<Circle>();

    CHECK(visitor.visit(*pColor, *pShape) == "red circle"s);

    pColor = std::make_unique<Blue>();

    CHECK(visitor.visit(*pColor, *pShape) == "blue shape"s);
}

TEST_CASE("visitor double dispatch (const, const)")
{
    using Dispatcher = jv::dispatcher<ColorHierarchy, ShapeHierarchy>;

    const auto red = Red{};
    const auto circle = Circle{};

    const Color& color = red;
    const Shape& shape = circle;

    const auto f = jv::overload
    (
        [](const Red& c, const Square& s)   { return "red square"s; },
        [](const Red& c, const Circle& s)   { return "red circle"s; },
        [](const Blue& c, const Square& s)  { return "blue square"s; },
        [](const Blue& c, const Circle& s)  { return "blue circle"s; }
    );

    const auto s = Dispatcher::visit(f, color, shape);

    CHECK(s == "red circle");
    
    const auto t = Dispatcher::match(color, shape)
    (
        [](const Red& c, const Circle& s)   { return true; },
        [](const Color& c, const Shape& s)  { return false; }
    );

    CHECK(t == true);
}

TEST_CASE("visitor double dispatch (const, non-const)")
{
    using Dispatcher = jv::dispatcher<ColorHierarchy, ShapeHierarchy>;

    const auto red = Red{};
    auto circle = Circle{};

    const Color& color = red;
    Shape& shape = circle;

    const auto f = jv::overload
    (
        [](const Red& c, Square& s)   { return "red square"s; },
        [](const Red& c, Circle& s)   { return "red circle"s; },
        [](const Blue& c, Square& s)  { return "blue square"s; },
        [](const Blue& c, Circle& s)  { return "blue circle"s; }
    );

    const auto s = Dispatcher::visit(f, color, shape);

    CHECK(s == "red circle");
    
    const auto t = Dispatcher::match(color, shape)
    (
        [](const Red& c, Circle& s)   { return true; },
        [](const Color& c, Shape& s)  { return false; }
    );

    CHECK(t == true);
}

TEST_CASE("visitor double dispatch (non-const, const)")
{
    using Dispatcher = jv::dispatcher<ColorHierarchy, ShapeHierarchy>;

    auto red = Red{};
    const auto circle = Circle{};

    Color& color = red;
    const Shape& shape = circle;

    const auto f = jv::overload
    (
        [](Red& c, const Square& s)   { return "red square"s; },
        [](Red& c, const Circle& s)   { return "red circle"s; },
        [](Blue& c, const Square& s)  { return "blue square"s; },
        [](Blue& c, const Circle& s)  { return "blue circle"s; }
    );

    const auto s = Dispatcher::visit(f, color, shape);

    CHECK(s == "red circle");
    
    const auto t = Dispatcher::match(color, shape)
    (
        [](Red& c, const Circle& s)   { return true; },
        [](Color& c, const Shape& s)  { return false; }
    );

    CHECK(t == true);
}

TEST_CASE("visitor double dispatch (non-const, non-const)")
{
    using Dispatcher = jv::dispatcher<ColorHierarchy, ShapeHierarchy>;

    auto red = Red{};
    auto circle = Circle{};

    Color& color = red;
    Shape& shape = circle;

    const auto f = jv::overload
    (
        [](Red& c, Square& s)   { return "red square"s; },
        [](Red& c, Circle& s)   { return "red circle"s; },
        [](Blue& c, Square& s)  { return "blue square"s; },
        [](Blue& c, Circle& s)  { return "blue circle"s; }
    );

    const auto s = Dispatcher::visit(f, color, shape);

    CHECK(s == "red circle");
    
    const auto t = Dispatcher::match(color, shape)
    (
        [](Red& c, Circle& s)   { return true; },
        [](Color& c, Shape& s)  { return false; }
    );

    CHECK(t == true);
}