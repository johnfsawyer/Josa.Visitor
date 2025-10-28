#pragma once
#include "common.hpp"
#include "hierarchy.hpp"
#include "list.hpp"
#include "overload.hpp"
#include <unordered_map>
#include <typeindex>
#include <array>
#include <type_traits>

namespace josa::visitor
{
    namespace detail
    {
        struct hasher
        {
            auto operator () (const std::array<std::type_index, 2>& key) const -> std::size_t
            {
                constexpr auto h = std::hash<std::type_index>{};
                return h(key[0]) ^ (h(key[1]) << 1);
            }
        };

        template <bool Const1, bool Const2, typename F, typename Base1, typename Base2, typename ConcretePair, typename... Args>
        auto make_dispatcher_2()
        {
            struct dispatcher
            {
                static auto dispatch(F&& f, mk_const_t<Const1, Base1>& obj1, mk_const_t<Const2, Base2>& obj2,
                                    Args&&... args) -> decltype(auto)
                {
                    //  A compile-error on the following line probably means a case is missing from a visitor
                    //  class or overloaded function.
                    //
                    return f(static_cast<mk_const_t<Const1, meta::at_t<0, ConcretePair>>&>(obj1), 
                        static_cast<mk_const_t<Const2, meta::at_t<1, ConcretePair>>&>(obj2), 
                        std::forward<Args>(args)...);
                }
            };

            return &dispatcher::dispatch;
        }

        template <bool Const1, bool Const2, typename F, typename Base1, typename Base2,typename ConcreteAllPairsTL, typename ArgTL>
        struct dispatch_map_maker_2_helper;

        template <bool Const1, bool Const2, typename F, typename Base1, typename Base2, typename... ConcretePairs, typename... Args>
        struct dispatch_map_maker_2_helper<Const1, Const2, F, Base1, Base2, meta::list<ConcretePairs...>, meta::list<Args...>>
        {
            using key_t = std::array<std::type_index, 2>;
            using value_t = std::common_type_t<decltype(make_dispatcher_2<Const1, Const2, F, Base1, Base2, ConcretePairs, Args...>())...>;
            using map_t = std::unordered_map<key_t, value_t, hasher>;

            static auto make() -> map_t
            {
                return {{{std::type_index(typeid(meta::at_t<0, ConcretePairs>)), std::type_index(typeid(meta::at_t<1, ConcretePairs>))},
                    make_dispatcher_2<Const1, Const2, F, Base1, Base2, ConcretePairs, Args...>()}...};
            }
        };

        template <bool Const1, bool Const2, typename F, typename Base1, typename Base2, typename ConcreteTL1, typename ConcreteTL2, typename ArgTL>
        struct dispatch_map_maker_2;

        template <bool Const1, bool Const2, typename F, typename Base1, typename Base2,typename... Concretes1, typename... Concretes2, typename... Args>
        struct dispatch_map_maker_2<Const1, Const2, F, Base1, Base2, meta::list<Concretes1...>, meta::list<Concretes2...>, meta::list<Args...>>
        {
            static auto make() -> decltype(auto)
            {
                return dispatch_map_maker_2_helper<Const1, Const2, F, Base1, Base2,
                    meta::all_pairs_t<meta::list<Concretes1...>, meta::list<Concretes2...>>, meta::list<Args...>>::make();
            }
        };
    }

    template <typename Base1, typename Base2, typename... Concretes1, typename... Concretes2>
    struct dispatcher<hierarchy<base_type<Base1>, concrete_types<Concretes1...>>,
                    hierarchy<base_type<Base2>, concrete_types<Concretes2...>>>
    {
        template <typename F, typename... Args>
        static auto visit(F&& f, const Base1& obj1, const Base2& obj2, Args&&... args) -> decltype(auto)
        {
            static const auto dispatch_map =
                detail::dispatch_map_maker_2<true, true, F, Base1, Base2,
                meta::list<Concretes1...>, meta::list<Concretes2...>, meta::list<Args...>>::make();

            if (const auto it = dispatch_map.find({std::type_index(typeid(obj1)), std::type_index(typeid(obj2))}); it != dispatch_map.end())
                return it->second(std::forward<F>(f), obj1, obj2, std::forward<Args>(args)...);
    
            throw unhandled_type{typeid(obj1).name(), typeid(obj2).name()};
        }

        template <typename F, typename... Args>
        static auto visit(F&& f, const Base1& obj1, Base2& obj2, Args&&... args) -> decltype(auto)
        {
            static const auto dispatch_map =
                detail::dispatch_map_maker_2<true, false, F, Base1, Base2,
                meta::list<Concretes1...>, meta::list<Concretes2...>, meta::list<Args...>>::make();

            if (const auto it = dispatch_map.find({std::type_index(typeid(obj1)), std::type_index(typeid(obj2))}); it != dispatch_map.end())
                return it->second(std::forward<F>(f), obj1, obj2, std::forward<Args>(args)...);

            throw unhandled_type{typeid(obj1).name(), typeid(obj2).name()};
        }

        template <typename F, typename... Args>
        static auto visit(F&& f, Base1& obj1, const Base2& obj2, Args&&... args) -> decltype(auto)
        {
            static const auto dispatch_map =
                detail::dispatch_map_maker_2<false, true, F, Base1, Base2,
                meta::list<Concretes1...>, meta::list<Concretes2...>, meta::list<Args...>>::make();

            if (const auto it = dispatch_map.find({std::type_index(typeid(obj1)), std::type_index(typeid(obj2))}); it != dispatch_map.end())
                return it->second(std::forward<F>(f), obj1, obj2, std::forward<Args>(args)...);

            throw unhandled_type{typeid(obj1).name(), typeid(obj2).name()};
        }

        template <typename F, typename... Args>
        static auto visit(F&& f, Base1& obj1, Base2& obj2, Args&&... args) -> decltype(auto)
        {
            static const auto dispatch_map =
                detail::dispatch_map_maker_2<false, false, F, Base1, Base2,
                meta::list<Concretes1...>, meta::list<Concretes2...>, meta::list<Args...>>::make();

            if (const auto it = dispatch_map.find({std::type_index(typeid(obj1)), std::type_index(typeid(obj2))}); it != dispatch_map.end())
                return it->second(std::forward<F>(f), obj1, obj2, std::forward<Args>(args)...);

            throw unhandled_type{typeid(obj1).name(), typeid(obj2).name()};
        }
            
        static auto match(const Base1& obj1, const Base2& obj2) -> decltype(auto)
        {
            return [&obj1, &obj2](auto... fs) -> decltype(auto) {
                return dispatcher::visit(overload(fs...), obj1, obj2); };
        }

        static auto match(const Base1& obj1, Base2& obj2) -> decltype(auto)
        {
            return [&obj1, &obj2](auto&&... fs) -> decltype(auto) {
                return dispatcher::visit(overload(std::forward<decltype(fs)>(fs)...), obj1, obj2); };
        }

        static auto match(Base1& obj1, const Base2& obj2) -> decltype(auto)
        {
            return [&obj1, &obj2](auto&&... fs) -> decltype(auto) {
                return dispatcher::visit(overload(std::forward<decltype(fs)>(fs)...), obj1, obj2); };
        }

        static auto match(Base1& obj1, Base2& obj2) -> decltype(auto)
        {
            return [&obj1, &obj2](auto&&... fs) -> decltype(auto) {
                return dispatcher::visit(overload(std::forward<decltype(fs)>(fs)...), obj1, obj2); };
        }
    };

    template <typename Handler, typename Base1, typename Base2, typename... Concretes1, typename... Concretes2>
    struct enable_dispatch<Handler, hierarchy<base_type<Base1>, concrete_types<Concretes1...>>,
                                    hierarchy<base_type<Base2>, concrete_types<Concretes2...>>>
    {
        using dispatcher_t = dispatcher<hierarchy<base_type<Base1>, concrete_types<Concretes1...>>,
                                        hierarchy<base_type<Base2>, concrete_types<Concretes2...>>>;

        template <typename... Args>
        auto visit(const Base1& obj1, const Base2& obj2, Args&&... args) const -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj1, obj2, args...);
        }

        template <typename... Args>
        auto visit(const Base1& obj1, const Base2& obj2, Args&&... args) -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj1, obj2, args...);
        }

        template <typename... Args>
        auto visit(const Base1& obj1, Base2& obj2, Args&&... args) const -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj1, obj2, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto visit(const Base1& obj1, Base2& obj2, Args&&... args) -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj1, obj2, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto visit(Base1& obj1, const Base2& obj2, Args&&... args) const -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj1, obj2, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto visit(Base1& obj1, const Base2& obj2, Args&&... args) -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj1, obj2, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto visit(Base1& obj1, Base2& obj2, Args&&... args) const -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj1, obj2, std::forward<Args>(args)...);
        }

        template <typename... Args>
        auto visit(Base1& obj1, Base2& obj2, Args&&... args) -> decltype(auto)
        {
            return dispatcher_t::visit(handler(), obj1, obj2, std::forward<Args>(args)...);
        }

    private:

        auto handler() const -> const Handler& { return static_cast<const Handler&>(*this); }
        auto handler() -> Handler& { return static_cast<Handler&>(*this); }
    };
}
