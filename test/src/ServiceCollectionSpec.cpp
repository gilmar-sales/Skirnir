#include <Skirnir/Skirnir.hpp>

#include "gtest/gtest.h"

class SingletonService
{
};

class ServiceCollectionSpec : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mServiceCollection = skr::MakeArc<skr::ServiceCollection>();
    }

    void TearDown() override { mServiceCollection.reset(); }

    skr::Arc<skr::ServiceCollection> mServiceCollection;
};

TEST_F(ServiceCollectionSpec, ServiceCollectionShouldAddSingletonByInstance)
{
    mServiceCollection->AddSingleton(skr::MakeArc<SingletonService>());

    ASSERT_TRUE(mServiceCollection->Contains<SingletonService>());
}

TEST_F(ServiceCollectionSpec, ServiceCollectionShouldCreateServiceProvider)
{
    mServiceCollection->AddSingleton(skr::MakeArc<SingletonService>());

    ASSERT_NE(mServiceCollection->CreateServiceProvider(), nullptr);
}
