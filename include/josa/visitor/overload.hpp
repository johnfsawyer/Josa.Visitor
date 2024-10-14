#pragma once
#include <utility>
#include <type_traits>

namespace josa::visitor
{
    namespace detail
    {
        template <typename... Fs>
        struct overload_set;

        template <typename F>
        struct overload_set<F> : F
        {
            template <typename Fx>
            explicit overload_set(Fx&& f) : F{std::forward<Fx>(f)} {}
            using F::operator();
        };

        template <typename F, typename... Fs>
        struct overload_set<F, Fs...> : F, overload_set<Fs...>
        {
            template <typename Fx, typename... Fxs>
            explicit overload_set(Fx&& f, Fxs&&... fs)
                : F{std::forward<Fx>(f)}, overload_set<Fs...>{std::forward<Fxs>(fs)...} {}

            using F::operator();
            using overload_set<Fs...>::operator();
        };
    }

    template <typename... Fs>
    auto overload(Fs&&... fs)
    {
        return detail::overload_set<std::decay_t<Fs>...>(std::forward<Fs>(fs)...);
    }
}
