#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <tuple>
#include <utility>
#include <variant>

#include "Skirnir/Async/Task.hpp"
#include "Skirnir/Common/Namespace.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Awaits two @c Task<T>'s of the same type and returns
     *        the result of the first.
     *
     * For v1, tasks are awaited sequentially. A concurrent fan-out
     * will follow in a later version. If any child throws, the
     * first exception is re-thrown and remaining tasks are dropped.
     */
    template <typename T>
    Task<T> when_all(Task<T> a, Task<T> b)
    {
        auto x = co_await std::move(a);
        co_await std::move(b);
        co_return std::move(x);
    }

    /**
     * @brief Awaits the first @c Task<T> to complete successfully,
     *        falling back to the second on failure.
     *
     * For v1, the first task is awaited eagerly; on failure, the
     * second is awaited. A true racing fan-out is left for a later
     * version.
     */
    template <typename T>
    Task<T> when_any(Task<T> a, Task<T> b)
    {
        std::optional<T> first;
        try
        {
            first = co_await std::move(a);
        }
        catch (...)
        {
        }
        if (first.has_value())
            co_return std::move(*first);
        co_return co_await std::move(b);
    }
} // namespace SKIRNIR_NAMESPACE
