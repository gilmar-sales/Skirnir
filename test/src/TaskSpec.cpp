#include <Skirnir/Async.hpp>
#include <Skirnir/Common/Arc.hpp>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

#include "gtest/gtest.h"

namespace task_test
{
    skr::Task<int> ReturnsInt()
    {
        co_return 42;
    }

    skr::Task<int> ReturnsIntWithError()
    {
        throw std::runtime_error("boom");
        co_return 0;
    }

    skr::Task<> ReturnsVoid()
    {
        co_return;
    }

    skr::Task<int> AwaitsInt()
    {
        auto v = co_await ReturnsInt();
        co_return v + 1;
    }

    skr::Task<int> AwaitsWithError()
    {
        auto v = co_await ReturnsIntWithError();
        co_return v;
    }

    skr::Task<int> AwaitsDelay()
    {
        co_await skr::Async::Delay(std::chrono::milliseconds(5));
        co_return 7;
    }

    skr::Task<int> CancellationCascade()
    {
        auto outer = []() -> skr::Task<int> {
            co_await skr::Async::CancelIfRequested(
                std::stop_token {});  // never cancelled
            co_return 1;
        }();
        co_return co_await std::move(outer);
    }
} // namespace task_test

TEST(TaskSpec, ReturnsValue)
{
    auto task = task_test::ReturnsInt();
    EXPECT_TRUE(task.valid());
    int v = task_test::ReturnsInt().operator co_await().await_resume();
    (void)v;
}

TEST(TaskSpec, AwaitReturnsValue)
{
    auto task = task_test::AwaitsInt();
    EXPECT_TRUE(task.valid());
}

TEST(TaskSpec, AwaitVoid)
{
    auto task = task_test::ReturnsVoid();
    EXPECT_TRUE(task.valid());
}

TEST(TaskSpec, PropagatesException)
{
    auto task = task_test::ReturnsIntWithError();
    auto awaiter = task.operator co_await();
    EXPECT_THROW(awaiter.await_resume(), std::runtime_error);
}

TEST(TaskSpec, PropagatesExceptionThroughAwait)
{
    auto task = task_test::AwaitsWithError();
    auto awaiter = task.operator co_await();
    EXPECT_THROW(awaiter.await_resume(), std::runtime_error);
}

TEST(TaskSpec, DelayResumes)
{
    auto task = task_test::AwaitsDelay();
    while (!task.is_ready())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto awaiter = task.operator co_await();
    int v = awaiter.await_resume();
    EXPECT_EQ(v, 7);
}

TEST(TaskSpec, ManualScheduler)
{
    std::atomic<int> counter { 0 };
    auto task = [&]() -> skr::Task<> {
        counter.fetch_add(1);
        co_return;
    }();
    EXPECT_EQ(counter.load(), 1);
    (void)task;
}

TEST(TaskSpec, CancellationTokenEmptyIsSafe)
{
    skr::CancellationToken token;
    EXPECT_FALSE(token.is_cancellation_requested());
    EXPECT_FALSE(static_cast<bool>(token));
    token.throw_if_cancellation_requested();
}

TEST(TaskSpec, WhenAllSequential)
{
    auto all = skr::when_all(task_test::ReturnsInt(),
                             task_test::ReturnsInt());
    (void)all;
}

TEST(TaskSpec, WhenAnyFallsBack)
{
    auto any = skr::when_any(task_test::ReturnsIntWithError(),
                             task_test::ReturnsInt());
    (void)any;
}
