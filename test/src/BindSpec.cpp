#include <gtest/gtest.h>

#include <Skirnir/Configuration.hpp>

namespace bind_test
{
    struct DatabaseOptions
    {
        std::string host;
        int         port    = 0;
        bool        ssl     = false;
        double      timeout = 0.0;
    };

    struct NestedOptions
    {
        DatabaseOptions primary;
        std::string     region;
    };

    struct WithArray
    {
        std::vector<std::string> tags;
    };
} // namespace bind_test

TEST(BindSpec, BindsPrimitives)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "host": "localhost",
                          "port": 5432,
                          "ssl": true,
                          "timeout": 1.25
                      })")
                      .Build();

    auto opts = config->Bind<bind_test::DatabaseOptions>();
    EXPECT_EQ(opts->host, "localhost");
    EXPECT_EQ(opts->port, 5432);
    EXPECT_TRUE(opts->ssl);
    EXPECT_DOUBLE_EQ(opts->timeout, 1.25);
}

TEST(BindSpec, MissingFieldsKeepDefaults)
{
    auto config =
        skr::ConfigurationBuilder().AddJsonString(R"({"host": "x"})").Build();

    auto opts = config->Bind<bind_test::DatabaseOptions>();
    EXPECT_EQ(opts->host, "x");
    EXPECT_EQ(opts->port, 0);
    EXPECT_FALSE(opts->ssl);
    EXPECT_DOUBLE_EQ(opts->timeout, 0.0);
}

TEST(BindSpec, BindsNestedStruct)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "primary": {
                              "host": "db->local",
                              "port": 3306,
                              "ssl": true,
                              "timeout": 2.5
                          },
                          "region": "us-east"
                      })")
                      .Build();

    auto opts = config->Bind<bind_test::NestedOptions>();
    EXPECT_EQ(opts->region, "us-east");
    EXPECT_EQ(opts->primary.host, "db->local");
    EXPECT_EQ(opts->primary.port, 3306);
    EXPECT_TRUE(opts->primary.ssl);
    EXPECT_DOUBLE_EQ(opts->primary.timeout, 2.5);
}

TEST(BindSpec, BindFromSection)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "database": {
                              "host": "remote",
                              "port": 1234,
                              "ssl": true,
                              "timeout": 9.5
                          }
                      })")
                      .Build();

    auto db = config->Bind<bind_test::DatabaseOptions>("database");
    EXPECT_EQ(db->host, "remote");
    EXPECT_EQ(db->port, 1234);
    EXPECT_TRUE(db->ssl);
    EXPECT_DOUBLE_EQ(db->timeout, 9.5);
}

TEST(BindSpec, BindsArray)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({"tags": ["alpha", "beta", "gamma"]})")
                      .Build();

    auto opts = config->Bind<bind_test::WithArray>();
    ASSERT_EQ(opts->tags.size(), 3u);
    EXPECT_EQ(opts->tags[0], "alpha");
    EXPECT_EQ(opts->tags[2], "gamma");
}
