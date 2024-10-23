# Josa.Visitor
Josa.Visitor is a C++17 header-only library that implements the visitor pattern for class hierarchies (single-dispatch) or pairs of class hierarchies (double-dispatch), without requiring any modification to the classes (non-intrusive). Usage is similar to how std::visit is used with std::variants.

# Motivation

The Visitor pattern (as described in "Design Patterns" by Gamma et al.) is useful for extending the functionality of classes within a class hierarchy without implementing that functionality, which may not be essential functions of the classes, directly in each class. This allows the functionality to be kept in a single location instead of being distributed beteween all the classes.

There are some downsides and limitations to this pattern. The class hierarchy needs to be modified to support visitation with an `accept` function, declared as virtual in the base class and overridden in each class. The `accept` function solves the issue of dynamic dispatch. It may not be possible or desirable to modify classes this way, and it prescribes a particular function signature for the visitors' functions.

C++17 offers an alternative approach using `std::variant` and `std::visit`; however, class hierarchies may still be a more desirable solution in some cases, or simply an artifact of legacy code.

Josa.Visitor is an alternative to the traditional Visitor pattern, requiring absolutely zero modification to the classes within the hierarchy. In addition it supports:

- double dispatch version: dynamical dispatch function calls based concrete type of two unrelated objects.
- flexible function signatures: only requirement is object being visited is first parameter (or first 2 for double-dispatch).
- multiple interfaces: visitor class, or standalone visit or match function.

# Basic Example

Let's say you have this cass hierarchy:

```
class Shape {...};

class Square : public Shape {...};
class Circle : public Shape {...};
class Triangle : public Shape {...};

```

First, create a type describing the hierarchy:

```
#include <josa/visitor/hierarchy.hpp>

using ShapeHierarchy = josa::visitor::hierarchy
<
    josa::visitor::base_type<Shape>,
    josa::visitor::concrete_types<Square, Circle, Triangle>
>;
```

Now, Shape objects can be visited from a pointer or reference to the base class Shape, using one of the following three methods.


Option 1 : Visit with a callable object containing overloads for the required types.

`josa::visitor::overload` can be used, as shown below, instead of creating a struct/class.

```
#include <josa/visitor.hpp>

auto getShapeName(const Shape& shape) -> std::string
{
    using Dispatcher = josa::visitor::dispatcher<ShapeHierarchy>;

    const auto f = josa::visitor::overload
    (
        [](const Square&) { return "square"s; },
        [](const Circle&) { return "circle"s; },
        [](const Triangle&) { return "triangle"s; }
    );

    return Dispatcher::visit(f, shape);
}
```

Option 2 : Match function

```
#include <josa/visitor.hpp>

auto getShapeName(const Shape& shape) -> std::string
{
    using Dispatcher = josa::visitor::dispatcher<ShapeHierarchy>;

    return Dispatcher::match(shape)
    (
        [](const Square&) { return "square"s; },
        [](const Circle&) { return "circle"s; },
        [](const Triangle&) { return "triangle"s; }
    );
}
```
Option 3 : Create a visitor class

A visitor struct/class is useful when it is necessary to recursively call visit, see [test/example-regex.cpp](test/example-regex.cpp) for better examples.

```
#include <josa/visitor.hpp>

struct ShapeNamer : josa::visitor::enable_dispatch<ShapeNamer, ShapeHierarchy>
{
    auto operator () (const Square&) const -> std::string { return "square"; }
    auto operator () (const Circle&) const -> std::string { return "circle"; }
    auto operator () (const Triangle&) const -> std::string { return "triangle"; }
};

auto getShapeName(const Shape& shape) -> std::string
{
    const ShapeNamer visitor;
    return visitor.visit(shape);
}

```
