
#include <gtest/gtest.h>

#include <Skirnir/Configuration.hpp>
#include <Skirnir/Logger.hpp>

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <cstdlib>
#include <string>
#include <utility>

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

namespace
{
    class ScopedEnv
    {
      public:
        ScopedEnv(std::string name, std::string value) : mName(std::move(name))
        {
#if defined(_WIN32)
            _putenv_s(mName.c_str(), value.c_str());
#else
            ::setenv(mName.c_str(), value.c_str(), 1);
#endif
        }

        ~ScopedEnv()
        {
#if defined(_WIN32)
            _putenv_s(mName.c_str(), "");
#else
            ::unsetenv(mName.c_str());
#endif
        }

        ScopedEnv(const ScopedEnv&)            = delete;
        ScopedEnv& operator=(const ScopedEnv&) = delete;

      private:
        std::string mName;
    };
} // namespace

TEST(ConfigurationSpec, EnvironmentVariables_ReadsKeyWithoutPrefix)
{
    ScopedEnv env("SKR_TEST_FOO", "bar");
    auto      config = skr::ConfigurationBuilder()
                        .AddEnvironmentVariables()
                        .Build();
    EXPECT_EQ(config->GetString("SKR_TEST_FOO"), "bar");
}

TEST(ConfigurationSpec, EnvironmentVariables_HonorsPrefix_StripsIt)
{
    ScopedEnv env("SKR_TEST_PREFIX_HOST", "localhost");
    auto      config = skr::ConfigurationBuilder()
                        .AddEnvironmentVariables("SKR_TEST_PREFIX_")
                        .Build();
    EXPECT_EQ(config->GetString("HOST"), "localhost");
}

TEST(ConfigurationSpec, EnvironmentVariables_DoubleUnderscoreBecomesNestedSection)
{
    ScopedEnv env("SKR_TEST_PREFIX_DB__PORT", "5432");
    auto      config = skr::ConfigurationBuilder()
                        .AddEnvironmentVariables("SKR_TEST_PREFIX_")
                        .Build();
    EXPECT_EQ(config->GetInt("DB.PORT"), 5432);
    auto db = config->GetSection("DB");
    ASSERT_TRUE(db);
    EXPECT_EQ(db->GetInt("PORT"), 5432);
}

TEST(ConfigurationSpec, EnvironmentVariables_CoercesBooleansAndNumbers)
{
    ScopedEnv b("SKR_TEST_BOOL", "true");
    ScopedEnv i("SKR_TEST_INT", "42");
    ScopedEnv d("SKR_TEST_DBL", "1.5");
    auto      config = skr::ConfigurationBuilder()
                        .AddEnvironmentVariables()
                        .Build();
    EXPECT_TRUE(config->GetBool("SKR_TEST_BOOL"));
    EXPECT_EQ(config->GetInt("SKR_TEST_INT"), 42);
    EXPECT_DOUBLE_EQ(config->GetDouble("SKR_TEST_DBL"), 1.5);
}

TEST(ConfigurationSpec, EnvironmentVariables_NonParseableValueStaysString)
{
    ScopedEnv s("SKR_TEST_STR", "hello world");
    auto      config = skr::ConfigurationBuilder()
                        .AddEnvironmentVariables()
                        .Build();
    EXPECT_EQ(config->GetString("SKR_TEST_STR"), "hello world");
}

TEST(ConfigurationSpec, EnvironmentVariables_IgnoresVariablesOutsidePrefix)
{
    ScopedEnv other("SKR_TEST_OTHER", "x");
    ScopedEnv inside("SKR_TEST_PREFIX_KEPT", "y");
    auto      config = skr::ConfigurationBuilder()
                        .AddEnvironmentVariables("SKR_TEST_PREFIX_")
                        .Build();
    EXPECT_FALSE(config->HasKey("SKR_TEST_OTHER"));
    EXPECT_FALSE(config->HasKey("OTHER"));
    EXPECT_EQ(config->GetString("KEPT"), "y");
}

TEST(ConfigurationSpec, EnvironmentVariables_MergesWithOtherSources)
{
    ScopedEnv env("SKR_TEST_PREFIX_b", "2");
    auto      config =
        skr::ConfigurationBuilder()
            .AddJsonString(R"({"a": 1})")
            .AddEnvironmentVariables("SKR_TEST_PREFIX_")
            .Build();
    EXPECT_EQ(config->GetInt("a"), 1);
    EXPECT_EQ(config->GetInt("b"), 2);
}
