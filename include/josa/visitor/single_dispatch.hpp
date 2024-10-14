#pragma once
#include "common.hpp"
#include "list.hpp"
#include "hierarchy.hpp"
#include "overload.hpp"
#include <unordered_map>
#include <typeindex>

namespace josa::visitor
{
    namespace detail
    {
        template <bool Const, typename F, typename Base, typename Concrete, typename... Args>
        auto make_dispatcher()
        {
            struct dispatcher
            {
                static auto dispatch(F&& f, mk_const_t<Const, Base>& obj, Args&&... args) -> decltype(auto)
                {
                    //  A compile-error on the following line probably means a case is missing from a visitor
                    //  class or overloaded function.
                    //
                    return f(static_cast<mk_const_t<Const, Concrete>&>(obj), std::forward<Args>(args)...);
                }
            };

            return &dispatcher::dispatch;
        }

        template <bool Const, typename F, typename Base, typename ConcreteTL, typename ArgTL>
        struct dispatch_map_maker;

        template <bool Const, typename F, typename Base, typename... Concretes, typename... Args>
        struct dispatch_map_maker<Const, F, Base, meta::list<Concretes...>, meta::list<Args...>>
        {
            using value_t = std::common_type_t<decltype(make_dispatcher<Const, F, Base, Concretes, Args...>())...>;
            using key_t = std::type_index;
            using map_t = std::unordered_map<key_t, value_t>;

            static auto make() -> map_t
            {
                return {{std::type_index(typeid(Concretes)), make_dispatcher<Const, F, Base, Concretes, Args...>()}...};
            }
        };
    }

    template <typename Base, typename... Concretes>
    struct dispatcher<hierarchy<base_type<Base>, concrete_types<Concretes...>>>
    {
        using ConcreteTypeList = meta::list<Concretes...>;

        template <typename F, typename... Args>
        static auto visit(F&& f, const Base& obj, Args&&... args) -> decltype(auto)
        {
            static const auto dispatch_map =
                detail::dispatch_map_maker<true, F, Base, ConcreteTypeList, meta::list<Args...>>::make();
                
            if (const auto it = dispatch_map.find(std::type_index(typeid(obj))); it != dispatch_map.end())
                return it->second(std::forward<F>(f), obj, std::forward<Args>(args)...);

            throw unhandled_type{typeid(obj).name()};
        }

        template <typename F, typename... Args>
        static auto visit(F&& f, Base& obj, Args&&... args) -> decltype(auto)
        {
            static const auto dispatch_map =
                detail::dispatch_map_maker<false, F, Base, ConcreteTypeList, meta::list<Args...>>::make();

            if (const auto it = dispatch_map.find(std::type_index(typeid(obj))); it != dispatch_map.end())
                return it->second(std::forward<F>(f), obj, std::forward<Args>(args)...);

            throw unhandled_type{typeid(obj).name()};
        }

        static auto match(const Base& obj) -> decltype(auto)
        {
            return [&obj](auto&&... fs) -> decltype(auto) {
                return visit(overload(std::forward<decltype(fs)>(fs)...), obj); };
        }

        static auto match(Base& obj) -> decltype(auto)
        {
            return [&obj](auto&&... fs) -> decltype(auto) {
                return visit(overload(std::forward<decltype(fs)>(fs)...), obj); };
        }
    };

    template <typename Handler, typename Base, typename... Concretes>
    struct enable_dispatch<Handler, hierarchy<base_type<Base>, concrete_types<Concretes...>>>
    {
        using dispatcher_t = dispatcher<hierarchy<base_type<Base>, concrete_types<Concretes...>>>;

        template <typename... Args>
        auto visit(const Base& obj, Args&&... args) const -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto visit(const Base& obj, Args&&... args) -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto visit(Base& obj, Args&&... args) const -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj, std::forward<Args>(args)...);       
        }

        template <typename... Args>
        auto visit(Base& obj, Args&&... args) -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj, std::forward<Args>(args)...);
        }

    private:

        auto handler() const -> const Handler& { return static_cast<const Handler&>(*this); }
        auto handler() -> Handler& { return static_cast<Handler&>(*this); }
    };
}
