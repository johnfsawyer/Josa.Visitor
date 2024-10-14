#pragma once

namespace josa::visitor 
{
    template <typename BaseType, typename ConcreteTypes> struct hierarchy;
    template <typename T> struct base_type;
    template <typename... Ts> struct concrete_types;
}
