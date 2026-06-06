#pragma once

#include "Logger.hpp"

#include <string>

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief RAII handle returned by @c LoggerOptions::BeginScope.
     *
     * Pushing a scope prepends its name to every @c LogRecord dispatched
     * by any @c Logger<T> on the same thread, until the scope is
     * destroyed.
     *
     * @attention Thread affinity: the scope stack is per-thread. The
     * destructor MUST run on the same thread that constructed the
     * @c LogScope, otherwise the @c PopScope call will operate on the
     * wrong thread's stack and may leak the scope name into another
     * request's log. This is the caller's responsibility when the
     * handle is captured into asynchronous tasks.
     */
    class LogScope
    {
      public:
        LogScope(Ref<LoggerOptions> options, std::string name);
        ~LogScope();

        LogScope(const LogScope&)            = delete;
        LogScope& operator=(const LogScope&) = delete;
        LogScope(LogScope&&)                 = delete;
        LogScope& operator=(LogScope&&)      = delete;

        const std::string& Name() const noexcept
        {
            return mName;
        }

      private:
        WeakRef<LoggerOptions> mOptions;
        std::string            mName;
    };
} // namespace SKIRNIR_NAMESPACE
