#pragma once

#include <memory>

#include "Namespace.hpp"

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T>
using WeakRef = std::weak_ptr<T>;

namespace SKIRNIR_NAMESPACE
{
    template <typename T, typename... TArgs>
        requires(std::is_constructible_v<T, TArgs...>)
    inline Ref<T> MakeRef(TArgs&&... args)
    {
        return std::make_shared<T>(std::forward<TArgs>(args)...);
    }

    /**
     * @brief Performs a static pointer cast on a shared_ptr.
     *
     * @tparam TDest  The target type to cast to
     * @tparam TSource The source type being cast from
     * @param source  The shared_ptr to cast
     * @return        A shared_ptr of type TDest, or nullptr if cast fails
     */
    template <typename TDest, typename TSource>
    inline Ref<TDest> RefCast(const Ref<TSource>& source)
    {
        return std::static_pointer_cast<TDest>(source);
    }
} // namespace SKIRNIR_NAMESPACE
