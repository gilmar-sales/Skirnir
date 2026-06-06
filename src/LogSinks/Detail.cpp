#include "Detail.hpp"

#include "Skirnir/LogLevel.hpp"

#include <format>

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

    std::string SanitizeForLog(std::string_view s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            switch (c)
            {
                case '\r':
                    out.append("\\r");
                    break;
                case '\n':
                    out.append("\\n");
                    break;
                case '\t':
                    out.append("\\t");
                    break;
                case '\0':
                    out.append("\\0");
                    break;
                case '\b':
                    out.append("\\b");
                    break;
                case '\f':
                    out.append("\\f");
                    break;
                case '\v':
                    out.append("\\v");
                    break;
                case '\x1b':
                    out.append("\\x1b");
                    break;
                default:
                {
                    const auto u = static_cast<unsigned char>(c);
                    if (u < 0x20 || u == 0x7f)
                    {
                        out.append(std::format("\\x{:02x}", u));
                    }
                    else
                    {
                        out.push_back(c);
                    }
                    break;
                }
            }
        }
        return out;
    }
} // namespace SKIRNIR_NAMESPACE::detail
