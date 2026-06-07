#pragma once

#include <coroutine>
#include <type_traits>
#include <utility>

#include "Skirnir/Common/Namespace.hpp"

namespace SKIRNIR_NAMESPACE
{
    namespace detail
    {
        inline constexpr struct InlineScheduleTag
        {
        } inlineSchedule;
    } // namespace detail

    /**
     * @brief Minimal scheduler concept used by @ref Task.
     *
     * A @c Scheduler is anything callable as
     * @code
     * s.schedule(std::coroutine_handle<>);
     * @endcode
     * The call must be non-throwing and is responsible for resuming the
     * coroutine on whichever thread/executor is appropriate. An
     * implementation that runs the coroutine synchronously on the
     * current thread is valid (see @ref InlineScheduler).
     *
     * This is intentionally narrower than @c std::execution::scheduler
     * (C++26 @c <execution>) so the core @c Task header has no
     * dependency on the standard executors.
     */
    template <typename S>
    concept Scheduler = requires(S s, std::coroutine_handle<> h) {
        { s.schedule(h) } noexcept;
    };

    /**
     * @brief Scheduler that resumes a coroutine synchronously on the
     *        current thread.
     *
     * Useful for unit tests, single-threaded applications, and for
     * resuming after operations that already happen on the correct
     * thread.
     */
    struct InlineScheduler
    {
        void schedule(std::coroutine_handle<> h) const noexcept
        {
            if (h)
                h.resume();
        }
    };

    namespace detail
    {
        struct SchedulerRef
        {
            void (*schedule)(void*, std::coroutine_handle<>) noexcept = nullptr;
            void* state                                               = nullptr;

            void operator()(std::coroutine_handle<> h) const noexcept
            {
                schedule(state, h);
            }

            bool empty() const noexcept { return schedule == nullptr; }
        };

        template <Scheduler S>
        SchedulerRef makeSchedulerRef(S& s) noexcept
        {
            return SchedulerRef {
                +[](void* state, std::coroutine_handle<> h) noexcept {
                    (*static_cast<S*>(state)).schedule(h);
                },
                std::addressof(s)
            };
        }

        template <Scheduler S>
        SchedulerRef makeSchedulerRef(const S& s) noexcept
        {
            return SchedulerRef {
                +[](void* state, std::coroutine_handle<> h) noexcept {
                    (*static_cast<const S*>(state)).schedule(h);
                },
                const_cast<S*>(std::addressof(s))
            };
        }

        inline SchedulerRef makeInlineSchedulerRef() noexcept
        {
            static InlineScheduler s;
            return makeSchedulerRef(s);
        }
    } // namespace detail
} // namespace SKIRNIR_NAMESPACE
