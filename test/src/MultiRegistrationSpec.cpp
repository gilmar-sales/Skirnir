#include <Skirnir/Skirnir.hpp>

#include "gtest/gtest.h"

#include <set>
#include <string>
#include <vector>

class IFoo
{
  public:
    virtual ~IFoo()                  = default;
    virtual std::string Name() const = 0;
};

class FooA : public IFoo
{
  public:
    std::string Name() const override { return "A"; }
};

class FooB : public IFoo
{
  public:
    std::string Name() const override { return "B"; }
};

class FooC : public IFoo
{
  public:
    std::string Name() const override { return "C"; }
};

class Consumer
{
  public:
    explicit Consumer(std::vector<skr::Arc<IFoo>> foos) : mFoos(std::move(foos)) {}

    std::set<std::string> Names() const
    {
        std::set<std::string> result;
        for (const auto& f : mFoos)
            result.insert(f->Name());
        return result;
    }

  private:
    std::vector<skr::Arc<IFoo>> mFoos;
};

TEST(MultiRegistrationSpec, GetServicesReturnsAllRegistrations)
{
    auto sp = skr::ServiceCollection()
                  .AddTransient<IFoo, FooA>()
                  .AddTransient<IFoo, FooB>()
                  .AddTransient<IFoo, FooC>()
                  .CreateServiceProvider();

    auto foos = sp->GetServices<IFoo>();
    EXPECT_EQ(foos.size(), 3u);

    std::set<std::string> names;
    for (const auto& f : foos)
        names.insert(f->Name());
    EXPECT_EQ(names, (std::set<std::string> { "A", "B", "C" }));
}

TEST(MultiRegistrationSpec, ConsumerReceivesAllRegistrations)
{
    auto sp = skr::ServiceCollection()
                  .AddTransient<IFoo, FooA>()
                  .AddTransient<IFoo, FooB>()
                  .AddTransient<Consumer>()
                  .CreateServiceProvider();

    auto consumer = sp->GetService<Consumer>();
    ASSERT_TRUE(consumer);
    EXPECT_EQ(consumer->Names(), (std::set<std::string> { "A", "B" }));
}

TEST(MultiRegistrationSpec, SingletonMultiRegistrationFirstWins)
{
    // For singletons, the first registration wins (matches .NET behavior).
    auto sp = skr::ServiceCollection()
                  .AddSingleton<IFoo, FooA>()
                  .AddSingleton<IFoo, FooB>()
                  .CreateServiceProvider();

    auto direct = sp->GetService<IFoo>();
    ASSERT_TRUE(direct);
    EXPECT_EQ(direct->Name(), "A");

    // GetServices returns all registrations, but singleton dedup means
    // the same instance is returned for each.
    auto foos = sp->GetServices<IFoo>();
    EXPECT_GE(foos.size(), 1u);
    for (const auto& f : foos)
    {
        EXPECT_EQ(f, direct);
    }
}

TEST(MultiRegistrationSpec, SingleGetReturnsFirstRegistration)
{
    auto sp = skr::ServiceCollection()
                  .AddTransient<IFoo, FooA>()
                  .AddTransient<IFoo, FooB>()
                  .CreateServiceProvider();

    auto first = sp->GetService<IFoo>();
    ASSERT_TRUE(first);
    EXPECT_EQ(first->Name(), "A");
}
