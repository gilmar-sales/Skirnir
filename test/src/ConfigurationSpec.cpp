#include "Skirnir/Logger.hpp"

#include <gtest/gtest.h>

#include <Skirnir/Configuration.hpp>

using namespace skr;

TEST(ConfigurationTest, LoadFromString_SimpleValue)
{
    auto config =
        ConfigurationBuilder().AddJsonString(R"({"key": "value"})").Build();

    EXPECT_TRUE(config->HasKey("key"));
    EXPECT_EQ(config->GetValue("key"), std::optional<std::string>("value"));
}

TEST(ConfigurationTest, LoadFromString_NestedValue)
{
    auto config = ConfigurationBuilder()
                      .AddJsonString(R"({"parent": {"child": "nestedValue"}})")
                      .Build();

    EXPECT_TRUE(config->HasKey("parent.child"));
    EXPECT_EQ(config->GetValue("parent.child"),
              std::optional<std::string>("nestedValue"));
}

TEST(ConfigurationTest, LoadFromString_NumberValue)
{
    auto config =
        ConfigurationBuilder().AddJsonString(R"({"count": 42})").Build();

    EXPECT_TRUE(config->HasKey("count"));
    EXPECT_EQ(config->GetValue("count")->data(), std::optional<std::string>("42"));
}

TEST(ConfigurationTest, LoadFromString_BooleanValues)
{
    auto config = ConfigurationBuilder()
                      .AddJsonString(R"({"enabled": true, "disabled": false})")
                      .Build();

    EXPECT_EQ(config->GetValue("enabled"), std::optional<std::string>("true"));
    EXPECT_EQ(config->GetValue("disabled"),
              std::optional<std::string>("false"));
}

TEST(ConfigurationTest, LoadFromString_LoggingLevel)
{
    auto config = ConfigurationBuilder()
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

TEST(ConfigurationTest, LoggerOptions_ConfigureFrom)
{
    auto config = ConfigurationBuilder()
                      .AddJsonString(R"({
                          "logging": {
                              "level": "Information",
                              "namespaces": {
                                  "skr.Repository": "Warning",
                                  "skr.OtherRepository": "Trace"
                              }
                          }
                      })")
                      .Build();

    auto options = MakeRef<LoggerOptions>();
    options->ConfigureFrom(config);

    // Default log level should be Information
    EXPECT_EQ(options->GetLogLevelForNamespace("skr.Unknown"),
              LogLevel::Information);

    // Repository should have Warning level
    EXPECT_EQ(options->GetLogLevelForNamespace("skr.Repository"),
              LogLevel::Warning);

    // OtherRepository should have Trace level
    EXPECT_EQ(options->GetLogLevelForNamespace("skr.OtherRepository"),
              LogLevel::Trace);
}

TEST(ConfigurationTest, LoggerOptions_NamespaceHierarchy)
{
    auto config = ConfigurationBuilder()
                      .AddJsonString(R"({
                          "logging": {
                              "level": "Error",
                              "namespaces": {
                                  "MyApp.Services": "Debug"
                              }
                          }
                      })")
                      .Build();

    auto options = MakeRef<skr::LoggerOptions>();
    options->ConfigureFrom(config);

    // Unknown namespace should use default (Error)
    EXPECT_EQ(options->GetLogLevelForNamespace("Unknown"), LogLevel::Error);

    // MyApp.Services should have Debug level
    EXPECT_EQ(options->GetLogLevelForNamespace("MyApp.Services"),
              LogLevel::Debug);

    // MyApp.Services.Database should match MyApp.Services (partial match)
    EXPECT_EQ(options->GetLogLevelForNamespace("MyApp.Services.Database"),
              LogLevel::Debug);
}

TEST(ConfigurationTest, ConfigurationBuilder_Chaining)
{
    auto config =
        ConfigurationBuilder().AddJsonString(R"({"key1": "value1"})").Build();

    EXPECT_TRUE(config->HasKey("key1"));
    EXPECT_EQ(config->GetValue("key1"), std::optional<std::string>("value1"));
}