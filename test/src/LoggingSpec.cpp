#include <gtest/gtest.h>

#include <Skirnir/Configuration.hpp>
#include <Skirnir/LogRecord.hpp>
#include <Skirnir/LogScope.hpp>
#include <Skirnir/LogSinks.hpp>
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

#ifndef _WIN32
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

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
    r.timestamp = std::chrono::system_clock::now();
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

// -----------------------------------------------------------------------
// 11. Scopes_SurviveAsyncSink (regression for F-01 use-after-free)
// -----------------------------------------------------------------------
TEST(LoggingSpec, Scopes_SurviveAsyncSink)
{
    auto inner = skr::MakeRef<TestSink>();
    auto async = skr::MakeRef<skr::AsyncSink>(inner, 64);

    auto options = skr::MakeRef<skr::LoggerOptions>();
    options->ClearSinks();
    options->AddSink(async);

    skr::Logger<LogCategory> logger(options);

    {
        auto scope = options->BeginScope(std::string("scoped-name"));
        logger.LogInformation("hi");
        // scope is destroyed here; the string it pushed to the
        // thread-local stack is freed. AsyncSink must still see the
        // scope name because scopes are owned by the LogRecord itself.
    }

    async->Flush();

    auto recs = inner->Snapshot();
    ASSERT_EQ(recs.size(), 1u);
    ASSERT_EQ(recs[0].scopes.size(), 1u);
    EXPECT_EQ(recs[0].scopes[0], "scoped-name");
}

// -----------------------------------------------------------------------
// 12. ConsoleSink_EscapesControlChars (regression for F-04 log injection)
// -----------------------------------------------------------------------
// Note: we do not capture stdout in this test because std::print writes to
// the C stdout (not the std::cout streambuf), and a portable capture would
// require platform-specific plumbing. The sanitizer is shared with
// FileSink (see test #13 below) so coverage is still effective.

// -----------------------------------------------------------------------
// 13. FileSink_EscapesControlChars (regression for F-04 log injection)
// -----------------------------------------------------------------------
TEST(LoggingSpec, FileSink_EscapesControlChars)
{
    static std::atomic<int> counter {0};
    auto path =
        std::filesystem::current_path() /
        ("skirnir_log_injection_test_" +
         std::to_string(counter.fetch_add(1)) + "_" +
         std::to_string(static_cast<long long>(
             std::chrono::system_clock::now().time_since_epoch().count())) +
         ".log");

    {
        auto options = skr::MakeRef<skr::LoggerOptions>();
        options->ClearSinks();
        options->AddSink(skr::MakeRef<skr::FileSink>(path));

        skr::Logger<LogCategory> logger(options);
        logger.LogInformation(
            "user said: {}\n[Information] forged 'Admin': approved",
            "hello");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.is_open()) << "Failed to open " << path;
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());

    // No raw newline immediately followed by a "[Information]" line —
    // i.e. the attacker could not forge a record by inserting a newline.
    EXPECT_EQ(content.find("\n[Information] forged"), std::string::npos);
    // The newline should have been escaped to "\n".
    EXPECT_NE(content.find("\\n"), std::string::npos);
}

// -----------------------------------------------------------------------
// 14. FileSink_RejectsSymlinkTarget (POSIX)
// -----------------------------------------------------------------------
#ifndef _WIN32
TEST(LoggingSpec, FileSink_RejectsSymlinkTarget)
{
    static std::atomic<int> counter {0};
    auto base =
        std::filesystem::current_path() /
        ("skirnir_symlink_test_" +
         std::to_string(counter.fetch_add(1)) + "_" +
         std::to_string(static_cast<long long>(
             std::chrono::system_clock::now().time_since_epoch().count())));
    auto target = base.string() + ".real";
    auto link   = base.string() + ".log";

    // Create a real file, then a symlink pointing at it.
    {
        std::ofstream t(target);
        t << "real\n";
    }
    ASSERT_EQ(::symlink(target.c_str(), link.c_str()), 0)
        << "failed to create symlink: " << std::strerror(errno);

    bool threw = false;
    try
    {
        skr::FileSink sink(link);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    EXPECT_TRUE(threw) << "FileSink must refuse to follow a symlink";

    // The symlink target must not have been appended to.
    std::ifstream in(target, std::ios::binary);
    std::string   content((std::istreambuf_iterator<char>(in)),
                          std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "real\n");

    std::error_code ec;
    std::filesystem::remove(target, ec);
    std::filesystem::remove(link, ec);
}
#endif

// -----------------------------------------------------------------------
// 15. FileSink_RejectsPathToDirectory
// -----------------------------------------------------------------------
TEST(LoggingSpec, FileSink_RejectsPathToDirectory)
{
    auto dir = std::filesystem::current_path() /
               ("skirnir_dir_test_" +
                std::to_string(static_cast<long long>(
                    std::chrono::system_clock::now().time_since_epoch().count())));
    std::filesystem::create_directory(dir);

    bool threw = false;
    try
    {
        skr::FileSink sink(dir);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    EXPECT_TRUE(threw) << "FileSink must refuse a directory path";

    std::error_code ec;
    std::filesystem::remove(dir, ec);
}

// -----------------------------------------------------------------------
// 16. FileSink_RotatesOnSize
// -----------------------------------------------------------------------
TEST(LoggingSpec, FileSink_RotatesOnSize)
{
    static std::atomic<int> counter {0};
    auto base =
        std::filesystem::current_path() /
        ("skirnir_rotation_test_" +
         std::to_string(counter.fetch_add(1)) + "_" +
         std::to_string(static_cast<long long>(
             std::chrono::system_clock::now().time_since_epoch().count())));
    auto path = base.string() + ".log";

    {
        skr::FileSinkOptions opts;
        opts.maxBytes = 64;
        opts.maxFiles = 3;
        auto sink     = skr::MakeRef<skr::FileSink>(path, opts, /*autoFlush=*/true);

        // Each record is well under 64 bytes by itself, but the sink must
        // rotate once the cumulative size would exceed 64 bytes.
        for (int i = 0; i < 50; ++i)
        {
            skr::LogRecord r;
            r.level     = skr::LogLevel::Information;
            r.timestamp = std::chrono::system_clock::now();
            r.category  = "rotate";
            r.message   = "line-" + std::to_string(i);
            sink->Write(r);
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_TRUE(std::filesystem::exists(base.string() + ".log.1"));

    std::error_code ec;
    std::filesystem::remove(path, ec);
    for (int i = 1; i <= 3; ++i)
    {
        std::filesystem::remove(base.string() + ".log." + std::to_string(i), ec);
    }
}

// -----------------------------------------------------------------------
// 17. FileSink_AppendsAcrossInstancesAfterRotation
// -----------------------------------------------------------------------
TEST(LoggingSpec, FileSink_AppendsAcrossInstancesAfterRotation)
{
    static std::atomic<int> counter {0};
    auto base =
        std::filesystem::current_path() /
        ("skirnir_append_rot_" +
         std::to_string(counter.fetch_add(1)) + "_" +
         std::to_string(static_cast<long long>(
             std::chrono::system_clock::now().time_since_epoch().count())));
    auto path = base.string() + ".log";

    {
        skr::FileSinkOptions opts;
        opts.maxBytes = 32;
        auto sink     = skr::FileSink(path, opts);
        skr::LogRecord r;
        r.level     = skr::LogLevel::Information;
        r.timestamp = std::chrono::system_clock::now();
        r.category  = "C";
        r.message   = "first-longish-message";
        sink.Write(r);
    }
    {
        auto sink = skr::FileSink(path);
        skr::LogRecord r;
        r.level     = skr::LogLevel::Warning;
        r.timestamp = std::chrono::system_clock::now();
        r.category  = "C";
        r.message   = "second";
        sink.Write(r);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::ifstream in(path, std::ios::binary);
    std::string   content((std::istreambuf_iterator<char>(in)),
                          std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("second"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(path, ec);
    for (int i = 1; i <= 5; ++i)
    {
        std::filesystem::remove(base.string() + ".log." + std::to_string(i), ec);
    }
}

// -----------------------------------------------------------------------
// 18. JsonSink_AppendsAcrossInstances
// -----------------------------------------------------------------------
TEST(LoggingSpec, JsonSink_AppendsAcrossInstances)
{
    static std::atomic<int> counter {0};
    auto base =
        std::filesystem::current_path() /
        ("skirnir_json_append_" +
         std::to_string(counter.fetch_add(1)) + "_" +
         std::to_string(static_cast<long long>(
             std::chrono::system_clock::now().time_since_epoch().count())));
    auto path = base.string() + ".ndjson";

    {
        auto sink = skr::JsonSink(path);
        skr::LogRecord r;
        r.level     = skr::LogLevel::Information;
        r.timestamp = std::chrono::system_clock::now();
        r.category  = "A";
        r.message   = "alpha";
        sink.Write(r);
    }
    {
        auto sink = skr::JsonSink(path);
        skr::LogRecord r;
        r.level     = skr::LogLevel::Warning;
        r.timestamp = std::chrono::system_clock::now();
        r.category  = "B";
        r.message   = "beta";
        sink.Write(r);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::ifstream in(path, std::ios::binary);
    std::string   content((std::istreambuf_iterator<char>(in)),
                          std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("\"message\":\"alpha\""), std::string::npos);
    EXPECT_NE(content.find("\"message\":\"beta\""), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

// -----------------------------------------------------------------------
// 19. LoggerOptions_ConfigureFrom_IsThreadSafe
// -----------------------------------------------------------------------
namespace thread_safety_test
{
    struct AlphaCategory
    {
    };
    struct BetaCategory
    {
    };
} // namespace thread_safety_test

TEST(LoggingSpec, LoggerOptions_ConfigureFrom_IsThreadSafe)
{
    auto config = skr::ConfigurationBuilder()
                      .AddJsonString(R"({
                          "logging": {
                              "logLevel": {
                                  "default": "Information",
                                  "thread_safety_test::AlphaCategory": "Debug",
                                  "thread_safety_test::BetaCategory":  "Warning"
                              }
                          }
                      })")
                      .Build();

    auto options = skr::MakeRef<skr::LoggerOptions>();

    constexpr int kProducers = 4;
    constexpr int kIters     = 200;

    std::vector<std::thread> writers;
    for (int t = 0; t < kProducers; ++t)
    {
        writers.emplace_back([options, config]() {
            for (int i = 0; i < kIters; ++i)
            {
                options->ConfigureFrom(config);
            }
        });
    }

    std::atomic<bool> stop {false};
    std::vector<std::thread> readers;
    for (int t = 0; t < kProducers; ++t)
    {
        readers.emplace_back([options, &stop]() {
            while (!stop.load())
            {
                (void) options->GetLogLevelFor<thread_safety_test::AlphaCategory>();
                (void) options->GetLogLevelFor<thread_safety_test::BetaCategory>();
                (void) options->GetLogLevelFor<LogCategory>();
            }
        });
    }

    for (auto& th : writers)
        th.join();
    stop.store(true);
    for (auto& th : readers)
        th.join();

    // After concurrent ConfigureFrom, the final state must still be
    // consistent: the namespace-specific entries reflect the last
    // committed write, never a half-applied state.
    EXPECT_EQ(options->GetLogLevelFor<thread_safety_test::AlphaCategory>(),
              skr::LogLevel::Debug);
    EXPECT_EQ(options->GetLogLevelFor<thread_safety_test::BetaCategory>(),
              skr::LogLevel::Warning);
}

