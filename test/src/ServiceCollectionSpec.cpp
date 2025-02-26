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
        mServiceCollection = std::make_shared<skr::ServiceCollection>();
    }

    void TearDown() override { mServiceCollection.reset(); }

    std::shared_ptr<skr::ServiceCollection> mServiceCollection;
};

TEST_F(ServiceCollectionSpec, ServiceCollectionShouldAddSingletonByInstance)
{
    mServiceCollection->AddSingleton(std::make_shared<SingletonService>());

    ASSERT_TRUE(mServiceCollection->Contains<SingletonService>());
}

TEST_F(ServiceCollectionSpec, ServiceCollectionShouldBreakWhenAddSingletonTwice)
{
    mServiceCollection->AddSingleton(std::make_shared<SingletonService>());

    ASSERT_DEATH(
        mServiceCollection->AddSingleton(std::make_shared<SingletonService>()),
        "Can't register twice");
}

TEST_F(ServiceCollectionSpec, ServiceCollectionCreateServiceProvider)
{
    mServiceCollection->AddSingleton(std::make_shared<SingletonService>());

    ASSERT_NE(mServiceCollection->CreateServiceProvider(), nullptr);
}
