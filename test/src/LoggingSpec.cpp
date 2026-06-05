#include <gtest/gtest.h>

#include <Skirnir/Configuration.hpp>
#include <Skirnir/LogRecord.hpp>
#include <Skirnir/LogScope.hpp>
#include <Skirnir/Logger.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

namespace
{
    class TestSink final : public skr::ILogSink
    {
      public:
        void Write(const skr::LogRecord& r) override
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mRecords.push_back(r);
        }
        void Flush() override
        {
        }

        std::vector<skr::LogRecord> Snapshot()
        {
            std::lock_guard<std::mutex> lock(mMutex);
            return mRecords;
        }

      private:
        std::mutex                  mMutex;
        std::vector<skr::LogRecord> mRecords;
    };

    // A non-fundamental tag type used as the Logger<T> template parameter
    // in tests, so that refl::type_namespace<T> resolves to a real
    // namespace rather than failing for built-in types like @c int.
    struct LogCategory
    {
    };
} // namespace

// -----------------------------------------------------------------------
// 1. Sinks_RouteByLevel
// -----------------------------------------------------------------------
TEST(LoggingSpec, Sinks_RouteByLevel)
{
    auto options = skr::MakeRef<skr::LoggerOptions>();
    auto sink    = skr::MakeRef<TestSink>();
    options->AddSink(sink);
    options->logLevel = skr::LogLevel::Warning;

    skr::Logger<LogCategory> logger(options);

    logger.LogDebug("d");
    logger.LogInformation("i");
    logger.LogWarning("w");
    logger.LogError("e");

    auto recs = sink->Snapshot();
    ASSERT_EQ(recs.size(), 2u);
    EXPECT_EQ(recs[0].level, skr::LogLevel::Warning);
    EXPECT_EQ(recs[0].message, "w");
    EXPECT_EQ(recs[1].level, skr::LogLevel::Error);
    EXPECT_EQ(recs[1].message, "e");
}

// -----------------------------------------------------------------------
// 3. Scopes_Nest
// -----------------------------------------------------------------------
TEST(LoggingSpec, Scopes_Nest)
{
    auto options = skr::MakeRef<skr::LoggerOptions>();
    auto sink    = skr::MakeRef<TestSink>();
    options->AddSink(sink);

    skr::Logger<LogCategory> logger(options);

    {
        auto outer = options->BeginScope("outer");
        {
            auto inner = options->BeginScope("inner");
            logger.LogInformation("hi");
        }
        logger.LogInformation("only-outer");
    }
    logger.LogInformation("no-scope");

    auto recs = sink->Snapshot();
    ASSERT_EQ(recs.size(), 3u);

    EXPECT_EQ(recs[0].scopes.size(), 2u);
    EXPECT_EQ(recs[0].scopes[0], "outer");
    EXPECT_EQ(recs[0].scopes[1], "inner");

    EXPECT_EQ(recs[1].scopes.size(), 1u);
    EXPECT_EQ(recs[1].scopes[0], "outer");

    EXPECT_TRUE(recs[2].scopes.empty());
}

// -----------------------------------------------------------------------
// 4. Scopes_ThreadLocal
// -----------------------------------------------------------------------
TEST(LoggingSpec, Scopes_ThreadLocal)
{
    auto optionsA = skr::MakeRef<skr::LoggerOptions>();
    auto sinkA    = skr::MakeRef<TestSink>();
    optionsA->AddSink(sinkA);

    auto optionsB = skr::MakeRef<skr::LoggerOptions>();
    auto sinkB    = skr::MakeRef<TestSink>();
    optionsB->AddSink(sinkB);

    std::thread tA([&]() {
        auto s = optionsA->BeginScope("A-only");
        skr::Logger<LogCategory> logger(optionsA);
        logger.LogInformation("a-msg");
    });

    std::thread tB([&]() {
        skr::Logger<LogCategory> logger(optionsB);
        logger.LogInformation("b-msg");
    });

    tA.join();
    tB.join();

    auto recsA = sinkA->Snapshot();
    auto recsB = sinkB->Snapshot();

    ASSERT_EQ(recsA.size(), 1u);
    ASSERT_EQ(recsB.size(), 1u);

    // A's record has "A-only" scope.
    ASSERT_EQ(recsA[0].scopes.size(), 1u);
    EXPECT_EQ(recsA[0].scopes[0], "A-only");

    // B's record has NO scopes — thread-local isolation.
    EXPECT_TRUE(recsB[0].scopes.empty());
}

// -----------------------------------------------------------------------
// 5. AsyncSink_DrainsOnFlush
// -----------------------------------------------------------------------
TEST(LoggingSpec, AsyncSink_DrainsOnFlush)
{
    auto options = skr::MakeRef<skr::LoggerOptions>();
    options->ClearSinks();
    auto inner = skr::MakeRef<TestSink>();
    auto async = skr::MakeRef<skr::AsyncSink>(inner, 64);
    options->AddSink(async);

    skr::Logger<LogCategory> logger(options);

    constexpr int N = 50;
    for (int i = 0; i < N; ++i)
        logger.LogInformation("msg-{}", i);

    async->Flush();
    EXPECT_EQ(inner->Snapshot().size(), static_cast<std::size_t>(N));
}

// -----------------------------------------------------------------------
// 7. AsyncSink_DrainsOnDestruction
// -----------------------------------------------------------------------
TEST(LoggingSpec, AsyncSink_DrainsOnDestruction)
{
    auto inner = skr::MakeRef<TestSink>();
    {
        auto options = skr::MakeRef<skr::LoggerOptions>();
        options->ClearSinks();
        auto async = skr::MakeRef<skr::AsyncSink>(inner, 64);
        options->AddSink(async);

        skr::Logger<LogCategory> logger(options);
        for (int i = 0; i < 10; ++i)
            logger.LogInformation("d-msg-{}", i);
        // Drop the shared_ptr; AsyncSink dtor must drain.
    }
    EXPECT_EQ(inner->Snapshot().size(), 10u);
}

// -----------------------------------------------------------------------
// 8. JsonSink_ProducesValidNdjson
// -----------------------------------------------------------------------
TEST(LoggingSpec, JsonSink_ProducesValidNdjson)
{
    std::ostringstream oss;
    auto sink = skr::MakeRef<skr::JsonSink>(oss);

    skr::LogRecord r;
    r.level    = skr::LogLevel::Information;
    r.category = "Cat";
    r.message  = "hello";
    sink->Write(r);
    sink->Flush();

    auto str = oss.str();
    EXPECT_FALSE(str.empty());
    EXPECT_EQ(str.back(), '\n');
    EXPECT_NE(str.find("\"level\":\"Information\""), std::string::npos);
    EXPECT_NE(str.find("\"category\":\"Cat\""), std::string::npos);
    EXPECT_NE(str.find("\"message\":\"hello\""), std::string::npos);
}

// -----------------------------------------------------------------------
// 9. FileSink_AppendsAcrossInstances
// -----------------------------------------------------------------------
TEST(LoggingSpec, FileSink_AppendsAcrossInstances)
{
    static std::atomic<int> counter {0};
    auto path =
        std::filesystem::current_path() /
        ("skirnir_filesink_test_" +
         std::to_string(counter.fetch_add(1)) + "_" +
         std::to_string(static_cast<long long>(
             std::chrono::system_clock::now().time_since_epoch().count())) +
         ".log");

    {
        auto sink = skr::MakeRef<skr::FileSink>(path);
        skr::LogRecord r;
        r.level    = skr::LogLevel::Information;
        r.timestamp = std::chrono::high_resolution_clock::now();
        r.category = "A";
        r.message  = "first";
        sink->Write(r);
    }
    {
        auto sink = skr::MakeRef<skr::FileSink>(path);
        skr::LogRecord r;
        r.level    = skr::LogLevel::Warning;
        r.timestamp = std::chrono::high_resolution_clock::now();
        r.category = "B";
        r.message  = "second";
        sink->Write(r);
    }

    // Read the file with a short sleep to let Windows release the file
    // locks from the now-destroyed FileSinks.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.is_open()) << "Failed to open " << path;
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("first"), std::string::npos);
    EXPECT_NE(content.find("second"), std::string::npos);
}

// -----------------------------------------------------------------------
// 10. LogFatal_StillThrows
// -----------------------------------------------------------------------
TEST(LoggingSpec, LogFatal_StillThrows)
{
    auto options = skr::MakeRef<skr::LoggerOptions>();
    auto sink    = skr::MakeRef<TestSink>();
    options->AddSink(sink);

    skr::Logger<LogCategory> logger(options);
    EXPECT_THROW(logger.LogFatal("boom"), std::runtime_error);

    auto recs = sink->Snapshot();
    ASSERT_EQ(recs.size(), 1u);
    EXPECT_EQ(recs[0].level, skr::LogLevel::Fatal);
    EXPECT_EQ(recs[0].message, "boom");
}

