#include <Skirnir/Skirnir.hpp>

#include "gtest/gtest.h"

#include <string>

namespace keyed_test
{
    class IHandler
    {
      public:
        virtual ~IHandler()                  = default;
        virtual std::string Handle() const   = 0;
        virtual std::string Name() const     = 0;
    };

    class AlphaHandler : public IHandler
    {
      public:
        std::string Handle() const override { return "alpha-handled"; }
        std::string Name() const override { return "alpha"; }
    };

    class BetaHandler : public IHandler
    {
      public:
        std::string Handle() const override { return "beta-handled"; }
        std::string Name() const override { return "beta"; }
    };

    inline constexpr char keyAlpha[] = "alpha";
    inline constexpr char keyBeta[]  = "beta";
} // namespace keyed_test

TEST(KeyedServiceSpec, GetKeyedServiceReturnsCorrectImpl)
{
    using namespace keyed_test;
    auto sp = skr::ServiceCollection()
                  .AddKeyedSingleton<IHandler, AlphaHandler>(keyAlpha)
                  .AddKeyedSingleton<IHandler, BetaHandler>(keyBeta)
                  .CreateServiceProvider();

    auto a = sp->GetKeyedService<IHandler>(keyAlpha);
    auto b = sp->GetKeyedService<IHandler>(keyBeta);
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    EXPECT_EQ(a->Name(), "alpha");
    EXPECT_EQ(b->Name(), "beta");
    EXPECT_EQ(a->Handle(), "alpha-handled");
    EXPECT_EQ(b->Handle(), "beta-handled");
}

TEST(KeyedServiceSpec, GetKeyedServiceThrowsForUnknownKey)
{
    using namespace keyed_test;
    auto sp = skr::ServiceCollection()
                  .AddKeyedSingleton<IHandler, AlphaHandler>(keyAlpha)
                  .CreateServiceProvider();

    EXPECT_THROW(sp->GetKeyedService<IHandler>("nope"), std::runtime_error);
}

TEST(KeyedServiceSpec, TryGetKeyedServiceReturnsNulloptForUnknownKey)
{
    using namespace keyed_test;
    auto sp = skr::ServiceCollection()
                  .AddKeyedSingleton<IHandler, AlphaHandler>(keyAlpha)
                  .CreateServiceProvider();

    auto result = sp->TryGetKeyedService<IHandler>("nope");
    EXPECT_FALSE(result.has_value());

    auto hit = sp->TryGetKeyedService<IHandler>(keyAlpha);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ((*hit)->Name(), "alpha");
}

TEST(KeyedServiceSpec, UnkeyedGetServiceStillReturnsFirstRegistration)
{
    using namespace keyed_test;
    auto sp = skr::ServiceCollection()
                  .AddKeyedSingleton<IHandler, AlphaHandler>(keyAlpha)
                  .AddKeyedSingleton<IHandler, BetaHandler>(keyBeta)
                  .CreateServiceProvider();

    // No un-keyed registration exists; the first *keyed* registration is
    // still returned by GetService. This is intentional: keyed entries are
    // simply additional registrations under the same contract id.
    auto direct = sp->GetService<IHandler>();
    ASSERT_TRUE(direct);
    EXPECT_EQ(direct->Name(), "alpha");
}

TEST(KeyedServiceSpec, KeyedTransientReturnsNewInstanceEachTime)
{
    using namespace keyed_test;
    auto sp = skr::ServiceCollection()
                  .AddKeyedTransient<IHandler, AlphaHandler>(keyAlpha)
                  .CreateServiceProvider();

    auto a1 = sp->GetKeyedService<IHandler>(keyAlpha);
    auto a2 = sp->GetKeyedService<IHandler>(keyAlpha);
    ASSERT_TRUE(a1);
    ASSERT_TRUE(a2);
    EXPECT_NE(a1, a2);
}

TEST(KeyedServiceSpec, KeyedInjectionResolvesThroughCtorWrapper)
{
    using namespace keyed_test;
    class Consumer
    {
      public:
        explicit Consumer(skr::Keyed<IHandler, keyAlpha> k) : mHandler(k.ptr)
        {
        }
        std::string Name() const { return mHandler->Name(); }

      private:
        skr::Arc<IHandler> mHandler;
    };

    auto sp = skr::ServiceCollection()
                  .AddKeyedSingleton<IHandler, AlphaHandler>(keyAlpha)
                  .AddKeyedSingleton<IHandler, BetaHandler>(keyBeta)
                  .AddSingleton<Consumer>()
                  .CreateServiceProvider();

    auto consumer = sp->GetService<Consumer>();
    ASSERT_TRUE(consumer);
    EXPECT_EQ(consumer->Name(), "alpha");
}
