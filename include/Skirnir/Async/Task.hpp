#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
#include <optional>
#include <stop_token>
#include <system_error>
#include <type_traits>
#include <utility>

#include "Skirnir/Async/Cancellation.hpp"
#include "Skirnir/Async/Scheduler.hpp"
#include "Skirnir/Common/Namespace.hpp"

namespace SKIRNIR_NAMESPACE
{
    template <typename T>
    class Task;

    namespace detail
    {
        struct TaskFrameBase
        {
            std::stop_source         stop;
            std::coroutine_handle<>  continuation = nullptr;
            detail::SchedulerRef     scheduler;
            std::atomic<std::size_t> refcount { 1 };

            void retain() noexcept
            {
                refcount.fetch_add(1, std::memory_order_relaxed);
            }

            bool release() noexcept
            {
                return refcount.fetch_sub(1, std::memory_order_acq_rel) == 1;
            }
        };

        struct FinalAwaiter
        {
            TaskFrameBase* frame;

            bool await_ready() const noexcept { return false; }

            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<>) const noexcept
            {
                if (frame->continuation)
                    return frame->continuation;
                return std::noop_coroutine();
            }

            void await_resume() const noexcept {}
        };

        template <typename T>
        struct TaskFrame;

        template <typename T>
        struct TaskFrame : TaskFrameBase
        {
            std::optional<T>   value;
            std::exception_ptr error = nullptr;

            Task<T> get_return_object() noexcept;

            std::suspend_never initial_suspend() noexcept { return {}; }

            FinalAwaiter final_suspend() noexcept
            {
                return FinalAwaiter { this };
            }

            template <typename U>
                requires std::is_constructible_v<T, U&&>
            void return_value(U&& v) noexcept(
                std::is_nothrow_constructible_v<T, U&&>)
            {
                try
                {
                    value.emplace(std::forward<U>(v));
                }
                catch (...)
                {
                    error = std::current_exception();
                }
            }

            void unhandled_exception() noexcept
            {
                error = std::current_exception();
            }
        };

        template <>
        struct TaskFrame<void> : TaskFrameBase
        {
            std::exception_ptr error = nullptr;

            Task<void> get_return_object() noexcept;

            std::suspend_never initial_suspend() noexcept { return {}; }

            FinalAwaiter final_suspend() noexcept
            {
                return FinalAwaiter { this };
            }

            void return_void() noexcept {}

            void unhandled_exception() noexcept
            {
                error = std::current_exception();
            }
        };
    } // namespace detail

    template <typename T>
    struct TaskAwaiter
    {
        std::coroutine_handle<typename Task<T>::promise_type> handle;
        Task<T>                                               task;

        bool await_ready() const noexcept { return !handle || handle.done(); }

        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<> awaiter) noexcept
        {
            auto& frame        = handle.promise();
            frame.continuation = awaiter;
            if (!frame.scheduler.empty())
            {
                auto sched = frame.scheduler;
                sched(handle);
            }
            else
            {
                handle.resume();
            }
            return std::noop_coroutine();
        }

        decltype(auto) await_resume()
        {
            auto& p = handle.promise();
            if (p.error)
                std::rethrow_exception(p.error);
            if constexpr (!std::is_void_v<T>)
            {
                return std::move(*p.value);
            }
        }
    };

    template <typename T = void>
    class Task
    {
      public:
        using promise_type = detail::TaskFrame<T>;
        using value_type   = T;

        Task() noexcept = default;

        Task(const Task& other) noexcept : mHandle(other.mHandle)
        {
            if (mHandle)
                mHandle.promise().retain();
        }

        Task(Task&& other) noexcept : mHandle(other.mHandle)
        {
            other.mHandle = nullptr;
        }

        Task& operator=(const Task& other) noexcept
        {
            if (this != &other)
            {
                release();
                mHandle = other.mHandle;
                if (mHandle)
                    mHandle.promise().retain();
            }
            return *this;
        }

        Task& operator=(Task&& other) noexcept
        {
            if (this != &other)
            {
                release();
                mHandle       = other.mHandle;
                other.mHandle = nullptr;
            }
            return *this;
        }

        ~Task() { release(); }

        [[nodiscard]] bool is_ready() const noexcept
        {
            return !mHandle || mHandle.done();
        }

        [[nodiscard]] bool valid() const noexcept { return mHandle != nullptr; }

        [[nodiscard]] CancellationToken cancellation() const noexcept
        {
            if (!mHandle)
                return CancellationToken {};
            return CancellationToken { mHandle.promise().stop.get_token() };
        }

        std::coroutine_handle<promise_type> handle() const noexcept
        {
            return mHandle;
        }

        void request_stop() noexcept
        {
            if (mHandle)
                mHandle.promise().stop.request_stop();
        }

        TaskAwaiter<T> operator co_await() const& noexcept;

        TaskAwaiter<T> operator co_await() && noexcept;

      private:
        template <typename U>
        friend struct detail::TaskFrame;
        friend struct detail::TaskFrame<void>;

        template <typename, typename>
        friend struct WhenAllAwaiter;

        template <typename>
        friend struct WhenAnyAwaiter;

        explicit Task(std::coroutine_handle<promise_type> handle) noexcept :
            mHandle(handle)
        {
        }

        void release() noexcept
        {
            if (mHandle)
            {
                if (mHandle.promise().release())
                {
                    mHandle.destroy();
                }
                mHandle = nullptr;
            }
        }

        std::coroutine_handle<promise_type> mHandle = nullptr;
    };

    template <typename T>
    inline TaskAwaiter<T> Task<T>::operator co_await() const& noexcept
    {
        return TaskAwaiter<T> { mHandle, *this };
    }

    template <typename T>
    inline TaskAwaiter<T> Task<T>::operator co_await() && noexcept
    {
        return TaskAwaiter<T> { mHandle, std::move(*this) };
    }

    namespace detail
    {
        template <typename T>
        inline Task<T> TaskFrame<T>::get_return_object() noexcept
        {
            return Task<T>(
                std::coroutine_handle<TaskFrame<T>>::from_promise(*this));
        }

        inline Task<void> TaskFrame<void>::get_return_object() noexcept
        {
            return Task<void>(
                std::coroutine_handle<TaskFrame<void>>::from_promise(*this));
        }
    } // namespace detail

    /**
     * @brief Detaches a Task so its coroutine is allowed to run to
     *        completion on its own.
     *
     * Used to write fire-and-forget background work.
     */
    struct Detached
    {
    };
} // namespace SKIRNIR_NAMESPACE
