#include <josa/visitor/hierarchy.hpp>

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
