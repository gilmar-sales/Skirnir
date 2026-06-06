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
    explicit SimpleApp(const skr::Arc<skr::ServiceProvider>& rootServiceProvider) :
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

    skr::Arc<skr::ServiceProvider> mServiceProvider;
    skr::Arc<SimpleApp>            mApp;
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
    auto transients = std::set<skr::Arc<TransientService>>();

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
    skr::WeakArc<SimpleApp>            app     = mApp;
    skr::WeakArc<skr::ServiceProvider> service = mServiceProvider;

    // Act
    mServiceProvider.reset();
    mApp.reset();

    // Assert
    ASSERT_TRUE(app.expired());
    ASSERT_TRUE(service.expired());
}

class NonExistentService
{
};

class MissingDep
{
  public:
    explicit MissingDep(skr::Arc<NonExistentService>) {}
};

TEST_F(ServiceProviderSpec, ValidateOnBuildSucceedsForWellFormedGraph)
{
    auto sp = skr::ServiceCollection()
                  .AddSingleton<SingletonService>()
                  .CreateServiceProvider();
    EXPECT_NO_THROW(sp->ValidateOnBuild());
}

TEST_F(ServiceProviderSpec, ValidateOnBuildFailsForMissingDependency)
{
    // MissingDep requires NonExistentService, which we deliberately do not
    // register below.
    auto sp = skr::ServiceCollection()
                  .AddSingleton<MissingDep>()
                  .CreateServiceProvider();
    EXPECT_THROW(sp->ValidateOnBuild(), std::runtime_error);
}

TEST_F(ServiceProviderSpec, PrintDiagnosticsContainsExpectedText)
{
    auto sp = skr::ServiceCollection()
                  .AddSingleton<SingletonService>()
                  .AddTransient<TransientService>()
                  .CreateServiceProvider();

    std::ostringstream oss;
    sp->PrintDiagnostics(oss);
    auto output = oss.str();
    EXPECT_NE(output.find("Singleton"), std::string::npos);
    EXPECT_NE(output.find("Transient"), std::string::npos);
    EXPECT_NE(output.find("service#"), std::string::npos);
}

TEST_F(ServiceProviderSpec, TryGetReturnsNulloptForUnregistered)
{
    auto sp = skr::ServiceCollection()
                  .AddSingleton<SingletonService>()
                  .CreateServiceProvider();
    auto result = sp->TryGetService<NonExistentService>();
    EXPECT_FALSE(result.has_value());
}

TEST_F(ServiceProviderSpec, TryGetReturnsValueForRegistered)
{
    auto sp = skr::ServiceCollection()
                  .AddSingleton<SingletonService>()
                  .CreateServiceProvider();
    auto result = sp->TryGetService<SingletonService>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, sp->GetService<SingletonService>());
}

TEST_F(ServiceProviderSpec, TryGetReturnsNulloptForRootScoped)
{
    auto sp = skr::ServiceCollection()
                  .AddScoped<ScopedService>()
                  .CreateServiceProvider();
    // Resolving a Scoped at the root provider is an error; the non-throwing
    // path surfaces it as nullopt.
    auto result = sp->TryGetService<ScopedService>();
    EXPECT_FALSE(result.has_value());
}

namespace captive_test
{
    class NeedsScoped
    {
      public:
        explicit NeedsScoped(skr::Arc<ScopedService>) {}
    };
} // namespace captive_test

TEST_F(ServiceProviderSpec, ValidateOnBuildFlagsCaptiveScopedDependency)
{
    using namespace captive_test;
    auto sp = skr::ServiceCollection()
                  .AddSingleton<NeedsScoped>()
                  .AddScoped<ScopedService>()
                  .CreateServiceProvider();
    EXPECT_THROW(sp->ValidateOnBuild(), std::runtime_error);
}

TEST_F(ServiceProviderSpec, ValidateOnBuildAllowsScopedDependingOnSingleton)
{
    using namespace captive_test;
    // Reverse direction is fine: a Scoped can hold a Singleton.
    class ScopedHolder
    {
      public:
        explicit ScopedHolder(skr::Arc<SingletonService>) {}
    };
    auto sp = skr::ServiceCollection()
                  .AddScoped<ScopedHolder>()
                  .AddSingleton<SingletonService>()
                  .CreateServiceProvider();
    // ValidateOnBuild constructs singletons eagerly but only walks singleton
    // sub-graphs for captive detection, so this passes.
    EXPECT_NO_THROW(sp->ValidateOnBuild());
}

TEST_F(ServiceProviderSpec, ValidateOnBuildIgnoresTransients)
{
    // Singleton -> Transient -> Singleton: the Transient breaks the chain
    // and the inner Singleton is available, so construction succeeds and no
    // captive-dependency error is reported.
    class Inner
    {
    };
    class TransientHolder
    {
      public:
        explicit TransientHolder(skr::Arc<Inner>) {}
    };
    auto sp = skr::ServiceCollection()
                  .AddSingleton<TransientHolder>()
                  .AddTransient<TransientHolder>()
                  .AddSingleton<Inner>()
                  .CreateServiceProvider();
    EXPECT_NO_THROW(sp->ValidateOnBuild());
}
