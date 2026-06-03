#include <Skirnir/Reflection.hpp>
#include <gtest/gtest.h>

class SingletonService
{
  public:
    SingletonService(int test) {}
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
    static_assert(std::is_same_v<refl::as_tuple<SingletonService>, std::tuple<int>>);
    ASSERT_TRUE((std::is_same_v<refl::as_tuple<SingletonService>, std::tuple<int>>) );
}