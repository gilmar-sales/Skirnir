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

class ServiceScopeSpec : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const auto serviceCollection =
            skr::ServiceCollection()
                .AddSingleton<SingletonService>()
                .AddScoped<ScopedService>()
                .AddTransient<TransientService>();

        mRootServiceProvider = serviceCollection.CreateServiceProvider();
        mServiceScope        = mRootServiceProvider->CreateServiceScope();
    }

    void TearDown() override { mServiceScope.reset(); }

    std::shared_ptr<skr::ServiceScope>    mServiceScope;
    std::shared_ptr<skr::ServiceProvider> mRootServiceProvider;
};

TEST_F(ServiceScopeSpec, ServiceScopeShouldGetSingleton)
{
    ASSERT_NE(
        mServiceScope->GetServiceProvider()->GetService<SingletonService>(),
        nullptr);
}

TEST_F(ServiceScopeSpec, ServiceScopeShouldGetTransient)
{
    ASSERT_NE(
        mServiceScope->GetServiceProvider()->GetService<TransientService>(),
        nullptr);
}

TEST_F(ServiceScopeSpec, ServiceScopeShouldGetScoped)
{
    ASSERT_NE(mServiceScope->GetServiceProvider()->GetService<ScopedService>(),
              nullptr);
}
