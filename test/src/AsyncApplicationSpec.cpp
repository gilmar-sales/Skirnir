#include <Skirnir/Async.hpp>
#include <Skirnir/Common/Arc.hpp>
#include <Skirnir/DependencyInjection/ApplicationBuilder.hpp>

#include <atomic>

#include "gtest/gtest.h"

class AsyncAppService
{
};

class CountingAsyncApp : public skr::IAsyncApplication
{
  public:
    using IAsyncApplication::IAsyncApplication;
    explicit CountingAsyncApp(
        const skr::Arc<skr::ServiceProvider> &root) :
        IAsyncApplication(root)
    {
    }

    skr::Task<> RunAsync() override
    {
        mRunCount.fetch_add(1);
        co_return;
    }

    std::atomic<int> mRunCount { 0 };
};

TEST(AsyncApplicationSpec, BuildAsyncResolvesApplication)
{
    auto builder = skr::ApplicationBuilder();
    auto app     = builder.BuildAsync<CountingAsyncApp>();

    EXPECT_TRUE(app);
    EXPECT_EQ(app->mRunCount.load(), 0);
}

TEST(AsyncApplicationSpec, RunAsyncExecutesBody)
{
    auto builder = skr::ApplicationBuilder();
    auto app     = builder.BuildAsync<CountingAsyncApp>();

    skr::AsyncApplicationHost::Run(*app);
    EXPECT_EQ(app->mRunCount.load(), 1);
}

TEST(AsyncApplicationSpec, RequestStopSetsCancellation)
{
    auto builder = skr::ApplicationBuilder();
    auto app     = builder.BuildAsync<CountingAsyncApp>();

    EXPECT_FALSE(app->Cancellation().is_cancellation_requested());
    app->RequestStop();
    EXPECT_TRUE(app->Cancellation().is_cancellation_requested());
}

TEST(AsyncApplicationSpec, RunAsyncCanBeDrivenManually)
{
    auto builder = skr::ApplicationBuilder();
    auto app     = builder.BuildAsync<CountingAsyncApp>();

    auto task = app->RunAsync();
    EXPECT_TRUE(task.valid());
    EXPECT_EQ(app->mRunCount.load(), 1);
}
