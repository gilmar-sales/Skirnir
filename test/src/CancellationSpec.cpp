#include <Skirnir/Async.hpp>

#include <chrono>
#include <stdexcept>

#include "gtest/gtest.h"

namespace cancel_test
{
    skr::Task<int> AwaitingDelay()
    {
        co_await skr::Async::DelayFor(std::chrono::milliseconds(60));
        co_return 1;
    }
} // namespace cancel_test

TEST(CancellationSpec, EmptyTokenIsNotCancellable)
{
    skr::CancellationToken token;
    EXPECT_FALSE(token.is_cancellation_requested());
}

TEST(CancellationSpec, StopSourceTokenReportsRequested)
{
    std::stop_source src;
    skr::CancellationToken token { src.get_token() };
    EXPECT_FALSE(token.is_cancellation_requested());
    src.request_stop();
    EXPECT_TRUE(token.is_cancellation_requested());
}

TEST(CancellationSpec, ThrowIfCancellationRequested)
{
    std::stop_source src;
    src.request_stop();
    skr::CancellationToken token { src.get_token() };
    EXPECT_THROW(token.throw_if_cancellation_requested(), std::system_error);
}

TEST(CancellationSpec, CancelIfRequestedDoesNotThrowWhenNotCancelled)
{
    auto task = []() -> skr::Task<int> {
        co_await skr::Async::CancelIfRequested(std::stop_token {});
        co_return 1;
    }();
    (void)task;
}

TEST(CancellationSpec, CancelIfRequestedThrowsWhenCancelled)
{
    std::stop_source src;
    auto task = [&]() -> skr::Task<int> {
        co_await skr::Async::CancelIfRequested(src.get_token());
        co_return 1;
    }();
    src.request_stop();
    (void)task;
}
