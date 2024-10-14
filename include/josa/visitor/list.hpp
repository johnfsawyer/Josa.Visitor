#pragma once
#include <type_traits>
#include <utility>

namespace josa::meta {

template <typename... Ts> struct list;

template <std::size_t N>
using size_constant = std::integral_constant<std::size_t, N>;

template <bool B>
using bool_constant = std::integral_constant<bool, B>;

template <typename> struct always_false : std::false_type {};

//--------------------------------------------------------------------------------------------------
//  size - get the number of elements in a list
//--------------------------------------------------------------------------------------------------

template <typename List> struct size;

template <typename... Ts> struct size<list<Ts...>>
    :   size_constant<sizeof...(Ts)> {};

//--------------------------------------------------------------------------------------------------
//  empty - determine if a list is empty (i.e. is list<>)
//--------------------------------------------------------------------------------------------------

template <typename List> struct empty;

template <> struct empty<list<>> : std::true_type {};

template <typename Hd, typename... Tl> struct empty<list<Hd, Tl...>> : std::false_type {};

//--------------------------------------------------------------------------------------------------
//  contains - determine if a list contains a specified type.
//--------------------------------------------------------------------------------------------------

template <typename T, typename List> struct contains;

template <typename T> struct contains<T, list<>>
    :   std::false_type {};

template <typename T, typename... Tl> struct contains<T, list<T, Tl...>>
    :   std::true_type {};

template <typename T, typename Hd, typename... Tl> struct contains<T, list<Hd, Tl...>>
    :   contains<T, list<Tl...>> {};

//--------------------------------------------------------------------------------------------------
//  count - count the number of times a specified type occurs in a list.
//--------------------------------------------------------------------------------------------------

template <typename T, typename List> struct count;

template <typename T> struct count<T, list<>>
    :   size_constant<0> {};

template <typename T, typename... Tl> struct count<T, list<T, Tl...>>
    :   size_constant<1 + count<T, list<Tl...>>::value> {};

template <typename T, typename Hd, typename... Tl> struct count<T, list<Hd, Tl...>>
    :   count<T, list<Tl...>> {};

//--------------------------------------------------------------------------------------------------
//  all_unique - determine if the list contains only unique types, i.e. there are no duplicates.
//--------------------------------------------------------------------------------------------------

template <typename List> struct all_unique;

template <> struct all_unique<list<>>
    :   std::true_type {};

template <typename Hd, typename... Tl> struct all_unique<list<Hd, Tl...>>
    :   bool_constant<!contains<Hd, list<Tl...>>::value && all_unique<list<Tl...>>::value> {};

//--------------------------------------------------------------------------------------------------
//  convert - given a list<Ts...> create a Type<Ts...>, for some specified Type.
//--------------------------------------------------------------------------------------------------

template <typename List, template <typename...> class Type> struct convert;

template <typename... Ts, template <typename...> class Type>
struct convert<list<Ts...>, Type>
{
    using type = Type<Ts...>;
};

template <typename List, template <typename...> class Type>
using convert_t = typename convert<List, Type>::type;

//--------------------------------------------------------------------------------------------------
//  each_of - determine whether for every type T in a list, the predicate Predicate<T>::value is
//      true.
//--------------------------------------------------------------------------------------------------

template <typename List, template <typename> class Predicate> struct each_of;

template <template <typename> class Predicate>
struct each_of<list<>, Predicate>
    :   std::true_type {};

template <typename Hd, typename... Tl, template <typename> class Predicate>
struct each_of<list<Hd, Tl...>, Predicate>
    :   bool_constant<Predicate<Hd>::value && each_of<list<Tl...>, Predicate>::value> {};

//--------------------------------------------------------------------------------------------------
//  any_of - determine whether at least one type T in a list, the predicate Predicate<T>::value is
//      true.
//--------------------------------------------------------------------------------------------------

template <typename List, template <typename> class Predicate> struct any_of;

template <template <typename> class Predicate>
struct any_of<list<>, Predicate>
    :   std::false_type {};

template <typename Hd, typename... Tl, template <typename> class Predicate>
struct any_of<list<Hd, Tl...>, Predicate>
    :   bool_constant<Predicate<Hd>::value || any_of<list<Tl...>, Predicate>::value> {};

//--------------------------------------------------------------------------------------------------
//  prepend - creates a list by prepending a type to an existing list.
//--------------------------------------------------------------------------------------------------

template <typename T, typename List> struct prepend;

template <typename T, typename List>
using prepend_t = typename prepend<T, List>::type;

template <typename T, typename... Ts> struct prepend<T, list<Ts...>>
{
    using type = list<T, Ts...>;
};

//--------------------------------------------------------------------------------------------------
//  append - creates a list by appending a type to an existing list.
//--------------------------------------------------------------------------------------------------

template <typename List, typename T> struct append;

template <typename List, typename T>
using append_t = typename append<List, T>::type;

template <typename T, typename... Ts> struct append<list<Ts...>, T>
{
    using type = list<Ts..., T>;
};

//--------------------------------------------------------------------------------------------------
//  remove - creates a list where all elements of a specified type are removed from an existing
//      list.
//--------------------------------------------------------------------------------------------------

template <typename T, typename List> struct remove;

template <typename T, typename List>
using remove_t = typename remove<T, List>::type;

template <typename T> struct remove<T, list<>>
{
    using type = list<>;
};

template <typename T, typename... Tl> struct remove<T, list<T, Tl...>>
{
    using type = remove_t<T, list<Tl...>>;
};

template <typename T, typename Hd, typename... Tl> struct remove<T, list<Hd, Tl...>>
{
    using type = prepend_t<Hd, remove_t<T, list<Tl...>>>;
};

//--------------------------------------------------------------------------------------------------
//  transform - given a list creates a new list, where each type T from the original list is
//      typename Transform<T>::type in the new list.
//--------------------------------------------------------------------------------------------------

template <typename List, template <typename> class Transform> struct transform;

template <typename List, template <typename> class Transform>
using transform_t = typename transform<List, Transform>::type;

template <typename... Ts, template <typename> class Transform>
struct transform<list<Ts...>, Transform>
{
    using type = list<typename Transform<Ts>::type...>;
};

//--------------------------------------------------------------------------------------------------
//  wrap - given a list creates a new list, where each type T from the original list is
//      Wrapper<T> in the new list.
//--------------------------------------------------------------------------------------------------

template <typename List, template <typename> class Wrapper> struct wrap;

template <typename List, template <typename> class Wrapper>
using wrap_t = typename wrap<List, Wrapper>::type;

template <typename... Ts, template <typename> class Wrapper>
struct wrap<list<Ts...>, Wrapper>
{
    using type = list<Wrapper<Ts>...>;
};

//--------------------------------------------------------------------------------------------------
//  uniques - makes a list of unique types by removing types that appear earlier in the list
//--------------------------------------------------------------------------------------------------

template <typename List> struct uniques;

template <typename List> using uniques_t = typename uniques<List>::type;

template <> struct uniques<list<>>
{
    using type = list<>;
};

template <typename Hd, typename... Tl> struct uniques<list<Hd, Tl...>>
{
    using type = prepend_t<Hd, remove_t<Hd, list<Tl...>>>;
};

//--------------------------------------------------------------------------------------------------
//  concat - concatenate zero or more lists into a single list
//--------------------------------------------------------------------------------------------------

template <typename... Lists> struct concat;

template <typename... Lists>
using concat_t = typename concat<Lists...>::type;

template <> struct concat<>
{
    using type = list<>;
};

template <typename... Ts> struct concat<list<Ts...>>
{
    using type = list<Ts...>;
};

template <typename... Ts, typename... Us, typename... Lists>
struct concat<list<Ts...>, list<Us...>, Lists...>
{
    using type = concat_t<list<Ts..., Us...>, Lists...>;
};

//--------------------------------------------------------------------------------------------------
//  index_of - gets the index of the first occurrence of a specified type
//--------------------------------------------------------------------------------------------------

template <typename T, typename List> struct index_of;

template <typename T> struct index_of<T, list<>>
{
    static_assert(always_false<T>::value, "list does not contain any elements of type T");
};

template <typename T, typename... Tl>
struct index_of<T, list<T, Tl...>>
    :   size_constant<0> {};

template <typename T, typename Hd, typename... Tl>
struct index_of<T, list<Hd, Tl...>>
    :   size_constant<1 + index_of<T, list<Tl...>>::value> {};

//--------------------------------------------------------------------------------------------------
//  at - gets the type at a specified index
//--------------------------------------------------------------------------------------------------

template <std::size_t Index, typename List> struct at;

template <std::size_t Index, typename List>
using at_t = typename at<Index,List>::type;

template <> struct at<0, list<>>;

template <typename Hd, typename... Tl>
struct at<0, list<Hd, Tl...>>
{
    using type = Hd;
};

template <std::size_t Index, typename Hd, typename... Tl>
struct at<Index, list<Hd, Tl...>>
{
    using type = at_t<Index - 1, list<Tl...>>;
};

//--------------------------------------------------------------------------------------------------
//  div_at - gets the type at a specified index divided by a given divisor
//--------------------------------------------------------------------------------------------------

template <std::size_t Index, std::size_t Div, typename List>
struct div_at
{
    static_assert(Div != 0, "divide by zero");
    using type = at_t<Index / Div, List>;
};

template <std::size_t Index, std::size_t Div, typename List>
using div_at_t = typename div_at<Index, Div, List>::type;

//--------------------------------------------------------------------------------------------------
//  mod_at - gets the type at a specided index modulus a given divisor
//--------------------------------------------------------------------------------------------------

template <std::size_t Index, std::size_t Mod, typename List>
struct mod_at
{
    static_assert(Mod != 0, "divide by zero");
    using type = at_t<Index % Mod, List>;
};

template <std::size_t Index, std::size_t Mod, typename List>
using mod_at_t = typename mod_at<Index, Mod, List>::type;

//--------------------------------------------------------------------------------------------------
//  head - gets the head of a non-empty list
//--------------------------------------------------------------------------------------------------

template <typename List> struct head;

template <typename List> using head_t = typename head<List>::type;

template <typename Hd, typename... Tl> struct head<list<Hd, Tl...>>
{
    using type = Hd;
};

//--------------------------------------------------------------------------------------------------
//  tail - gets the tail of a list
//--------------------------------------------------------------------------------------------------

template <typename List> struct tail;

template <typename List> using tail_t = typename tail<List>::type;

template <> struct tail<list<>>
{
    using type = list<>;
};

template <typename Hd, typename... Tl> struct tail<list<Hd, Tl...>>
{
    using type = list<Tl...>;
};

//--------------------------------------------------------------------------------------------------
//  reverse
//--------------------------------------------------------------------------------------------------

template <typename List> struct reverse;

template <typename List> using reverse_t = typename reverse<List>::type;

template <> struct reverse<list<>>
{
    using type = list<>;
};

template <typename Hd, typename... Tl>
struct reverse<list<Hd, Tl...>>
{
    using type = append_t<reverse_t<list<Tl...>>, Hd>;
};

//--------------------------------------------------------------------------------------------------
//  for_each - for each type T in a list, execute F<T>{}(args...), where F is a class template that
//      can be default constructed and is callable with supplied args...
//--------------------------------------------------------------------------------------------------

namespace detail
{
    template <typename List, template <typename> class F>
    struct for_each_helper;

    template <template <typename> class F>
    struct for_each_helper<list<>, F>
    {
	    template <typename... Args>
	    static constexpr auto execute(Args&&...) -> void {}
    };

    template <typename Hd, typename... Tl, template <typename> class F>
    struct for_each_helper<list<Hd, Tl...>, F>
    {
	    template <typename... Args>
	    static constexpr auto execute(Args&&... args) -> void
        {
		    F<Hd>{}(std::forward<Args>(args)...);
		    for_each_helper<list<Tl...>, F>::execute(std::forward<Args>(args)...);
	    }
    };
}

template <typename List, template <typename> class F, typename... Args>
constexpr auto for_each(Args&&... args) -> void
{
	detail::for_each_helper<List, F>::execute(std::forward<Args>(args)...);
}

//--------------------------------------------------------------------------------------------------
//  index_type - structure pairing an index value with a type
//--------------------------------------------------------------------------------------------------

template <std::size_t I, typename T> struct indexed_type
    :   size_constant<I>
{
    using type = T;
};

//--------------------------------------------------------------------------------------------------
//  indexed - given a list, creates a new list where every T is converted to indexed_type<i,T>, 
//      where i is the index position.
//--------------------------------------------------------------------------------------------------

template <typename List, std::size_t I = 0> struct indexed;

template <typename List, std::size_t I = 0>
using indexed_t = typename indexed<List, I>::type;

template <std::size_t I> 
struct indexed<list<>, I>
{
    using type = list<>;
};

template <typename Hd, typename... Tl, std::size_t I>
struct indexed<list<Hd, Tl...>, I>
{
    using type = prepend_t<indexed_type<I, Hd>, indexed_t<list<Tl...>, I + 1>>;
};

//--------------------------------------------------------------------------------------------------
//  all_pairs
//--------------------------------------------------------------------------------------------------

template <typename List1, typename List2> struct all_pairs;

template <typename List1, typename List2>
using all_pairs_t = typename all_pairs<List1, List2>::type;

namespace detail {
    
template <typename List1, typename List2, typename IS> struct all_pairs_helper;

template <typename... Ts1, typename... Ts2, std::size_t... I>
struct all_pairs_helper<list<Ts1...>, list<Ts2...>, std::index_sequence<I...>>
{
    using list1 = list<Ts1...>;
    using list2 = list<Ts2...>;
    constexpr static auto size2 = sizeof...(Ts2);

    using type = list<list<div_at_t<I,size2,list1>, mod_at_t<I,size2,list2>>...>;
};

} // namespace detail

template <> struct all_pairs<list<>,list<>> { using type = list<>; };
template <typename... Ts> struct all_pairs<list<Ts...>,list<>> { using type = list<>; };
template <typename... Ts> struct all_pairs<list<>,list<Ts...>> { using type = list<>; };

template <typename... Ts1, typename... Ts2>
struct all_pairs<list<Ts1...>, list<Ts2...>>
{
    using type = typename detail::all_pairs_helper<list<Ts1...>, list<Ts2...>,
        std::make_index_sequence<sizeof...(Ts1) * sizeof...(Ts2)>>::type;
};

}
