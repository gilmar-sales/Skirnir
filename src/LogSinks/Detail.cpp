#include "Detail.hpp"

#include "Skirnir/LogLevel.hpp"

namespace SKIRNIR_NAMESPACE::detail
{
    const char* LevelName(LogLevel lvl)
    {
        switch (lvl)
        {
            case LogLevel::Debug:
                return "Debug";
            case LogLevel::Trace:
                return "Trace";
            case LogLevel::Information:
                return "Information";
            case LogLevel::Warning:
                return "Warning";
            case LogLevel::Error:
                return "Error";
            case LogLevel::Fatal:
                return "Fatal";
            case LogLevel::None:
                return "None";
        }
        return "?";
    }
} // namespace SKIRNIR_NAMESPACE::detail
