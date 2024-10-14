#pragma once
#include <josa/visitor/hierarchy.hpp>

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
