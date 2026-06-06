#pragma once

#include "Skirnir/Logging/LogLevel.hpp"

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief A complete log entry, ready to be handed to a sink.
     */
    struct LogRecord
    {
        LogLevel                              level = LogLevel::Information;
        std::chrono::system_clock::time_point timestamp {};
        std::string                           category {};
        std::string                           message {};
        std::vector<std::string>              scopes {};
    };
} // namespace SKIRNIR_NAMESPACE
