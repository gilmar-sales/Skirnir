#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <functional>
#include <system_error>

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Logging/Logger.hpp"
#include "Skirnir/Logging/LogLevel.hpp"
#include "Skirnir/Hotswap/Platform.hpp"

// `SKR_HOTSWAP_LOG_PREFIX` is preserved from the upstream hscpp API so existing
// call sites compile unchanged. The Skirnir Logger already records the source
// category, so the prefix is empty by default; the Skirnir record's `category`
// (`skr::hotswap::log::LogCategory`) carries the diagnostic context.
#ifndef SKR_HOTSWAP_LOG_PREFIX
    #define SKR_HOTSWAP_LOG_PREFIX ""
#endif

namespace skr::hotswap
{
    class LoggerOptions;
}

namespace skr::hotswap::log
{
    /**
     * @brief Tag type used as the Skirnir Logger category for every log
     *        record produced by the Hotswap runtime.
     */
    struct LogCategory
    {
    };

    /**
     * @brief Compatibility enum mirroring upstream hscpp's level set. Maps to
     *        @ref SKIRNIR_NAMESPACE::LogLevel on dispatch.
     */
    enum class Level
    {
        Debug,
        Info,
        Warning,
        Error,
    };

    /**
     * @brief Stream terminator. When streamed into a @ref Stream the buffered
     *        message is flushed to the Skirnir Logger.
     */
    class End
    {
      public:
        End() = default;
        explicit End(const std::string& str);

        const std::string& Str() const;

      private:
        std::string m_Str;
    };

    /**
     * @brief Stream manipulator that appends the current platform-specific
     *        last-error string.
     */
    class LastOsError
    {
    };

    /**
     * @brief Stream manipulator that appends a translated OS error string.
     */
    class OsError
    {
      public:
        explicit OsError(const std::error_code& errorCode);

        std::error_code ErrorCode() const;

      private:
        std::error_code m_ErrorCode;
    };

    /**
     * @brief Buffered log message.
     *
     * Accumulates fragments streamed via `operator<<` and dispatches the
     * composed message to the underlying Skirnir @c Logger<LogCategory> when
     * @ref End is streamed in.
     */
    class Stream
    {
      public:
        using EndCallback = std::function<void(const std::string&)>;

        Stream() = default;
        Stream(SKIRNIR_NAMESPACE::LogLevel level,
               bool                       enabled,
               EndCallback                endCb = {});

        Stream& operator<<(const std::string& str);
        Stream& operator<<(const LastOsError& lastError);
        Stream& operator<<(const OsError& errorLog);
        void    operator<<(const End& endLog);

        template <typename T>
        Stream& operator<<(const T& val)
        {
            if (!m_bEnabled)
            {
                return *this;
            }

            if constexpr (requires { std::declval<std::decay_t<T>>().data(); std::declval<std::decay_t<T>>().size(); })
            {
                using V = std::remove_cvref_t<decltype(*std::declval<std::decay_t<T>>().data())>;
                if constexpr (std::is_same_v<V, char8_t>)
                {
                    m_Stream << std::string(reinterpret_cast<const char*>(val.data()), val.size());
                }
                else
                {
                    m_Stream << val;
                }
            }
            else
            {
                m_Stream << val;
            }
            return *this;
        }

      private:
        SKIRNIR_NAMESPACE::LogLevel m_Level {SKIRNIR_NAMESPACE::LogLevel::Information};
        bool                        m_bEnabled {true};
        EndCallback                 m_EndCb;
        std::stringstream           m_Stream;
    };

    // ---- Factory functions -------------------------------------------------

    Stream Debug();
    Stream Info();
    Stream Warning();
    Stream Error();

    /**
     * @brief Returns a stream that, on flush, dispatches as @c LogInformation
     *        AND invokes the legacy OutputDebugString callback when enabled.
     *        Gated by @ref EnableBuildLogging / @ref DisableBuildLogging.
     */
    Stream Build();

    // ---- Configuration -----------------------------------------------------

    /**
     * @brief Sets the minimum @ref Level at which factory functions produce
     *        enabled streams. Records below this level are silently dropped.
     */
    void SetLevel(Level level);

    void EnableBuildLogging();
    void DisableBuildLogging();

    /**
     * @brief Installs the @c LoggerOptions used by the hotswap logger.
     *
     * The hotswap subsystem uses a process-wide default @c LoggerOptions
     * instance, lazily created on first use. Passing a non-null @p options
     * replaces that default for the lifetime of the process; subsequent
     * log records are dispatched through the provided options. Passing
     * @c nullptr restores the lazily-created default.
     */
    void Configure(skr::Arc<SKIRNIR_NAMESPACE::LoggerOptions> options);

    /**
     * @brief Returns the currently installed @c LoggerOptions, creating the
     *        process default on first call.
     */
    skr::Arc<SKIRNIR_NAMESPACE::LoggerOptions> Options();
} // namespace skr::hotswap::log
