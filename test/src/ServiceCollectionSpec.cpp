#include "gtest/gtest.h"

import Skirnir;

class SingletonService
{
};

class ServiceCollectionSpec : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mServiceCollection = skr::MakeRef<skr::ServiceCollection>();
    }

    void TearDown() override { mServiceCollection.reset(); }

    Ref<skr::ServiceCollection> mServiceCollection;
};

TEST_F(ServiceCollectionSpec, ServiceCollectionShouldAddSingletonByInstance)
{
    mServiceCollection->AddSingleton(skr::MakeRef<SingletonService>());

    ASSERT_TRUE(mServiceCollection->Contains<SingletonService>());
}

TEST_F(ServiceCollectionSpec, ServiceCollectionCreateServiceProvider)
{
    mServiceCollection->AddSingleton(skr::MakeRef<SingletonService>());

    ASSERT_NE(mServiceCollection->CreateServiceProvider(), nullptr);
}
