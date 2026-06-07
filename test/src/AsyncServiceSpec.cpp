#include <Skirnir/Async.hpp>
#include <Skirnir/Common/Arc.hpp>
#include <Skirnir/DependencyInjection/ApplicationBuilder.hpp>
#include <Skirnir/DependencyInjection/ServiceScope.hpp>

#include <atomic>

#include "gtest/gtest.h"

class TestSingleton
{
};

class TestScoped
{
  public:
    std::atomic<int>& tag;

    explicit TestScoped(std::atomic<int>& t) : tag(t) { tag.fetch_add(1); }
};

class TestTransient
{
};

class AsyncScopeApp : public skr::IAsyncApplication
{
  public:
    explicit AsyncScopeApp(const skr::Arc<skr::ServiceProvider>& root) :
        IAsyncApplication(root)
    {
    }

    skr::Task<> RunAsync() override
    {
        auto singleton =
            co_await mRootServiceProvider->GetServiceAsync<TestSingleton>();
        EXPECT_TRUE(singleton);

        auto scope = mRootServiceProvider->CreateServiceScope();
        auto scoped =
            co_await scope->GetServiceProvider()->GetServiceAsync<TestScoped>();
        EXPECT_TRUE(scoped);

        auto t =
            co_await mRootServiceProvider->GetServiceAsync<TestTransient>();
        EXPECT_TRUE(t);
        co_return;
    }
};

TEST(AsyncServiceSpec, GetServiceAsyncResolvesSingleton)
{
    auto builder = skr::ApplicationBuilder();
    builder.GetServiceCollection()->AddSingleton<TestSingleton>();

    auto app = builder.BuildAsync<AsyncScopeApp>();
    skr::AsyncApplicationHost::Run(*app);
}

TEST(AsyncServiceSpec, GetServiceAsyncResolvesScoped)
{
    auto             builder = skr::ApplicationBuilder();
    std::atomic<int> tag { 0 };
    builder.GetServiceCollection()
        ->AddSingleton<TestSingleton>()
        .AddTransient<TestTransient>()
        .AddScoped<TestScoped>([&tag](skr::ServiceProvider&) {
            return skr::MakeArc<TestScoped>(tag);
        });

    auto app = builder.BuildAsync<AsyncScopeApp>();
    skr::AsyncApplicationHost::Run(*app);
    EXPECT_GE(tag.load(), 1);
}

TEST(AsyncServiceSpec, GetServiceAsyncResolvesTransient)
{
    auto builder = skr::ApplicationBuilder();
    builder.GetServiceCollection()->AddTransient<TestTransient>();

    auto app = builder.BuildAsync<AsyncScopeApp>();
    skr::AsyncApplicationHost::Run(*app);
}
