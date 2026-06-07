#pragma once

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>

#include "Skirnir/Async/AsyncApplication.hpp"
#include "Skirnir/Async/AsyncApplicationHost.hpp"
#include "Skirnir/Async/Cancellation.hpp"
#include "Skirnir/Async/Scheduler.hpp"
#include "Skirnir/Async/Task.hpp"
#include "Skirnir/Async/WhenAll.hpp"
#include "Skirnir/Common/Namespace.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Helpers for working with @c Task<>'s.
     */
    namespace Async
    {
        /**
         * @brief Suspends the current task for at least @p duration.
         *
         * The coroutine resumes on a worker thread. Cancellation via
         * the task's @c std::stop_token is respected — if cancellation
         * is requested during the wait, the task resumes early and
         * throws @c std::system_error with @c errc::operation_canceled
         * on @c await_resume.
         */
        struct DelayAwaitable
        {
            std::chrono::milliseconds duration;
            std::stop_token           token;

            bool await_ready() const noexcept { return duration.count() <= 0; }

            void await_suspend(std::coroutine_handle<> h)
            {
                if (token.stop_requested())
                {
                    h.resume();
                    return;
                }
                auto d   = duration;
                auto t   = token;
                auto fut = std::async(std::launch::async, [d]() {
                    std::this_thread::sleep_for(d);
                });
                std::thread([h, fut = std::move(fut), t]() mutable {
                    fut.get();
                    if (t.stop_requested())
                    {
                        h.resume();
                        return;
                    }
                    h.resume();
                }).detach();
            }

            void await_resume() const
            {
                if (token.stop_requested())
                    throw std::system_error(
                        std::make_error_code(std::errc::operation_canceled));
            }
        };

        inline DelayAwaitable Delay(std::chrono::milliseconds duration,
                                    std::stop_token           token = {})
        {
            return DelayAwaitable { duration, token };
        }

        inline DelayAwaitable DelayFor(std::chrono::milliseconds duration,
                                       CancellationToken         token = {})
        {
            return DelayAwaitable { duration, token.stop_token() };
        }

        /**
         * @brief Awaits a value: when the task is cancelled, throws.
         */
        struct CancelIfRequestedAwaitable
        {
            std::stop_token token;

            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<> h) const
            {
                if (token.stop_requested())
                    h.resume();
            }

            void await_resume() const
            {
                if (token.stop_requested())
                    throw std::system_error(
                        std::make_error_code(std::errc::operation_canceled));
            }
        };

        inline CancelIfRequestedAwaitable CancelIfRequested(
            std::stop_token token)
        {
            return CancelIfRequestedAwaitable { token };
        }
    } // namespace Async
} // namespace SKIRNIR_NAMESPACE
