
#include <gtest/gtest.h>

#include <Skirnir/Configuration.hpp>
#include <Skirnir/Logger.hpp>

TEST(ConfigurationSpec, LoadFromString_SimpleValue)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({"key": "value"})")
                      .Build();

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
    EXPECT_EQ(config->GetValue("count"), std::optional<std::string>("42"));
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
                      .AddJsonString(R"({"a": 1, "b": 2})")
                      .AddJsonString(R"({"b": 20, "c": 30})")
                      .Build();

    EXPECT_EQ(config->GetInt("a"), 1);
    EXPECT_EQ(config->GetInt("b"), 20);
    EXPECT_EQ(config->GetInt("c"), 30);
}

TEST(ConfigurationSpec, ConfigurationBuilder_ChainingMergesObjects)
{
    auto config =
        skr::ConfigurationBuilder()
            .AddJsonString(R"({"logging": {"level": "Information"}})")
            .AddJsonString(R"({"logging": {"namespaces": {"MyApp": "Debug"}}})")
            .Build();

    EXPECT_EQ(config->GetString("logging.level"), "Information");
    EXPECT_EQ(config->GetString("logging.namespaces.MyApp"), "Debug");
}

TEST(ConfigurationSpec, TypedGetters)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "flag": true,
                          "count": 7,
                          "ratio": 1.5,
                          "name": "skirnir",
                          "tags": ["a", "b", "c"]
                      })")
                      .Build();

    EXPECT_TRUE(config->GetBool("flag"));
    EXPECT_FALSE(config->GetBool("missing", false));
    EXPECT_EQ(config->GetInt("count"), 7);
    EXPECT_EQ(config->GetInt("missing", 99), 99);
    EXPECT_DOUBLE_EQ(config->GetDouble("ratio"), 1.5);
    EXPECT_EQ(config->GetString("name"), "skirnir");
    auto tags = config->GetArray("tags");
    ASSERT_EQ(tags.size(), 3u);
    EXPECT_EQ(tags[0], "a");
    EXPECT_EQ(tags[2], "c");
}

TEST(ConfigurationSpec, GetSection)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "db": {
                              "host": "localhost",
                              "port": 5432
                          }
                      })")
                      .Build();

    auto db = config->GetSection("db");
    ASSERT_TRUE(db);
    EXPECT_EQ(db->GetString("host"), "localhost");
    EXPECT_EQ(db->GetInt("port"), 5432);
    EXPECT_EQ(db->GetValue("host"), std::optional<std::string>("localhost"));
}

TEST(ConfigurationSpec, GetSection_OutlivesParent)
{
    auto config  = skr::ConfigurationBuilder()
                       .AddJsonString(R"({"a": {"b": 42}})")
                       .Build();
    auto section = config->GetSection("a");
    config.reset(); // destroy the parent configuration
    EXPECT_EQ(section->GetInt("b"), 42);
}

TEST(ConfigurationSpec, MalformedJson_Throws)
{
    EXPECT_THROW(
        skr::ConfigurationBuilder().AddJsonString(R"({not json)").Build(),
        std::runtime_error);
}
