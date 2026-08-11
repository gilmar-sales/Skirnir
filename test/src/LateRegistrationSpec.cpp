#include <Skirnir/Skirnir.hpp>

#include "gtest/gtest.h"

#include <string>

namespace
{
    struct LateDep
    {
        int value = 42;
    };

    struct LateService
    {
        explicit LateService(skr::Arc<LateDep> dep) : dep(std::move(dep)) {}
        skr::Arc<LateDep> dep;
    };

    struct LatePlugin
    {
        std::string name = "plugin";
    };

    struct ILateContract
    {
        virtual ~ILateContract() = default;
        virtual int Id() const   = 0;
    };

    struct LateImpl : ILateContract
    {
        int Id() const override { return 7; }
    };

    struct ScopedToken
    {
        int n = 1;
    };
} // namespace

TEST(LateRegistrationSpec, AddSingletonAfterBuildResolves)
{
    auto sp = skr::ServiceCollection().CreateServiceProvider();

    EXPECT_FALSE(sp->Contains<LatePlugin>());
    sp->AddSingleton<LatePlugin>();
    EXPECT_TRUE(sp->Contains<LatePlugin>());

    auto a = sp->GetService<LatePlugin>();
    auto b = sp->GetService<LatePlugin>();
    ASSERT_TRUE(a);
    EXPECT_EQ(a.get(), b.get());
    EXPECT_EQ(a->name, "plugin");
}

TEST(LateRegistrationSpec, AddSingletonWithCtorDeps)
{
    auto sp = skr::ServiceCollection()
                  .AddSingleton<LateDep>()
                  .CreateServiceProvider();

    sp->AddSingleton<LateService>();
    auto service = sp->GetService<LateService>();
    ASSERT_TRUE(service);
    ASSERT_TRUE(service->dep);
    EXPECT_EQ(service->dep->value, 42);
}

TEST(LateRegistrationSpec, AddTransientProducesDistinctInstances)
{
    auto sp = skr::ServiceCollection().CreateServiceProvider();
    sp->AddTransient<LatePlugin>();

    auto a = sp->GetService<LatePlugin>();
    auto b = sp->GetService<LatePlugin>();
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    EXPECT_NE(a.get(), b.get());
}

TEST(LateRegistrationSpec, AddScopedVisibleInExistingScope)
{
    auto sp = skr::ServiceCollection().CreateServiceProvider();
    auto scope = sp->CreateServiceScope();

    sp->AddScoped<ScopedToken>();

    auto scopedSp = scope->GetServiceProvider();
    auto a        = scopedSp->GetService<ScopedToken>();
    auto b        = scopedSp->GetService<ScopedToken>();
    ASSERT_TRUE(a);
    EXPECT_EQ(a.get(), b.get());
}

TEST(LateRegistrationSpec, RemoveEvictsSingletonCache)
{
    auto sp = skr::ServiceCollection().CreateServiceProvider();
    sp->AddSingleton<LatePlugin>();

    auto before = sp->GetService<LatePlugin>();
    ASSERT_TRUE(before);

    EXPECT_TRUE(sp->Remove<LatePlugin>());
    EXPECT_FALSE(sp->Contains<LatePlugin>());
    EXPECT_FALSE(sp->TryGetService<LatePlugin>().has_value());

    sp->AddSingleton<LatePlugin>();
    auto after = sp->GetService<LatePlugin>();
    ASSERT_TRUE(after);
    EXPECT_NE(before.get(), after.get());
}

TEST(LateRegistrationSpec, RemoveClearsScopedCacheInLiveScope)
{
    auto sp = skr::ServiceCollection().CreateServiceProvider();
    sp->AddScoped<ScopedToken>();

    auto scope = sp->CreateServiceScope();
    auto a     = scope->GetServiceProvider()->GetService<ScopedToken>();
    ASSERT_TRUE(a);

    EXPECT_TRUE(sp->Remove<ScopedToken>());
    EXPECT_FALSE(sp->Contains<ScopedToken>());
    EXPECT_FALSE(
        scope->GetServiceProvider()->TryGetService<ScopedToken>().has_value());
}

TEST(LateRegistrationSpec, AddContractAndRemove)
{
    auto sp = skr::ServiceCollection().CreateServiceProvider();
    sp->AddSingleton<ILateContract, LateImpl>();

    auto service = sp->GetService<ILateContract>();
    ASSERT_TRUE(service);
    EXPECT_EQ(service->Id(), 7);

    EXPECT_TRUE(sp->Remove<ILateContract>());
    EXPECT_FALSE(sp->TryGetService<ILateContract>().has_value());
}

TEST(LateRegistrationSpec, RemoveReturnsFalseWhenMissing)
{
    auto sp = skr::ServiceCollection().CreateServiceProvider();
    EXPECT_FALSE(sp->Remove<LatePlugin>());
}
