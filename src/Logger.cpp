export module Skirnir:Logger;

import std;

import :Core;
import :Reflection;

export namespace skr
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

    struct LoggerOptions
    {
#ifdef NDEBUG
        LogLevel logLevel = LogLevel::Trace;
#else
        LogLevel logLevel = LogLevel::Debug;
#endif
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
        }

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
            if (!assertion)
            {
                LogFatal(fmt, std::forward<TArgs>(args)...);
            }
        }

        Ref<LoggerOptions> mLoggerOptions;
    };

} // namespace skr
