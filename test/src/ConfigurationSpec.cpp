
#include <gtest/gtest.h>

#include <Skirnir/Logger.hpp>
#include <Skirnir/Configuration.hpp>

TEST(ConfigurationSpec, LoadFromString_SimpleValue)
{
    auto config =
        skr::ConfigurationBuilder().AddJsonString(R"({"key": "value"})").Build();

    EXPECT_TRUE(config->HasKey("key"));
    EXPECT_EQ(config->GetValue("key"), std::optional<std::string>("value"));
}

TEST(ConfigurationSpec, LoadFromString_NestedValue)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({"parent": {"child": "nestedValue"}})")
                      .Build();

    EXPECT_TRUE(config->HasKey("parent.child"));
    EXPECT_EQ(config->GetValue("parent.child"),
              std::optional<std::string>("nestedValue"));
}

TEST(ConfigurationSpec, LoadFromString_NumberValue)
{
    auto config =
       skr::ConfigurationBuilder().AddJsonString(R"({"count": 42})").Build();

    EXPECT_TRUE(config->HasKey("count"));
    EXPECT_EQ(config->GetValue("count")->data(),
              std::optional<std::string>("42"));
}

TEST(ConfigurationSpec, LoadFromString_BooleanValues)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({"enabled": true, "disabled": false})")
                      .Build();

    EXPECT_EQ(config->GetValue("enabled"), std::optional<std::string>("true"));
    EXPECT_EQ(config->GetValue("disabled"),
              std::optional<std::string>("false"));
}

TEST(ConfigurationSpec, LoadFromString_LoggingLevel)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "logging": {
                              "level": "Warning",
                              "namespaces": {
                                  "MyApp": "Debug"
                              }
                          }
                      })")
                      .Build();

    EXPECT_TRUE(config->HasKey("logging.level"));
    EXPECT_EQ(config->GetValue("logging.level"),
              std::optional<std::string>("Warning"));
    EXPECT_TRUE(config->HasKey("logging.namespaces"));
}


TEST(ConfigurationSpec, ConfigurationBuilder_Chaining)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({"key1": "value1"})")
                      .Build();

    EXPECT_TRUE(config->HasKey("key1"));
    EXPECT_EQ(config->GetValue("key1"), std::optional<std::string>("value1"));
}