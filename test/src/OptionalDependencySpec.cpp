#include <Skirnir/Skirnir.hpp>

#include "gtest/gtest.h"

#include <optional>
#include <string>

namespace optional_test
{
    class ILogger
    {
      public:
        virtual ~ILogger()                          = default;
        virtual std::string Tag() const             = 0;
    };

    class ConsoleLogger : public ILogger
    {
      public:
        std::string Tag() const override { return "console"; }
    };

    class Consumer
    {
      public:
        explicit Consumer(std::optional<skr::Arc<ILogger>> logger) :
            mLogger(std::move(logger))
        {
        }

        bool HasLogger() const { return mLogger.has_value(); }
        std::string Tag() const
        {
            return mLogger.has_value() ? (*mLogger)->Tag() : "none";
        }

      private:
        std::optional<skr::Arc<ILogger>> mLogger;
    };
} // namespace optional_test

TEST(OptionalDependencySpec, ResolvesNulloptWhenServiceIsUnregistered)
{
    using namespace optional_test;
    auto sp = skr::ServiceCollection()
                  .AddSingleton<Consumer>()
                  .CreateServiceProvider();

    auto consumer = sp->GetService<Consumer>();
    ASSERT_TRUE(consumer);
    EXPECT_FALSE(consumer->HasLogger());
    EXPECT_EQ(consumer->Tag(), "none");
}

TEST(OptionalDependencySpec, ResolvesValueWhenServiceIsRegistered)
{
    using namespace optional_test;
    auto sp = skr::ServiceCollection()
                  .AddSingleton<ILogger, ConsoleLogger>()
                  .AddSingleton<Consumer>()
                  .CreateServiceProvider();

    auto consumer = sp->GetService<Consumer>();
    ASSERT_TRUE(consumer);
    EXPECT_TRUE(consumer->HasLogger());
    EXPECT_EQ(consumer->Tag(), "console");
}
