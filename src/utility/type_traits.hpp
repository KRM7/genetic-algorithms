#ifndef GA_TYPE_TRAITS_HPP
#define GA_TYPE_TRAITS_HPP

#include <type_traits>

namespace genetic_algorithm::detail
{
    template<template<typename...> class T1, template<typename...> class T2>
    struct is_same_template : std::false_type {};

    template<template<typename...> class T>
    struct is_same_template<T, T> : std::true_type {};

    template<template<typename...> class T1, template<typename...> class T2>
    inline constexpr bool is_same_template_v = is_same_template<T1, T2>::value;



    template<template<typename...> class...>
    struct is_one_of_templ : std::false_type {};

    template<template<typename...> class Match,
             template<typename...> class Target,
             template<typename...> class... Targets>
    struct is_one_of_templ<Match, Target, Targets...>
        : std::bool_constant<is_same_template_v<Match, Target> ||
                             is_one_of_templ<Match, Targets...>::value>
    {};

    template<template<typename...> class T, template<typename...> class... Ts>
    inline constexpr bool is_one_of_templ_v = is_one_of_templ<T, Ts...>::value;



    namespace _
    {
        template<size_t N, typename... Ts>
        struct number_of_types_impl : std::integral_constant<size_t, N> {};

        template<size_t N, typename T, typename... Ts>
        struct number_of_types_impl<N, T, Ts...>
            : std::integral_constant<size_t, number_of_types_impl<N + 1, Ts...>::value>
        {};
    }

    template<typename... Ts>
    struct number_of_types : std::integral_constant<size_t, 0> {};

    template<typename T, typename... Ts>
    struct number_of_types<T, Ts...>
        : std::integral_constant<size_t, _::number_of_types_impl<1, Ts...>::value>
    {};

    template<typename... Ts>
    inline constexpr size_t number_of_types_v = number_of_types<Ts...>::value;

    

    template<size_t N, typename... Args>
    struct nth_type_of {};

    template<typename Arg, typename... Args>
    struct nth_type_of<0, Arg, Args...> : std::type_identity<Arg> {};

    template<size_t N, typename Arg, typename... Args>
    struct nth_type_of<N, Arg, Args...>
    {
        using type = typename nth_type_of<N - 1, Args...>::type;
    };

    template<size_t N, typename... Args>
    using nth_type_of_t = typename nth_type_of<N, Args...>::type;



    namespace _
    {
        template<typename Derived, template<typename...> class BaseTempl>
        struct is_derived_from_spec_impl
        {
        private:
            template<typename... TArgs>
            static std::true_type f(BaseTempl<TArgs...>*);

            template<typename...>
            static std::false_type f(...);

        public:
            using type = decltype(f(static_cast<Derived*>(nullptr)));
        };
    }

    template<typename Derived, template<typename...> class BaseTempl>
    using is_derived_from_spec_of = typename _::is_derived_from_spec_impl<Derived, BaseTempl>::type;

    template<typename Derived, template<typename...> class BaseTempl>
    inline constexpr bool is_derived_from_spec_of_v = is_derived_from_spec_of<Derived, BaseTempl>::value;



    template<typename S, template<typename...> class Templ>
    struct is_specialization_of : std::false_type {};

    template<template<typename...> class Templ, typename... TArgs>
    struct is_specialization_of<Templ<TArgs...>, Templ> : std::true_type {};

    template<typename S, template<typename...> class Templ>
    inline constexpr bool is_specialization_of_v = is_specialization_of<S, Templ>::value;


} // namespace genetic_algorithm::detail

#endif // !GA_TYPE_TRAITS_HPP