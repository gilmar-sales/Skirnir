#pragma once

#include <optional>
#include <type_traits>
#include <vector>

#include "Arc.hpp"
#include "Namespace.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Detects whether @c T is @c std::vector<Arc<U>> for some U.
     */
    template <typename T>
    struct is_vector_of_arc : std::false_type
    {
    };

    template <typename U, typename Alloc>
    struct is_vector_of_arc<std::vector<Arc<U>, Alloc>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_vector_of_arc_v = is_vector_of_arc<T>::value;

    /**
     * @brief Detects whether @c T is @c std::optional<Arc<U>> for some U.
     */
    template <typename T>
    struct is_optional_of_arc : std::false_type
    {
    };

    template <typename U>
    struct is_optional_of_arc<std::optional<Arc<U>>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_optional_of_arc_v =
        is_optional_of_arc<T>::value;

    namespace detail
    {
        inline constexpr char SkirnirEmptyKey[] = "";
    }
} // namespace SKIRNIR_NAMESPACE
