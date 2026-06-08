#include <Skirnir/Common/Reflection.hpp>
#include <gtest/gtest.h>

class SingletonService
{
  public:
    SingletonService(int test) {}
};

enum Size {
    Small,
    Medium,
    Large,
    ExtraLarge
};

class ReflectionSpec : public ::testing::Test
{
};

TEST_F(ReflectionSpec, ReflectionShouldGetCorrectTypeName)
{
    auto name = refl::type_name<SingletonService>();

    ASSERT_STREQ(name.data(), "SingletonService");
}

TEST_F(ReflectionSpec, ReflectionShouldGetCorrectConstructorArgs)
{
    static_assert(
        std::is_same_v<refl::first_ctor_params_tuple<SingletonService>,
                       std::tuple<int>>);
    ASSERT_TRUE((std::is_same_v<refl::first_ctor_params_tuple<SingletonService>,
                                std::tuple<int>>) );
}

TEST_F(ReflectionSpec, ReflectionShouldGetEnumName)
{
    auto name = refl::enum_to_string(Large);

    ASSERT_STREQ(name.data(), "Large");
}