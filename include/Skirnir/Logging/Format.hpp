#pragma once

#include "Skirnir/Common/Namespace.hpp"

#include <string>
#include <utility>

#ifdef SKIRNIR_USE_FMT
#  include <fmt/chrono.h>
#  include <fmt/format.h>
#else
#  include <format>
#  include <print>
#endif

namespace SKIRNIR_NAMESPACE::detail
{
#ifdef SKIRNIR_USE_FMT
    template <typename... TArgs>
    using FormatString = fmt::format_string<TArgs...>;

    template <typename... TArgs>
    inline std::string
    Format(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        return fmt::format(std::move(fmt), std::forward<TArgs>(args)...);
    }
#else
    template <typename... TArgs>
    using FormatString = std::format_string<TArgs...>;

    template <typename... TArgs>
    inline std::string
    Format(std::format_string<TArgs...> fmt, TArgs&&... args)
    {
        return std::format(std::move(fmt), std::forward<TArgs>(args)...);
    }
#endif
} // namespace SKIRNIR_NAMESPACE::detail
