#pragma once

#include "LogLevel.hpp"
#include "LogRecord.hpp"
#include "LogSinks/ILogSink.hpp"
#include "Reflection.hpp"

#include <chrono>
#include <format>
#include <map>
#include <mutex>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace SKIRNIR_NAMESPACE
{
    class ConfigurationOptions;
    class LogScope;

    /**
     * @brief Configuration for the logging subsystem.
     *
     * Holds the default level, namespace-specific level overrides, and the
     * list of sinks that receive every @c LogRecord dispatched by any
     * @c Logger<T> in the application.
     */
    class LoggerOptions : public std::enable_shared_from_this<LoggerOptions>
    {
      public:
#ifdef NDEBUG
        LogLevel logLevel = LogLevel::Trace;
#else
        LogLevel logLevel = LogLevel::Debug;
#endif

        /**
         * @brief Configures the default log level from a configuration source.
         * @param config The configuration options
         * @param path The key path to the log level (default: "logging.logLevel.default")
         */
        void ConfigureFrom(Ref<ConfigurationOptions> config,
                           std::string_view          path = "logging.logLevel.default");

        template <typename T>
        LogLevel GetLogLevelFor()
        {
            constexpr auto typeName = refl::type_name<T>();
            constexpr auto typeNs   = refl::type_namespace<T>();

            auto it = mLogLevels.find(std::string(typeName));
            if (it != mLogLevels.end())
            {
                return it->second;
            }

            it = mLogLevels.find(std::string(typeNs));
            if (it != mLogLevels.end())
            {
                return it->second;
            }

            // Check for partial matches (e.g., if namespaceValue is
            // "MyApp.Services" check for "MyApp")
            for (const auto& [key, level] : mLogLevels)
            {
                if (typeNs.size() > key.size() &&
                    typeNs.compare(0, key.size(), key) == 0 &&
                    typeNs[key.size()] == '.')
                {
                    return level;
                }
            }

            return logLevel;
        }

        // ----- Sink management ----------------------------------------

        /**
         * @brief Appends a sink. Returns *this for chaining.
         */
        LoggerOptions& AddSink(Ref<ILogSink> sink);

        const std::vector<Ref<ILogSink>>& Sinks() const noexcept
        {
            return mSinks;
        }

        /**
         * @brief Removes every configured sink. Useful when an
         *        extension wants to wrap the existing sinks in an
         *        @c AsyncSink.
         */
        void ClearSinks();

        /**
         * @brief Dispatches a record to every configured sink.
         *
         * Lazily appends a @c ConsoleSink the first time it is called with
         * an empty sink list, so existing applications that do not
         * configure logging keep producing output on the console.
         */
        void Dispatch(const LogRecord& record);

        // ----- Scope management ---------------------------------------

        /**
         * @brief Begins a log scope on the current thread. Returns a
         *        RAII handle that pops the scope on destruction.
         */
        Ref<LogScope> BeginScope(std::string name);

        /// @cond INTERNAL
        void                       PushScope(std::string name);
        void                       PopScope();
        std::vector<std::string>   CurrentScopes() const;
        /// @endcond

      private:
        std::map<std::string, LogLevel>  mLogLevels;
        mutable std::mutex               mSinksMutex;
        std::vector<Ref<ILogSink>>       mSinks;
        std::once_flag                   mDefaultSinkFlag;
    };

    class ILogger
    {
    };

    template <typename T>
    class Logger : public ILogger
    {
      public:
        Logger(Ref<LoggerOptions> loggerOptions) :
            mLoggerOptions(std::move(loggerOptions))
        {
            mLogLevel = mLoggerOptions->GetLogLevelFor<T>();
        }

        template <typename... TArgs>
        inline void LogTrace(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            DispatchImpl(LogLevel::Trace, std::source_location::current(),
                         fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        inline void LogDebug(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            DispatchImpl(LogLevel::Debug, std::source_location::current(),
                         fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        inline void LogInformation(std::format_string<TArgs...> fmt,
                                   TArgs&&... args)
        {
            DispatchImpl(LogLevel::Information, std::source_location::current(),
                          fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        inline void LogWarning(std::format_string<TArgs...> fmt,
                               TArgs&&... args)
        {
            DispatchImpl(LogLevel::Warning, std::source_location::current(),
                          fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        inline void LogError(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            DispatchImpl(LogLevel::Error, std::source_location::current(),
                         fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        inline void LogFatal(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            DispatchImpl(LogLevel::Fatal, std::source_location::current(),
                         fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        inline void Assert(bool                       assertion,
                           std::format_string<TArgs...> fmt,
                           TArgs&&... args)
        {
#ifndef NDEBUG
            if (!assertion)
            {
                LogFatal(fmt, std::forward<TArgs>(args)...);
            }
#endif
        }

      private:
        template <typename... TArgs>
        inline void DispatchImpl(LogLevel                     lvl,
                                 std::source_location         loc,
                                 std::format_string<TArgs...> fmt,
                                 TArgs&&... args)
        {
            if (mLogLevel > lvl)
                return;

            std::string message = std::format(fmt, std::forward<TArgs>(args)...);

            LogRecord record;
            record.level     = lvl;
            record.timestamp = std::chrono::system_clock::now();
            record.category.assign(refl::type_name<T>());
            record.message   = std::move(message);
            record.scopes    = mLoggerOptions->CurrentScopes();

            mLoggerOptions->Dispatch(record);

            if (lvl == LogLevel::Fatal)
            {
                throw std::runtime_error(record.message);
            }
        }

        LogLevel           mLogLevel;
        Ref<LoggerOptions> mLoggerOptions;
    };

} // namespace SKIRNIR_NAMESPACE
