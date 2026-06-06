#pragma once

#include <optional>
#include <type_traits>
#include <vector>

#include "Namespace.hpp"
#include "Ref.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Detects whether @c T is @c std::vector<Ref<U>> for some U.
     */
    template <typename T>
    struct is_vector_of_ref : std::false_type
    {
    };

    template <typename U, typename Alloc>
    struct is_vector_of_ref<std::vector<Ref<U>, Alloc>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_vector_of_ref_v = is_vector_of_ref<T>::value;

    /**
     * @brief Detects whether @c T is @c std::optional<Ref<U>> for some U.
     */
    template <typename T>
    struct is_optional_of_ref : std::false_type
    {
    };

    template <typename U>
    struct is_optional_of_ref<std::optional<Ref<U>>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_optional_of_ref_v = is_optional_of_ref<T>::value;

    namespace detail
    {
        inline constexpr char SkirnirEmptyKey[] = "";
    }
} // namespace SKIRNIR_NAMESPACE
