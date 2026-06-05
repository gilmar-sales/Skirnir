#include "Skirnir/LogRecord.hpp"

#define SIMDJSON_STATIC_REFLECTION 1
#include <simdjson.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <filesystem>
#include <format>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <print>
#include <sstream>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace SKIRNIR_NAMESPACE
{
    namespace
    {
        const char* LevelName(LogLevel lvl)
        {
            switch (lvl)
            {
                case LogLevel::Debug:
                    return "Debug";
                case LogLevel::Trace:
                    return "Trace";
                case LogLevel::Information:
                    return "Information";
                case LogLevel::Warning:
                    return "Warning";
                case LogLevel::Error:
                    return "Error";
                case LogLevel::Fatal:
                    return "Fatal";
                case LogLevel::None:
                    return "None";
            }
            return "?";
        }
    } // namespace

    // ------------------------------------------------------------------
    // ConsoleSink
    // ------------------------------------------------------------------

    ConsoleSink::ConsoleSink(bool useColors) : mUseColors(useColors)
    {
    }

    void ConsoleSink::Write(const LogRecord& r)
    {
        const std::string prefix =
            std::format("[{}] {} '{}': ",
                        LevelName(r.level),
                        r.timestamp,
                        r.category);

        std::string scopesStr;
        if (!r.scopes.empty())
        {
            scopesStr.push_back('[');
            for (std::size_t i = 0; i < r.scopes.size(); ++i)
            {
                if (i)
                    scopesStr.push_back('/');
                scopesStr.append(r.scopes[i]);
            }
            scopesStr += "] ";
        }

        std::lock_guard<std::mutex> lock(mMutex);
        (void) mUseColors; // color toggle reserved for future fmt branch
        std::print("{}{}{}\n", prefix, scopesStr, r.message);
    }

    // ------------------------------------------------------------------
    // FileSink
    // ------------------------------------------------------------------

    FileSink::FileSink(std::filesystem::path path, bool autoFlush) :
        mPath(std::move(path)), mAutoFlush(autoFlush)
    {
#ifdef _WIN32
        std::wstring wpath = mPath.wstring();
        mFile              = _wfopen(wpath.c_str(), L"a");
#else
        mFile = std::fopen(mPath.string().c_str(), "a");
#endif
        if (!mFile)
        {
            throw std::runtime_error(
                "Skirnir: FileSink failed to open '" + mPath.string() + "'");
        }
    }

    FileSink::~FileSink()
    {
        if (mFile)
        {
            std::fclose(mFile);
            mFile = nullptr;
        }
    }

    void FileSink::Write(const LogRecord& r)
    {
        std::ostringstream oss;
        oss << '[' << LevelName(r.level) << "] " << r.timestamp << " '"
            << r.category << "': ";
        if (!r.scopes.empty())
        {
            oss << '[';
            for (std::size_t i = 0; i < r.scopes.size(); ++i)
            {
                if (i)
                    oss << '/';
                oss << r.scopes[i];
            }
            oss << "] ";
        }
        oss << r.message << '\n';
        std::string line = oss.str();

        std::lock_guard<std::mutex> lock(mMutex);
        if (mFile)
        {
            std::fwrite(line.data(), 1, line.size(), mFile);
            if (mAutoFlush)
                std::fflush(mFile);
        }
    }

    void FileSink::Flush()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mFile)
            std::fflush(mFile);
    }

    // ------------------------------------------------------------------
    // JsonSink
    // ------------------------------------------------------------------

    JsonSink::JsonSink(std::ostream& os) : mOs(os)
    {
    }

    JsonSink::JsonSink(std::filesystem::path path) :
        mOwned(std::make_unique<std::ofstream>(path)), mOs(*mOwned)
    {
        if (!mOwned->is_open())
            throw std::runtime_error(
                "Skirnir: JsonSink failed to open output file");
    }

    void JsonSink::Write(const LogRecord& r)
    {
        auto json = simdjson::to_json(r);

        if (json.has_value())
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mOs << json.value() << '\n';
        }
    }

    void JsonSink::Flush()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mOs.flush();
    }

    // ------------------------------------------------------------------
    // AsyncSink
    // ------------------------------------------------------------------

    AsyncSink::AsyncSink(Ref<ILogSink> inner, std::size_t queueCapacity) :
        mInner(std::move(inner)), mCapacity(queueCapacity ? queueCapacity : 1)
    {
        if (!mInner)
        {
            throw std::runtime_error(
                "Skirnir: AsyncSink requires a non-null inner sink");
        }
        mWorker = std::jthread([this](std::stop_token st) { WorkerLoop(st); });
    }

    AsyncSink::~AsyncSink()
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mStopping = true;
        }
        mCv.notify_all();
        if (mWorker.joinable())
            mWorker.join();
        if (mInner)
            mInner->Flush();
    }

    void AsyncSink::Write(const LogRecord& r)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mStopping)
        {
            mDropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        if (mQueue.size() >= mCapacity)
        {
            mDropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        mQueue.push_back(r);
        lock.unlock();
        mCv.notify_one();
    }

    void AsyncSink::Flush()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCv.wait(lock, [this] { return mQueue.empty(); });
        if (mInner)
            mInner->Flush();
    }

    std::uint64_t AsyncSink::DroppedCount() const noexcept
    {
        return mDropped.load(std::memory_order_relaxed);
    }

    void AsyncSink::WorkerLoop(std::stop_token st)
    {
        while (true)
        {
            LogRecord record;
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mCv.wait(lock,
                         [this, &st] { return mStopping || !mQueue.empty(); });
                if (mStopping && mQueue.empty())
                    return;
                if (mQueue.empty())
                {
                    if (st.stop_requested())
                        return;
                    continue;
                }
                record = std::move(mQueue.front());
                mQueue.pop_front();
            }
            // Notify Flush() waiters that the queue shrank.
            mCv.notify_all();
            try
            {
                mInner->Write(record);
            }
            catch (...)
            {
                // Sinks must not throw; swallow defensively.
            }
        }
    }
} // namespace SKIRNIR_NAMESPACE
