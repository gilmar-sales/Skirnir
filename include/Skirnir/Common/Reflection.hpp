#pragma once

#include <meta>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace refl
{
    template <typename T>
    consteval std::meta::info first_ctor()
    {
        auto ctors =
            std::meta::members_of(^^T, std::meta::access_context::current()) |
            std::views::filter(std::meta::is_constructor);

        return *ctors.begin();
    }

    template <typename T>
    consteval std::vector<std::meta::info> first_ctor_params()
    {
        return std::meta::parameters_of(first_ctor<T>());
    }

    template <typename T>
    consteval std::meta::info first_ctor_params_tuple_m()
    {
        return std::meta::substitute(
            ^^std::tuple,
            first_ctor_params<T>() | std::views::transform(std::meta::type_of) |
                std::views::transform(std::meta::remove_cvref));
    }

    template <typename T>
    using first_ctor_params_tuple =
        typename std::remove_cv_t<typename[:first_ctor_params_tuple_m<T>():]>;

    template <typename T>
    constexpr std::string_view type_name()
    {
        return std::meta::display_string_of(^^T);
    }

    template <typename T>
    constexpr std::string_view type_namespace()
    {
        return std::meta::display_string_of(std::meta::parent_of(^^T));
    }

    template <typename T>
    consteval auto first_ctor_params()
    {
        return first_ctor_params<T>();
    }

    template <typename T>
    using first_template_arg_of =
        typename[:std::meta::template_arguments_of(^^T)[0]:];

    template <typename T>
    inline constexpr auto nonstatic_members_v =
        std::meta::nonstatic_data_members_of(
            ^^T, std::meta::access_context::current());

    template <typename T>
    inline constexpr auto nonstatic_member_names_v = []() consteval {
        std::vector<std::string_view> names;
        for (auto member : nonstatic_members_v<T>)
        {
            names.push_back(std::meta::display_string_of(member));
        }
        return names;
    }();

    template <typename T>
    consteval std::vector<std::string_view> nonstatic_member_names()
    {
        return nonstatic_member_names_v<T>;
    }

    namespace detail
    {
        template <typename T, typename Fn, std::size_t... Is>
        constexpr void for_each_member_impl(T& obj, Fn&& fn,
                                            std::index_sequence<Is...>)
        {
            (fn(nonstatic_member_names_v<T>[Is],
                [:nonstatic_members_v<T>[Is]:](obj)),
             ...);
        }
    } // namespace detail

    template <typename T, typename Fn>
    constexpr void for_each_member(T& obj, Fn&& fn)
    {
        detail::for_each_member_impl(
            obj, std::forward<Fn>(fn),
            std::make_index_sequence<nonstatic_members_v<T>.size()> {});
    }

    template <typename E>
        requires std::is_enum_v<E>
    constexpr std::string_view enum_to_string(E value)
    {
        template for (constexpr auto e :
                      std::define_static_array(std::meta::enumerators_of(^^E)))
        {
            if (value == [:e:])
            {
                return std::meta::identifier_of(e);
            }
        }

        return "<unknown>";
    }
} // namespace refl

// #pragma warning(pop)
