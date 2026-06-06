#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Configuration.hpp"
#include "Skirnir/Logging/LogScope.hpp"
#include "Skirnir/Logging/LogSinks.hpp"
#include "Skirnir/Logging/Logger.hpp"

#include <algorithm>
#include <cstring>
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

    void LoggerOptions::Dispatch(const LogRecord& record)
    {
        std::call_once(mDefaultSinkFlag, [this] {
            std::lock_guard<std::mutex> lock(mSinksMutex);
            if (mSinks.empty())
                mSinks.push_back(MakeArc<ConsoleSink>());
        });

        std::vector<Arc<ILogSink>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mSinksMutex);
            snapshot = mSinks;
        }
        for (auto& sink : snapshot)
        {
            sink->Write(record);
        }
    }

    LoggerOptions& LoggerOptions::AddSink(Arc<ILogSink> sink)
    {
        if (sink)
        {
            std::lock_guard<std::mutex> lock(mSinksMutex);
            mSinks.push_back(std::move(sink));
        }
        return *this;
    }

    void LoggerOptions::ClearSinks()
    {
        std::lock_guard<std::mutex> lock(mSinksMutex);
        mSinks.clear();
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
