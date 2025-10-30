#pragma once
#include <chrono>

#ifdef SKIRNIR_USE_FMT
    #include <fmt/chrono.h>
    #include <fmt/color.h>
    #include <fmt/core.h>
#else
    #include <print>
#endif

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
            if (mLoggerOptions->logLevel > LogLevel::Debug)
                return;

            fmt::print(fg(fmt::color::forest_green), "[Debug] {} '{}': ",
                       std::chrono::system_clock::now(), type_name<T>());

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
                       std::chrono::system_clock::now(), type_name<T>());

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
                       std::chrono::system_clock::now(), type_name<T>());

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
                       std::chrono::system_clock::now(), type_name<T>());

            fmt::print(fg(fmt::color::gold), fmt, std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogError(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Error)
                return;

            fmt::print(fg(fmt::color::crimson), "[Error] {} '{}': ",
                       std::chrono::system_clock::now(), type_name<T>());

            fmt::print(fg(fmt::color::crimson), fmt,
                       std::forward<TArgs>(args)...);

            fmt::print("\n");
        }

        template <typename... TArgs>
        inline void LogFatal(fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Error)
                return;

            const auto line = fmt::format(fmt, std::forward<TArgs>(args)...);

            fmt::print(fg(fmt::color::crimson), "[Fatal] {} '{}': {}\n",
                       std::chrono::system_clock::now(), type_name<T>(), line);

            throw std::runtime_error(line);
        }
#else

        template <typename... TArgs>
        inline void LogTrace(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Trace)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Trace] {} '{}': {}",
                         std::chrono::system_clock::now(), type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogDebug(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Debug)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Debug] {} '{}': {}",
                         std::chrono::system_clock::now(), type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogInformation(std::format_string<TArgs...> fmt,
                                   TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Information)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Information] {} '{}': {}",
                         std::chrono::system_clock::now(), type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogWarning(std::format_string<TArgs...> fmt,
                               TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Warning)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Warning] {} '{}': {}",
                         std::chrono::system_clock::now(), type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogError(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Error)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Error] {} '{}': {}",
                         std::chrono::system_clock::now(), type_name<T>(),
                         line);
        }

        template <typename... TArgs>
        inline void LogFatal(std::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (mLoggerOptions->logLevel > LogLevel::Fatal)
                return;

            const auto line = std::format(fmt, std::forward<TArgs>(args)...);

            std::println("[Fatal] {} '{}': {}",
                         std::chrono::system_clock::now(), type_name<T>(),
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

        Ref<LoggerOptions> mLoggerOptions;
    };

} // namespace SKIRNIR_NAMESPACE