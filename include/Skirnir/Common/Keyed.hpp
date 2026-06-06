#pragma once

#include <type_traits>

#include "ConstructorArgumentTraits.hpp"
#include "Namespace.hpp"
#include "Ref.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Tagged wrapper used to select a keyed service at injection time.
     *
     * The @c Key template argument is a NTTP that names the registration key.
     * The user must declare a constexpr character array for the key:
     *
     * @code
     * inline constexpr char keyA[] = "a";
     *
     * class Consumer {
     *     Consumer(Keyed<IFoo, keyA> k) : mFoo(k.ptr) {}
     * };
     * @endcode
     *
     * The @c ptr member is filled by the container's resolution logic; the
     * consumer never needs to call into the provider itself.
     */
    template <typename T, const char* Key = detail::SkirnirEmptyKey>
    struct Keyed
    {
        static constexpr const char* key = Key;
        Ref<T>                        ptr;
    };

    template <typename T>
    struct is_keyed : std::false_type
    {
    };

    template <typename T, const char* K>
    struct is_keyed<Keyed<T, K>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_keyed_v = is_keyed<T>::value;

    template <typename T>
    struct keyed_inner
    {
    };

    template <typename T, const char* K>
    struct keyed_inner<Keyed<T, K>>
    {
        using type = T;
    };

    template <typename T>
    using keyed_inner_t = typename keyed_inner<T>::type;
} // namespace SKIRNIR_NAMESPACE
