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

    template <typename T>
    constexpr std::string type_name()
    {
#if defined(__clang__) || defined(__GNUC__)
        // __PRETTY_FUNCTION__ on Clang/GCC looks like:
        //   std::string type_name() [with T = MyNamespace::MyType]
        std::string pretty = __PRETTY_FUNCTION__;
        auto        start  = pretty.find("T = ") + 4;
        auto        end    = pretty.rfind(']');
        return pretty.substr(start, end - start);
#elif defined(_MSC_VER)
        // __FUNCSIG__ on MSVC looks like:
        //   std::string __cdecl type_name<TYPE>(void)
        std::string pretty = __FUNCSIG__;
        auto        start  = pretty.find("type_name<") + 16;
        auto        end    = pretty.rfind(">(void)");
        return pretty.substr(start, end - start);
#else
        return "Unknown Compiler";
#endif
    }

    class ILogger
    {
    };

    template <typename T>
    class Logger : public ILogger
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
                LogError(fmt, std::forward<T>(args)...);
                abort();
            }
#endif
        }

        template <typename... T>
        inline void LogDebug(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Debug)
                return;

            fmt::print(fg(fmt::color::forest_green), "[Debug] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::forest_green), fmt,
                       std::forward<T>(args)...);

            fmt::print("\n");
        }

        template <typename... T>
        inline void LogTrace(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Trace)
                return;

            fmt::print(fg(fmt::color::gainsboro), "[Trace] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::gainsboro), fmt,
                       std::forward<T>(args)...);

            fmt::print("\n");
        }

        template <typename... T>
        inline void LogInformation(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Information)
                return;

            fmt::print(fg(fmt::color::sky_blue), "[Information] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::sky_blue), fmt, std::forward<T>(args)...);

            fmt::print("\n");
        }

        template <typename... T>
        inline void LogWarning(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Warning)
                return;

            fmt::print(fg(fmt::color::gold), "[Warning] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::gold), fmt, std::forward<T>(args)...);

            fmt::print("\n");
        }

        template <typename... T>
        inline void LogError(fmt::format_string<T...> fmt, T&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Error)
                return;

            fmt::print(fg(fmt::color::crimson), "[Error] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::crimson), fmt, std::forward<T>(args)...);

            fmt::print("\n");
        }

        static inline auto typeName = type_name<T>();
        Ref<LoggerOptions> mLoggerOptions;
    };

} // namespace SKIRNIR_NAMESPACE