#include "Detail.hpp"
#include "Skirnir/Logging/LogRecord.hpp"
#include "Skirnir/Logging/LogSinks/ConsoleSink.hpp"
#include "Skirnir/Logging/Format.hpp"

#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace SKIRNIR_NAMESPACE
{
    ConsoleSink::ConsoleSink(bool useColors) : mUseColors(useColors)
    {
    }

    void ConsoleSink::Write(const LogRecord& r)
    {
        const std::string prefix =
            detail::Format("[{}] {} '{}': ",
                        detail::LevelName(r.level),
                        detail::FormatTimestamp(r.timestamp),
                        detail::SanitizeForLog(r.category, true));

        std::string scopesStr;
        if (!r.scopes.empty())
        {
            scopesStr.push_back('[');
            for (std::size_t i = 0; i < r.scopes.size(); ++i)
            {
                if (i)
                    scopesStr.push_back('/');
                scopesStr.append(detail::SanitizeForLog(r.scopes[i], true));
            }
            scopesStr += "] ";
        }

        std::lock_guard<std::mutex> lock(mMutex);
        (void) mUseColors; // color toggle reserved for future fmt branch
#ifdef SKIRNIR_USE_FMT
        fmt::print("{}{}{}\n", prefix, scopesStr,
                   detail::SanitizeForLog(r.message, true));
#else
        std::print("{}{}{}\n", prefix, scopesStr,
                   detail::SanitizeForLog(r.message, true));
#endif
    }
} // namespace SKIRNIR_NAMESPACE
