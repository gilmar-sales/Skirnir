#pragma once

#include <meta>
#include <ranges>
#include <tuple>
#include <utility>

namespace refl
{
    namespace _detail
    {
        template <typename T>
        constexpr std::string_view type_name()
        {
            return std::meta::display_string_of(^^T);
        }

        template <typename T>
        consteval auto first_ctor()
        {
            auto ctors =
                std::meta::members_of(^^T,
                                      std::meta::access_context::current()) |
                std::views::filter(std::meta::is_constructor);

            return *ctors.begin();
        }

        template <typename T>
        consteval auto first_ctor_params()
        {
            return std::meta::parameters_of(first_ctor<T>());
        }

        template <typename T>
        consteval std::meta::info first_ctor_params_tuple_m()
        {
            return std::meta::substitute(
                ^^std::tuple,
                first_ctor_params<T>() |
                    std::views::transform(std::meta::type_of) |
                    std::views::transform(std::meta::remove_cvref));
        }

        template <typename T>
        using first_ctor_params_tuple = typename std::remove_cv_t<
            typename[:first_ctor_params_tuple_m<T>():]>;
    } // namespace _detail

    template <typename T>
    using first_ctor_params_tuple =
        typename _detail::first_ctor_params_tuple<T>;

    template <typename T>
    constexpr std::string_view type_name()
    {
        return _detail::type_name<T>();
    }

    template <typename T>
    consteval auto first_ctor_params()
    {
        return _detail::first_ctor_params<T>();
    }
} // namespace refl

// #pragma warning(pop)