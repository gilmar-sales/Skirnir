#pragma once

#include "Common.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Severity levels used by @c Logger<T>.
     */
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
} // namespace SKIRNIR_NAMESPACE
