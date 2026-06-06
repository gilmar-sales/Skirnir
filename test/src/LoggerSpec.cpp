#include <gtest/gtest.h>

#include <Skirnir/Configuration.hpp>
#include <Skirnir/Logging/Logger.hpp>

class Unknown
{
};

namespace log_test
{
    class Repository
    {
    };

    class OtherRepository
    {
    };
} // namespace log_test

TEST(LoggerSpec, LoggerOptions_ConfigureFrom)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "logging": {
                              "logLevel": {
                                  "default": "Information",
                                  "log_test::Repository": "Warning",
                                  "log_test::OtherRepository": "Trace"
                              }
                          }
                      })")
                      .Build();

    auto options = skr::MakeRef<skr::LoggerOptions>();
    options->ConfigureFrom(config);

    // Default log level should be Information
    EXPECT_EQ(options->GetLogLevelFor<Unknown>(), skr::LogLevel::Information);

    // Repository should have Warning level
    EXPECT_EQ(options->GetLogLevelFor<log_test::Repository>(),
              skr::LogLevel::Warning);

    // OtherRepository should have Trace level
    EXPECT_EQ(options->GetLogLevelFor<log_test::OtherRepository>(),
              skr::LogLevel::Trace);
}

namespace my_app
{
    namespace services
    {
        class DataBase
        {
        };
    } // namespace services
} // namespace my_app

TEST(LoggerSpec, LoggerOptions_NamespaceHierarchy)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "logging": {
                              "logLevel": {
                                  "default": "Error",
                                  "my_app::services": "Debug"
                              }
                          }
                      })")
                      .Build();

    auto options = skr::MakeRef<skr::LoggerOptions>();
    options->ConfigureFrom(config);

    // Unknown namespace should use default (Error)
    EXPECT_EQ(options->GetLogLevelFor<Unknown>(), skr::LogLevel::Error);

    // MyApp.Services.Database should match MyApp.Services (partial match)
    EXPECT_EQ(options->GetLogLevelFor<my_app::services::DataBase>(),
              skr::LogLevel::Debug);
}