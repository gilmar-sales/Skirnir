// Logging throughput microbenchmark.
//
// Measures LoggerOptions::Dispatch() under three sink configurations:
//   A. ConsoleSink        (synchronous, mutex-protected — reproduces
//                          the regression observed in the wrk benchmark
//                          at 657k → 16k req/s when log middleware is
//                          enabled).
//   B. AsyncSink(ConsoleSink)  (the new default).
//   C. NullSink           (sanity upper bound, no I/O).
//
// Each case runs N producer threads, each issuing kPerThread records for
// kSeconds wall time. We report records/sec/core and the AsyncSink drop
// count. The benchmark is intentionally minimal — no Google Benchmark
// dependency — so the CMake build stays self-contained.

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Logging/LogSinks.hpp"
#include "Skirnir/Logging/Logger.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

namespace
{
    struct BenchCategory
    {
    };

    struct BenchResult
    {
        std::uint64_t records;
        double        seconds;
        std::uint64_t dropped;
    };

    template <typename SinkFactory>
    BenchResult Run(SinkFactory&& makeSink, int threads, int perThread)
    {
        auto options = SKIRNIR_NAMESPACE::MakeArc<SKIRNIR_NAMESPACE::LoggerOptions>();
        options->ClearSinks();
        options->AddSink(makeSink());

        SKIRNIR_NAMESPACE::Logger<BenchCategory> logger(options);

        std::atomic<int> ready {0};
        std::atomic<bool> go {false};
        std::atomic<std::uint64_t> total {0};

        std::vector<std::thread> ts;
        ts.reserve(threads);
        for (int t = 0; t < threads; ++t)
        {
            ts.emplace_back([&]() {
                ready.fetch_add(1, std::memory_order_release);
                while (!go.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
                for (int i = 0; i < perThread; ++i)
                {
                    logger.LogInformation("bench-msg-{}", i);
                }
                total.fetch_add(perThread, std::memory_order_relaxed);
            });
        }

        while (ready.load(std::memory_order_acquire) < threads)
        {
            std::this_thread::yield();
        }

        const auto t0 = std::chrono::steady_clock::now();
        go.store(true, std::memory_order_release);

        for (auto& th : ts)
            th.join();

        // For the AsyncSink case, Flush() drains the queue so we measure
        // the steady-state producer rate fairly.
        options->Sinks().front()->Flush();

        const auto t1 = std::chrono::steady_clock::now();
        const double seconds =
            std::chrono::duration<double>(t1 - t0).count();

        // Probe the drop counter if the sink exposes one.
        std::uint64_t dropped = 0;
        if (auto* async = dynamic_cast<SKIRNIR_NAMESPACE::AsyncSink*>(
                options->Sinks().front().get()))
        {
            dropped = async->DroppedCount();
        }

        return {total.load(), seconds, dropped};
    }
} // namespace

int main()
{
    constexpr int kThreads    = 14;
    constexpr int kPerThread  = 200'000;

    std::printf("LoggingBench: %d threads x %d records each\n",
                kThreads, kPerThread);
    std::printf("-----------------------------------------------\n");

    {
        auto r = Run(
            [] {
                return SKIRNIR_NAMESPACE::MakeArc<SKIRNIR_NAMESPACE::ConsoleSink>();
            },
            kThreads, kPerThread);
        const double rate = static_cast<double>(r.records) / r.seconds;
        std::printf("[A] ConsoleSink (sync)         : %10.0f rec/s   "
                    "(%.2fs, dropped=%llu)\n",
                    rate, r.seconds,
                    static_cast<unsigned long long>(r.dropped));
    }
    {
        auto r = Run(
            [] {
                return SKIRNIR_NAMESPACE::MakeArc<SKIRNIR_NAMESPACE::AsyncSink>(
                    SKIRNIR_NAMESPACE::MakeArc<SKIRNIR_NAMESPACE::ConsoleSink>(),
                    8192);
            },
            kThreads, kPerThread);
        const double rate = static_cast<double>(r.records) / r.seconds;
        std::printf("[B] AsyncSink(ConsoleSink)     : %10.0f rec/s   "
                    "(%.2fs, dropped=%llu)\n",
                    rate, r.seconds,
                    static_cast<unsigned long long>(r.dropped));
    }
    {
        auto r = Run(
            [] { return SKIRNIR_NAMESPACE::MakeArc<SKIRNIR_NAMESPACE::NullSink>(); },
            kThreads, kPerThread);
        const double rate = static_cast<double>(r.records) / r.seconds;
        std::printf("[C] NullSink (no I/O)          : %10.0f rec/s   "
                    "(%.2fs, dropped=%llu)\n",
                    rate, r.seconds,
                    static_cast<unsigned long long>(r.dropped));
    }
    return 0;
}
