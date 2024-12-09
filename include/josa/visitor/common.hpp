#pragma once
#include <stdexcept>
#include <string>

namespace josa::visitor
{
    struct unhandled_type : std::logic_error 
    {
        explicit unhandled_type(std::string name)
            : logic_error{"unhandled type (" + name + ")"} {} 

        unhandled_type(std::string name1, std::string name2)
            : logic_error{"unhandled type (" + name1 + ", " + name2 + ")"} {} 
    };

    template <typename... Hierarchies> struct dispatcher;
    template <typename Handler, typename... Hierarchies> struct enable_dispatch;

    namespace detail
    {
        template <bool, typename> struct mk_const;
        template <bool b, typename T> using mk_const_t = typename mk_const<b,T>::type;

        template <typename T> struct mk_const<true, T> { using type = const T; };
        template <typename T> struct mk_const<false, T> { using type = T; };
    }
}
