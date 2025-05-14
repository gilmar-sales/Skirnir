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
    };

    template <typename T>
    constexpr char* type_name()
    {
#if defined(__clang__)
        // __PRETTY_FUNCTION__ on Clang/GCC looks like:
        //   std::string type_name() [with T = MyNamespace::MyType]
        static std::string pretty = __PRETTY_FUNCTION__;
        static auto        start  = pretty.find("T = ") + 4;
        static auto        end    = pretty.rfind(']');
        static auto        result = pretty.substr(start, end - start);
        return result.data();
#elif defined(__GNUC__)
        // __PRETTY_FUNCTION__ on Clang/GCC looks like:
        //   std::string type_name() [with T = MyNamespace::MyType]
        static std::string pretty = __PRETTY_FUNCTION__;
        static auto        start  = pretty.find("T = ") + 4;
        static auto        end = std::min(pretty.rfind(']'), pretty.rfind(';'));
        static auto        result = pretty.substr(start, end - start);
        return result.data();
#elif defined(_MSC_VER)
        // __FUNCSIG__ on MSVC looks like:
        //   std::string __cdecl type_name<TYPE>(void)
        static std::string pretty = __FUNCSIG__;
        static auto        start  = pretty.find("type_name<") + 16;
        static auto        end    = pretty.rfind(">(void)");
        static auto        result = pretty.substr(start, end - start);
        return result.data();
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
            if (mLoggerOptions->logLevel > LogLevel::Debug)
                return;

            fmt::print(fg(fmt::color::forest_green), "[Debug] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::forest_green), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogTrace(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Trace)
                return;

            fmt::print(fg(fmt::color::gainsboro), "[Trace] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::gainsboro), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogInformation(fmt::format_string<TArgs...> fmt,
                                   TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Information)
                return;

            fmt::print(fg(fmt::color::sky_blue), "[Information] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::sky_blue), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogWarning(fmt::format_string<TArgs...> fmt,
                               TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Warning)
                return;

            fmt::print(fg(fmt::color::gold), "[Warning] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::gold), fmt, std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogError(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Error)
                return;

            fmt::print(fg(fmt::color::crimson), "[Error] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::crimson), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogFatal(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Error)
                return;

            fmt::print(fg(fmt::color::crimson), "[Fatal] {} '{}': ",
                       std::chrono::system_clock::now(), typeName);

            fmt::print(fg(fmt::color::crimson), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
            abort();
        }

        static inline auto typeName = type_name<T>();
        Ref<LoggerOptions> mLoggerOptions;
    };

} // namespace SKIRNIR_NAMESPACE