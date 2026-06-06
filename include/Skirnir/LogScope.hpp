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
