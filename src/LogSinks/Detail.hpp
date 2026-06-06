#pragma once

#include "Skirnir/LogLevel.hpp"

#include <string>
#include <string_view>

namespace SKIRNIR_NAMESPACE::detail
{
    const char* LevelName(LogLevel lvl);

    std::string SanitizeForLog(std::string_view s);
}
