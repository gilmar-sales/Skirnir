#include "Skirnir/Hotswap/Log.hpp"

#include <memory>
#include <mutex>

#include "Skirnir/Hotswap/Platform.hpp"
#include "Skirnir/Logging/LogSinks/ConsoleSink.hpp"
#include "Skirnir/Logging/LogRecord.hpp"

namespace skr::hotswap::log
{
    namespace
    {
        Level                                                        s_Level   = Level::Info;
        bool                                                         s_bLogBuild = true;
        std::mutex                                                   s_OptionsMutex;
        skr::Arc<SKIRNIR_NAMESPACE::LoggerOptions>                   s_Options;

        SKIRNIR_NAMESPACE::LogLevel ToSkirnirLevel(Level level)
        {
            switch (level)
            {
                case Level::Debug:   return SKIRNIR_NAMESPACE::LogLevel::Debug;
                case Level::Info:    return SKIRNIR_NAMESPACE::LogLevel::Information;
                case Level::Warning: return SKIRNIR_NAMESPACE::LogLevel::Warning;
                case Level::Error:   return SKIRNIR_NAMESPACE::LogLevel::Error;
            }
            return SKIRNIR_NAMESPACE::LogLevel::Information;
        }

        bool IsLevelActive(Level level)
        {
            return static_cast<int>(level) >= static_cast<int>(s_Level);
        }

        const char* LevelName(SKIRNIR_NAMESPACE::LogLevel lvl)
        {
            using SKIRNIR_NAMESPACE::LogLevel;
            switch (lvl)
            {
                case LogLevel::Debug:       return "DEBUG";
                case LogLevel::Trace:       return "TRACE";
                case LogLevel::Information: return "INFO";
                case LogLevel::Warning:     return "WARN";
                case LogLevel::Error:       return "ERROR";
                case LogLevel::Fatal:       return "FATAL";
                case LogLevel::None:        return "NONE";
            }
            return "INFO";
        }

        skr::Arc<SKIRNIR_NAMESPACE::LoggerOptions> CreateDefaultOptions()
        {
            auto options = skr::MakeArc<SKIRNIR_NAMESPACE::LoggerOptions>();
            options->AddSink(skr::MakeArc<SKIRNIR_NAMESPACE::ConsoleSink>(false));
            options->logLevel = SKIRNIR_NAMESPACE::LogLevel::Trace;
            return options;
        }
    } // namespace

    End::End(const std::string& str) : m_Str(str)
    {
    }

    const std::string& End::Str() const
    {
        return m_Str;
    }

    OsError::OsError(const std::error_code& errorCode) : m_ErrorCode(errorCode)
    {
    }

    std::error_code OsError::ErrorCode() const
    {
        return m_ErrorCode;
    }

    Stream::Stream(SKIRNIR_NAMESPACE::LogLevel level, bool enabled, EndCallback endCb)
        : m_Level(level), m_bEnabled(enabled), m_EndCb(std::move(endCb))
    {
    }

    Stream& Stream::operator<<(const std::string& str)
    {
        if (!m_bEnabled || str.empty())
        {
            return *this;
        }
        m_Stream << str;
        return *this;
    }

    Stream& Stream::operator<<(const LastOsError& /*lastOsError*/)
    {
        if (!m_bEnabled)
        {
            return *this;
        }
        m_Stream << "[" << platform::GetLastErrorString() << "]";
        return *this;
    }

    Stream& Stream::operator<<(const OsError& osError)
    {
        if (!m_bEnabled)
        {
            return *this;
        }
        m_Stream << "[" << osError.ErrorCode().message() << "]";
        return *this;
    }

    void Stream::operator<<(const End& endLog)
    {
        if (!m_bEnabled)
        {
            return;
        }

        *this << endLog.Str();
        std::string message = m_Stream.str();

        auto options = log::Options();

        SKIRNIR_NAMESPACE::LogRecord record;
        record.level     = m_Level;
        record.timestamp = std::chrono::system_clock::now();
        record.category  = "skr::hotswap::log::LogCategory";
        record.message   = std::move(message);
        record.scopes    = options->CurrentScopes();

        if (static_cast<int>(record.level) < static_cast<int>(options->logLevel))
        {
            return;
        }

        options->Dispatch(record);

        if (m_EndCb)
        {
            m_EndCb(record.message);
        }
    }

    Stream Debug()
    {
        return Stream(SKIRNIR_NAMESPACE::LogLevel::Debug, IsLevelActive(Level::Debug));
    }

    Stream Info()
    {
        return Stream(SKIRNIR_NAMESPACE::LogLevel::Information, IsLevelActive(Level::Info));
    }

    Stream Warning()
    {
        return Stream(SKIRNIR_NAMESPACE::LogLevel::Warning, IsLevelActive(Level::Warning));
    }

    Stream Error()
    {
        return Stream(SKIRNIR_NAMESPACE::LogLevel::Error, IsLevelActive(Level::Error));
    }

    Stream Build()
    {
        Stream::EndCallback cb = [](const std::string& msg) {
            platform::WriteDebugString(msg);
        };
        return Stream(SKIRNIR_NAMESPACE::LogLevel::Information, s_bLogBuild, std::move(cb));
    }

    void SetLevel(Level level)
    {
        s_Level = level;
        auto options = log::Options();
        options->logLevel = ToSkirnirLevel(level);
    }

    void EnableBuildLogging()
    {
        s_bLogBuild = true;
    }

    void DisableBuildLogging()
    {
        s_bLogBuild = false;
    }

    void Configure(skr::Arc<SKIRNIR_NAMESPACE::LoggerOptions> options)
    {
        std::lock_guard<std::mutex> lock(s_OptionsMutex);
        if (options)
        {
            s_Options = std::move(options);
        }
        else
        {
            s_Options = CreateDefaultOptions();
        }
        s_Options->logLevel = ToSkirnirLevel(s_Level);
    }

    skr::Arc<SKIRNIR_NAMESPACE::LoggerOptions> Options()
    {
        std::lock_guard<std::mutex> lock(s_OptionsMutex);
        if (!s_Options)
        {
            s_Options = CreateDefaultOptions();
        }
        return s_Options;
    }
} // namespace skr::hotswap::log
