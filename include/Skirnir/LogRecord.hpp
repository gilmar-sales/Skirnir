#pragma once

#include "Common.hpp"
#include "LogLevel.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <ostream>
#include <source_location>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief A complete log entry, ready to be handed to a sink.
     */
    struct LogRecord
    {
        LogLevel                              level = LogLevel::Information;
        std::chrono::system_clock::time_point timestamp {};
        std::string_view                      category {};
        std::string                           message {};
        std::vector<std::string_view>         scopes {};
        std::source_location                  location {};
    };

    /**
     * @brief A destination for log records.
     *
     * Implementations must be thread-safe; @c Logger<T> dispatches from
     * arbitrary threads.
     */
    class ILogSink
    {
      public:
        virtual ~ILogSink() = default;
        virtual void Write(const LogRecord& record) = 0;
        virtual void Flush()
        {
        }
    };

    /**
     * @brief Discards every record. Useful in tests and for silencing
     *        a particular category.
     */
    class NullSink final : public ILogSink
    {
      public:
        void Write(const LogRecord&) override
        {
        }
    };

    /**
     * @brief Writes records to standard output, optionally with ANSI color.
     */
    class ConsoleSink final : public ILogSink
    {
      public:
        explicit ConsoleSink(bool useColors = true);

        void Write(const LogRecord& record) override;

      private:
        bool              mUseColors;
        std::mutex        mMutex;
    };

    /**
     * @brief Appends records as plain text to a file.
     */
    class FileSink final : public ILogSink
    {
      public:
        explicit FileSink(std::filesystem::path path, bool autoFlush = true);
        ~FileSink() override;

        void Write(const LogRecord& record) override;
        void Flush() override;

        const std::filesystem::path& Path() const noexcept
        {
            return mPath;
        }

      private:
        std::filesystem::path mPath;
        std::FILE*            mFile = nullptr;
        std::mutex            mMutex;
        bool                  mAutoFlush;
    };

    /**
     * @brief Emits records as NDJSON (one JSON object per line) to either
     *        an existing @c std::ostream or a file path.
     */
    class JsonSink final : public ILogSink
    {
      public:
        explicit JsonSink(std::ostream& os);
        explicit JsonSink(std::filesystem::path path);

        void Write(const LogRecord& record) override;
        void Flush() override;

      private:
        std::unique_ptr<std::ofstream> mOwned;
        std::ostream&                  mOs;
        std::mutex                     mMutex;
    };

    /**
     * @brief Decorator that forwards records to an inner sink from a
     *        background thread, with a bounded queue.
     *
     * @c Write() never blocks the caller. When the queue is full, the
     *        new record is dropped and a counter is incremented
     *        (exposable via @c DroppedCount).
     */
    class AsyncSink final : public ILogSink
    {
      public:
        explicit AsyncSink(Ref<ILogSink> inner,
                           std::size_t   queueCapacity = 8192);
        ~AsyncSink() override;

        void Write(const LogRecord& record) override;
        void Flush() override;

        std::uint64_t DroppedCount() const noexcept;

      private:
        void WorkerLoop(std::stop_token st);

        Ref<ILogSink>            mInner;
        std::mutex               mMutex;
        std::condition_variable  mCv;
        std::deque<LogRecord>    mQueue;
        std::size_t              mCapacity;
        std::jthread             mWorker;
        bool                     mStopping = false;
        std::atomic<std::uint64_t> mDropped {0};
    };
} // namespace SKIRNIR_NAMESPACE
