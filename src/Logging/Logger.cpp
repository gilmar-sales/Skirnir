#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Configuration.hpp"
#include "Skirnir/Logging/LogScope.hpp"
#include "Skirnir/Logging/LogSinks.hpp"
#include "Skirnir/Logging/Logger.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace SKIRNIR_NAMESPACE
{
    namespace
    {
        std::vector<std::string>& ScopeStack()
        {
            thread_local std::vector<std::string> stack;
            return stack;
        }
    } // namespace

    static LogLevel ParseLogLevel(std::string_view level)
    {
        if (level == "Debug" || level == "debug" || level == "DEBUG")
            return LogLevel::Debug;
        if (level == "Trace" || level == "trace" || level == "TRACE")
            return LogLevel::Trace;
        if (level == "Information" || level == "Info" ||
            level == "information" || level == "INFO")
            return LogLevel::Information;
        if (level == "Warning" || level == "warn" || level == "WARNING")
            return LogLevel::Warning;
        if (level == "Error" || level == "error" || level == "ERROR")
            return LogLevel::Error;
        if (level == "Fatal" || level == "fatal" || level == "FATAL")
            return LogLevel::Fatal;
        if (level == "None" || level == "none" || level == "NONE")
            return LogLevel::None;

        return LogLevel::Information;
    }

    void LoggerOptions::ConfigureFrom(Arc<ConfigurationOptions> config,
                                      std::string_view          path)
    {
        if (!config)
            return;

        // Default log level
        if (auto level = config->GetString(path); !level.empty())
        {
            logLevel = ParseLogLevel(level);
        }

        // Async-sink options live under "logging.async.*" — the default
        // path is "logging.logLevel.default" so the parent is "logging".
        // We tolerate arbitrary paths by deriving the parent of @p path.
        {
            std::string_view parent = path;
            const auto       dot    = parent.rfind('.');
            if (dot != std::string_view::npos)
                parent = parent.substr(0, dot);

            const std::string asyncEnabledKey =
                std::string(parent) + ".async.enabled";
            const std::string asyncCapacityKey =
                std::string(parent) + ".async.queueCapacity";

            if (config->HasKey(asyncEnabledKey))
            {
                asyncEnabled = config->GetBool(asyncEnabledKey, asyncEnabled);
            }
            if (config->HasKey(asyncCapacityKey))
            {
                const auto v = config->GetInt(asyncCapacityKey,
                                              static_cast<int64_t>(
                                                  asyncQueueCapacity));
                if (v > 0)
                {
                    asyncQueueCapacity = static_cast<std::size_t>(v);
                }
            }
        }

        // Namespace-specific levels live in the same object that holds the
        // default key, so locate the parent section and iterate its members.
        auto dot = path.rfind('.');
        if (dot == std::string_view::npos)
            return;

        std::string_view sectionPath = path.substr(0, dot);
        std::string_view defaultKey  = path.substr(dot + 1);

        // Collect the entries first while holding only a shared lock on the
        // map (ForEachMember reads from the configuration, not the map), and
        // then publish them under an exclusive lock in one shot. This keeps
        // the writer out of any hot read path on GetLogLevelFor<T>.
        std::vector<std::pair<std::string, LogLevel>> additions;
        config->ForEachMember(
            sectionPath,
            [&](std::string_view key, simdjson::dom::element value) {
                if (key == defaultKey)
                    return;
                if (!value.is_string())
                    return;
                std::string_view sv;
                if (value.get_string().get(sv) != simdjson::SUCCESS)
                    return;
                additions.emplace_back(std::string(key), ParseLogLevel(sv));
            });

        if (!additions.empty())
        {
            std::unique_lock<std::shared_mutex> lock(mLogLevelsMutex);
            for (auto& [key, level] : additions)
            {
                mLogLevels[std::move(key)] = level;
            }
        }
    }

    void LoggerOptions::PublishSinks()
    {
        // Called with @c mSinksMutex held. Builds an immutable copy of
        // @c mSinks and atomically publishes it for the hot read path.
        auto snap = std::make_shared<const std::vector<Arc<ILogSink>>>(mSinks);
        mSinkSnapshot = std::move(snap);
        mSinkSnapshotDirty.store(false, std::memory_order_release);
    }

    void LoggerOptions::Dispatch(const LogRecord& record)
    {
        // Lazy default sink install. Only one thread wins the race; the
        // others see a non-empty mSinks on the snapshot read below.
        if (mSinkSnapshotDirty.load(std::memory_order_acquire))
        {
            std::call_once(mDefaultSinkFlag, [this] {
                std::lock_guard<std::mutex> lock(mSinksMutex);
                if (mSinks.empty())
                {
                    if (asyncEnabled)
                    {
                        mSinks.push_back(MakeArc<AsyncSink>(
                            MakeArc<ConsoleSink>(),
                            asyncQueueCapacity ? asyncQueueCapacity : 1));
                    }
                    else
                    {
                        mSinks.push_back(MakeArc<ConsoleSink>());
                    }
                }
                PublishSinks();
            });
            // Lost the call_once race OR had a non-empty sink list at
            // the moment of the check. Fall through to load the snapshot.
        }

        // Hot path: single atomic load, no lock, no vector copy.
        auto snap = mSinkSnapshot;
        if (!snap)
        {
            std::lock_guard<std::mutex> lock(mSinksMutex);
            snap = mSinkSnapshot;
            if (!snap)
            {
                PublishSinks();
                snap = mSinkSnapshot;
            }
        }

        for (const auto& sink : *snap)
        {
            sink->Write(record);
        }
    }

    LoggerOptions& LoggerOptions::AddSink(Arc<ILogSink> sink)
    {
        if (sink)
        {
            std::lock_guard<std::mutex> lock(mSinksMutex);
            // Avoid double-wrapping if the user added an AsyncSink
            // explicitly — @c Dispatch's lazy default does the same.
            if (dynamic_cast<AsyncSink*>(sink.get()))
            {
                if (asyncEnabled)
                {
                    // Caller already provided async; respect their choice
                    // and keep the default install in sync with that.
                    mSinks.push_back(std::move(sink));
                    PublishSinks();
                    return *this;
                }
            }
            mSinks.push_back(std::move(sink));
            PublishSinks();
        }
        return *this;
    }

    void LoggerOptions::ClearSinks()
    {
        std::lock_guard<std::mutex> lock(mSinksMutex);
        mSinks.clear();
        PublishSinks();
    }

    void LoggerOptions::PushScope(std::string name)
    {
        ScopeStack().push_back(std::move(name));
    }

    void LoggerOptions::PopScope()
    {
        auto& stack = ScopeStack();
        if (!stack.empty())
            stack.pop_back();
    }

    std::vector<std::string> LoggerOptions::CurrentScopes() const
    {
        const auto& stack = ScopeStack();
        return {stack.begin(), stack.end()};
    }

    Arc<LogScope> LoggerOptions::BeginScope(std::string name)
    {
        return MakeArc<LogScope>(shared_from_this(), std::move(name));
    }
} // namespace SKIRNIR_NAMESPACE
