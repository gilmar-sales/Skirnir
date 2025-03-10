#pragma once

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>

#include "Common.hpp"

namespace SKIRNIR_NAMESPACE
{
    enum class LogLevel
    {
        Debug,
        Trace,
        Information,
        Warning,
        Error,
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
    };

    class Logger
    {
      public:
        Logger(Ref<LoggerOptions> loggerOptions) : mLoggerOptions(loggerOptions)
        {
        }

        template <typename... T>
        inline void Assert(bool assertion, fmt::format_string<T...> fmt,
                           T&&... args)
        {
#ifndef NDEBUG
            if (!assertion)
            {
                LogError(fmt, args...);
                abort();
            }
#endif
        }

        template <typename... T>
        inline void LogDebug(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Debug)
                return;

            fmt::print(fg(fmt::color::forest_green),
                       "[Debug] {}: ", std::chrono::system_clock::now());

            fmt::print(fg(fmt::color::forest_green), fmt, args...);

            fmt::print("\n");
        }

        template <typename... T>
        inline void LogTrace(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Trace)
                return;

            fmt::print(fg(fmt::color::gainsboro),
                       "[Trace] {}: ", std::chrono::system_clock::now());

            fmt::print(fg(fmt::color::gainsboro), fmt, args...);

            fmt::print("\n");
        }

        template <typename... T>
        inline void LogInformation(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Information)
                return;

            fmt::print(fg(fmt::color::sky_blue),
                       "[Information] {}: ", std::chrono::system_clock::now());

            fmt::print(fg(fmt::color::sky_blue), fmt, args...);

            fmt::print("\n");
        }

        template <typename... T>
        inline void LogWarning(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Warning)
                return;

            fmt::print(fg(fmt::color::gold),
                       "[Warning] {}: ", std::chrono::system_clock::now());

            fmt::print(fg(fmt::color::gold), fmt, args...);

            fmt::print("\n");
        }

        template <typename... T>
        inline void LogError(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Error)
                return;

            fmt::print(fg(fmt::color::crimson),
                       "[Error] {}: ", std::chrono::system_clock::now());

            fmt::print(fg(fmt::color::crimson), fmt, args...);

            fmt::print("\n");
        }

      private:
        template <typename... T>
        inline void Log(fmt::v11::text_style     style,
                        fmt::format_string<T...> fmt, T&&... args)
        {
            fmt::print(style, fmt, args...);
        }

        Ref<LoggerOptions> mLoggerOptions;
    };

} // namespace SKIRNIR_NAMESPACE