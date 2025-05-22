#include <Skirnir/Skirnir.hpp>
#include <unordered_set>

#include "gtest/gtest.h"

class SingletonService
{
};

class ScopedService
{
};

class TransientService
{
};

class ServiceProviderSpec : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto serviceCollection =
            skr::ServiceCollection()
                .AddSingleton<SingletonService>()
                .AddScoped<ScopedService>()
                .AddTransient<TransientService>();

        mServiceProvider = serviceCollection.CreateServiceProvider();
    }

    void TearDown() override { mServiceProvider.reset(); }

    Ref<skr::ServiceProvider> mServiceProvider;
};

TEST_F(ServiceProviderSpec, ServiceProviderShouldGetSingleton)
{
    ASSERT_NE(mServiceProvider->GetService<SingletonService>(), nullptr);
}

TEST_F(ServiceProviderSpec, ServiceProviderShouldGetSameSingletonAtAnyTime)
{
    for (int i = 0; i < 10000; ++i)
    {
        ASSERT_EQ(mServiceProvider->GetService<SingletonService>(),
                  mServiceProvider->GetService<SingletonService>());
    }
}

TEST_F(ServiceProviderSpec, ServiceProviderShouldGetTransient)
{
    ASSERT_NE(mServiceProvider->GetService<TransientService>(), nullptr);
}

TEST_F(ServiceProviderSpec, ServiceProviderShouldGetItSelf)
{
    ASSERT_NE(mServiceProvider->GetService<skr::ServiceProvider>(), nullptr);
    ASSERT_EQ(mServiceProvider->GetService<skr::ServiceProvider>(),
              mServiceProvider);
}

TEST_F(ServiceProviderSpec,
       ServiceProviderShouldGetDifferentTransientsAtAnyTime)
{
    auto transients = std::unordered_set<std::shared_ptr<TransientService>>();

    for (int i = 0; i < 10000; ++i)
    {
        auto transientService =
            mServiceProvider->GetService<TransientService>();
        ASSERT_FALSE(transients.contains(transientService));
        transients.insert(transientService);
    }
}

TEST_F(ServiceProviderSpec, RootServiceProviderShouldBreakWhenGetScoped)
{
    ASSERT_ANY_THROW(mServiceProvider->GetService<ScopedService>());
}
