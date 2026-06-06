#include <Skirnir/Skirnir.hpp>

#include "gtest/gtest.h"

#include <atomic>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace arc_test
{
    struct Counter
    {
        static std::atomic<int>& Instances()
        {
            static std::atomic<int> n { 0 };
            return n;
        }
    };

    struct Widget : Counter
    {
        int  id;
        bool disposed = false;
        explicit Widget(int v = 0) : id(v) { Instances().fetch_add(1); }
        Widget(Widget&& other) noexcept : id(other.id)
        {
            other.id = -1;
            Instances().fetch_add(1);
        }
        ~Widget() { Instances().fetch_sub(1); }
    };

    struct Base
    {
        virtual ~Base()                  = default;
        virtual std::string Name() const = 0;
    };

    struct DerivedA : Base
    {
        std::string Name() const override { return "A"; }
    };

    struct IFoo
    {
        virtual ~IFoo()                  = default;
        virtual std::string Name() const = 0;
    };

    struct FooA : IFoo
    {
        std::string Name() const override { return "A"; }
    };

    struct FooB : IFoo
    {
        std::string Name() const override { return "B"; }
    };

    struct ILogger
    {
        virtual ~ILogger()              = default;
        virtual std::string Tag() const = 0;
    };

    struct ConsoleLogger : ILogger
    {
        std::string Tag() const override { return "console"; }
    };

    struct SingleConsumer
    {
        skr::Arc<IFoo> mFoo;
        explicit SingleConsumer(skr::Arc<IFoo> foo) : mFoo(std::move(foo)) {}
        std::string Name() const { return mFoo->Name(); }
    };

    struct VectorConsumer
    {
        std::vector<skr::Arc<IFoo>> mFoos;
        explicit VectorConsumer(std::vector<skr::Arc<IFoo>> foos) :
            mFoos(std::move(foos))
        {
        }
        std::set<std::string> Names() const
        {
            std::set<std::string> result;
            for (const auto& f : mFoos)
                result.insert(f->Name());
            return result;
        }
    };

    struct OptionalConsumer
    {
        std::optional<skr::Arc<ILogger>> mLogger;
        explicit OptionalConsumer(std::optional<skr::Arc<ILogger>> logger) :
            mLogger(std::move(logger))
        {
        }
        bool        Has() const { return mLogger.has_value(); }
        std::string Tag() const
        {
            return mLogger.has_value() ? (*mLogger)->Tag() : "none";
        }
    };
} // namespace arc_test

TEST(ArcSpec, MakeArcConstructsAndDestroys)
{
    using namespace arc_test;
    Counter::Instances().store(0);
    {
        auto w = skr::MakeArc<Widget>(7);
        EXPECT_EQ(Counter::Instances().load(), 1);
        EXPECT_EQ(w->id, 7);
        EXPECT_EQ(w.use_count(), 1u);
    }
    EXPECT_EQ(Counter::Instances().load(), 0);
}

TEST(ArcSpec, CopyAndMovePreserveIdentity)
{
    using namespace arc_test;
    auto a = skr::MakeArc<Widget>(1);
    auto b = a;
    EXPECT_EQ(a.get(), b.get());
    EXPECT_EQ(a.use_count(), 2u);
    auto c = std::move(b);
    EXPECT_EQ(a.get(), c.get());
    EXPECT_EQ(a.use_count(), 2u);
    EXPECT_FALSE(b);
}

TEST(ArcSpec, ResetReleasesObject)
{
    using namespace arc_test;
    Counter::Instances().store(0);
    auto a = skr::MakeArc<Widget>(1);
    EXPECT_EQ(Counter::Instances().load(), 1);
    a.reset();
    EXPECT_FALSE(a);
    EXPECT_EQ(Counter::Instances().load(), 0);
}

TEST(ArcSpec, ArcCastPerformsStaticDowncast)
{
    using namespace arc_test;
    auto      derived = skr::MakeArc<DerivedA>();
    skr::Arc<Base> base    = derived;
    auto      a       = skr::ArcCast<DerivedA>(base);
    static_assert(std::is_same_v<decltype(a), skr::Arc<DerivedA>>,
                  "ArcCast<TDest> should yield Arc<TDest>");
    EXPECT_EQ(a->Name(), "A");
}

TEST(ArcSpec, WeakArcExpiresWhenLastArcIsDestroyed)
{
    using namespace arc_test;
    skr::WeakArc<Widget> w;
    {
        auto a = skr::MakeArc<Widget>(42);
        w      = a;
        EXPECT_FALSE(w.expired());
        EXPECT_EQ(w.use_count(), 1u);
    }
    EXPECT_TRUE(w.expired());
    auto locked = w.lock();
    EXPECT_FALSE(locked);
}

TEST(ArcSpec, WeakArcLockReturnsLiveArc)
{
    using namespace arc_test;
    auto            a      = skr::MakeArc<Widget>(7);
    skr::WeakArc<Widget> w      = a;
    auto            locked = w.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked.get(), a.get());
    EXPECT_EQ(locked.use_count(), 2u);
}

TEST(ArcSpec, ConcurrentCopyAndDestroyDoesNotCrash)
{
    using namespace arc_test;
    auto shared = skr::MakeArc<int>(123);

    constexpr int            kThreads = 8;
    constexpr int            kIters   = 1000;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
    {
        threads.emplace_back([shared]() mutable {
            for (int i = 0; i < kIters; ++i)
            {
                auto local = shared;
                EXPECT_EQ(*local, 123);
            }
        });
    }
    for (auto& th : threads)
        th.join();
    EXPECT_EQ(*shared, 123);
    EXPECT_EQ(shared.use_count(), 1u);
}

TEST(ArcSpec, ArcCopyPreservesLifetime)
{
    using namespace arc_test;
    auto raw       = skr::MakeArc<Widget>(11);
    auto storedRaw = raw;
    {
        auto a = skr::Arc<Widget>(raw);
        raw.reset();
        EXPECT_EQ(a->id, 11);
    }
    ASSERT_TRUE(storedRaw);
    EXPECT_EQ(storedRaw->id, 11);
}

TEST(ArcSpec, DiSingleInjectArcDependency)
{
    using namespace arc_test;
    auto sp = skr::ServiceCollection()
                  .AddSingleton<IFoo, FooA>()
                  .AddSingleton<SingleConsumer>()
                  .CreateServiceProvider();

    auto consumer = sp->GetService<SingleConsumer>();
    ASSERT_TRUE(consumer);
    EXPECT_EQ(consumer->Name(), "A");
}

TEST(ArcSpec, DiVectorInjectArcDependencies)
{
    using namespace arc_test;
    auto sp = skr::ServiceCollection()
                  .AddTransient<IFoo, FooA>()
                  .AddTransient<IFoo, FooB>()
                  .AddTransient<VectorConsumer>()
                  .CreateServiceProvider();

    auto consumer = sp->GetService<VectorConsumer>();
    ASSERT_TRUE(consumer);
    EXPECT_EQ(consumer->Names(), (std::set<std::string> { "A", "B" }));
}

TEST(ArcSpec, DiOptionalInjectArcDependencyPresent)
{
    using namespace arc_test;
    auto sp = skr::ServiceCollection()
                  .AddSingleton<ILogger, ConsoleLogger>()
                  .AddSingleton<OptionalConsumer>()
                  .CreateServiceProvider();

    auto consumer = sp->GetService<OptionalConsumer>();
    ASSERT_TRUE(consumer);
    EXPECT_TRUE(consumer->Has());
    EXPECT_EQ(consumer->Tag(), "console");
}

TEST(ArcSpec, DiOptionalInjectArcDependencyAbsent)
{
    using namespace arc_test;
    auto sp = skr::ServiceCollection()
                  .AddSingleton<OptionalConsumer>()
                  .CreateServiceProvider();

    auto consumer = sp->GetService<OptionalConsumer>();
    ASSERT_TRUE(consumer);
    EXPECT_FALSE(consumer->Has());
    EXPECT_EQ(consumer->Tag(), "none");
}
