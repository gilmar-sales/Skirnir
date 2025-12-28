#include "gtest/gtest.h"

import Skirnir;

class ReflectionSpec : public ::testing::Test
{
  protected:
    void SetUp() override {}

    void TearDown() override {}
};

class ExampleType;

TEST_F(ReflectionSpec, ReflectionShouldGetTypeName)
{
    // Arrange

    // Act
    auto name = skr::type_name<ExampleType>();

    // Assert
    ASSERT_STREQ(name, "ExampleType");
}

TEST_F(ReflectionSpec, ReflectionShouldGetConstructorArgs)
{
    // Arrange
    class ConstructorArgsExample
    {
      public:
        ConstructorArgsExample(int, long) {}
    };

    // Act
    auto args = skr::get_constructor_args<ConstructorArgsExample>();

    auto args_name  = skr::type_name<decltype(args)>();
    auto tuple_name = skr::type_name<std::tuple<int, long>>();
    // Assert
    ASSERT_STREQ(args_name, tuple_name);
}

TEST_F(ReflectionSpec, ReflectionShouldGetLambdaArgs)
{
    // Arrange
    auto my_lamda = [](int, float, long) {};

    // Act
    auto args = skr::callable_args<decltype(my_lamda)>();

    auto args_name  = skr::type_name<decltype(args)>();
    auto tuple_name = skr::type_name<std::tuple<int, float, long>>();
    // Assert
    ASSERT_STREQ(args_name, tuple_name);
}

void MyFunction(int, float, long, unsigned)
{
}

TEST_F(ReflectionSpec, ReflectionShouldGetFunctionArgs)
{
    // Arrange

    // Act
    auto args = skr::callable_args<decltype(MyFunction)>();

    auto args_name  = skr::type_name<decltype(args)>();
    auto tuple_name = skr::type_name<std::tuple<int, float, long, unsigned>>();
    // Assert
    ASSERT_STREQ(args_name, tuple_name);
}