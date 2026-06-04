#pragma once

#include "Common.hpp"
#include "Reflection.hpp"

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#ifdef SKIRNIR_USE_FMT
    #include <fmt/chrono.h>
    #include <fmt/color.h>
    #include <fmt/core.h>
#else
    #include <format>
    #include <print>
#endif

namespace SKIRNIR_NAMESPACE
{
    class ConfigurationOptions;

    enum class LogLevel
    {
        Debug,
        Trace,
        Information,
        Warning,
        Error,
        Fatal,
        None
    };

    class LoggerOptions
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
            auto it = mLogLevels.find(std::string(refl::type_name<T>()));
            if (it != mLogLevels.end())
            {
                return it->second;
            }

            it = mLogLevels.find(std::string(refl::type_namespace<T>()));
            if (it != mLogLevels.end())
            {
                return it->second;
            }

            // Check for partial matches (e.g., if namespaceValue is
            // "MyApp.Services" check for "MyApp")
            std::string_view nsStr = refl::type_namespace<T>();
            for (const auto& [key, level] : mLogLevels)
            {
                if (nsStr.size() > key.size() &&
                    nsStr.compare(0, key.size(), key) == 0 &&
                    nsStr[key.size()] == '.')
                {
                    return level;
                }
            }

            return logLevel;
        }

      private:
        std::map<std::string, LogLevel> mLogLevels;
    };

    class ILogger
    {
    };

    template <typename T>
    class Logger : public ILogger
    {
      public:
        Logger(Ref<LoggerOptions> loggerOptions) : mLoggerOptions(loggerOptions)
        {
            mLogLevel = mLoggerOptions->GetLogLevelFor<T>();
        }

#ifdef SKIRNIR_USE_FMT
        template <typename... TArgs>
        inline void Assert(bool assertion, fmt::format_string<TArgs...> fmt,
                           TArgs&&... args)
        {
    #ifndef NDEBUG
            if (!assertion)
            {
                LogFatal(fmt, std::forward<TArgs>(args)...);
            }
    #endif
        }

        template <typename... TArgs>
        inline void LogDebug(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Debug)
                return;

            fmt::print(fg(fmt::color::forest_green), "[Debug] {} '{}': ",
                       std::chrono::system_clock::now(), refl::type_name<T>());

            fmt::print(fg(fmt::color::forest_green), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogTrace(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Trace)
                return;

            fmt::print(fg(fmt::color::gainsboro), "[Trace] {} '{}': ",
                       std::chrono::system_clock::now(), refl::type_name<T>());

            fmt::print(fg(fmt::color::gainsboro), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogInformation(fmt::format_string<TArgs...> fmt,
                                   TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Information)
                return;

            fmt::print(fg(fmt::color::sky_blue), "[Information] {} '{}': ",
                       std::chrono::system_clock::now(), refl::type_name<T>());

            fmt::print(fg(fmt::color::sky_blue), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogWarning(fmt::format_string<TArgs...> fmt,
                               TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Warning)
                return;

            fmt::print(fg(fmt::color::gold), "[Warning] {} '{}': ",
                       std::chrono::system_clock::now(), refl::type_name<T>());

            fmt::print(fg(fmt::color::gold), fmt, std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogError(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Error)
                return;

            fmt::print(fg(fmt::color::crimson), "[Error] {} '{}': ",
                       std::chrono::system_clock::now(), refl::type_name<T>());

            fmt::print(fg(fmt::color::crimson), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogFatal(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Error)
                return;

            const auto line = fmt::format(fmt, std::forward<TArgs>(args)...);

            fmt::print(fg(fmt::color::crimson), "[Fatal] {} '{}': {}\n",
                       std::chrono::system_clock::now(), refl::type_name<T>(),
                       line);

            throw std::runtime_error(line);
        }
#else

        template <typename... TArgs>
        inline void LogTrace(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Trace)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Trace] {} '{}': {}",
                         std::chrono::system_clock::now(), refl::type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogDebug(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Debug)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Debug] {} '{}': {}",
                         std::chrono::system_clock::now(), refl::type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogInformation(std::format_string<TArgs...> fmt,
                                   TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Information)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Information] {} '{}': {}",
                         std::chrono::system_clock::now(), refl::type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogWarning(std::format_string<TArgs...> fmt,
                               TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Warning)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Warning] {} '{}': {}",
                         std::chrono::system_clock::now(), refl::type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogError(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Error)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Error] {} '{}': {}",
                         std::chrono::system_clock::now(), refl::type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogFatal(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLogLevel > LogLevel::Fatal)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Fatal] {} '{}': {}",
                         std::chrono::system_clock::now(), refl::type_name<T>(),
                         line);

            throw std::runtime_error(line);
        }

        template <typename... TArgs>
        inline void Assert(bool assertion, std::format_string<TArgs...> fmt,
                           TArgs&&... args)
        {
    #ifndef NDEBUG
            if (!assertion)
            {
                LogFatal(fmt, std::forward<TArgs>(args)...);
            }
    #endif
        }
#endif

        LogLevel           mLogLevel;
        Ref<LoggerOptions> mLoggerOptions;
    };

} // namespace SKIRNIR_NAMESPACE
