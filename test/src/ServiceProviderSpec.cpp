#include <Skirnir/Skirnir.hpp>

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

class SimpleApp : public skr::IApplication
{
  public:
    explicit SimpleApp(const Ref<skr::ServiceProvider>& rootServiceProvider) :
        IApplication(rootServiceProvider)
    {
    }

    void Run() override {}
};

class ServiceProviderSpec : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto builder = skr::ApplicationBuilder();
        builder.GetServiceCollection()
            ->AddSingleton<SingletonService>()
            .AddScoped<ScopedService>()
            .AddTransient<TransientService>();

        mApp = builder.Build<SimpleApp>();

        mServiceProvider = mApp->GetRootServiceProvider();
    }

    void TearDown() override { mServiceProvider.reset(); }

    Ref<skr::ServiceProvider> mServiceProvider;
    Ref<SimpleApp>            mApp;
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
    auto transients = std::set<Ref<TransientService>>();

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
    EXPECT_ANY_THROW(mServiceProvider->GetService<ScopedService>());
}

TEST_F(ServiceProviderSpec, ServiceProviderShouldClear)
{
    // Arrange
    WeakRef app     = mApp;
    WeakRef service = mServiceProvider;

    // Act
    mServiceProvider.reset();
    mApp.reset();

    // Assert
    ASSERT_TRUE(app.expired());
    ASSERT_TRUE(service.expired());
}
