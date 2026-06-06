#include "Skirnir/LogSinks/ConsoleSink.hpp"

#include "Skirnir/LogRecord.hpp"
#include "Detail.hpp"

#include <format>
#include <mutex>
#include <print>
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
            std::format("[{}] {} '{}': ",
                        detail::LevelName(r.level),
                        r.timestamp,
                        detail::SanitizeForLog(r.category));

        std::string scopesStr;
        if (!r.scopes.empty())
        {
            scopesStr.push_back('[');
            for (std::size_t i = 0; i < r.scopes.size(); ++i)
            {
                if (i)
                    scopesStr.push_back('/');
                scopesStr.append(detail::SanitizeForLog(r.scopes[i]));
            }
            scopesStr += "] ";
        }

        std::lock_guard<std::mutex> lock(mMutex);
        (void) mUseColors; // color toggle reserved for future fmt branch
        std::print("{}{}{}\n", prefix, scopesStr, detail::SanitizeForLog(r.message));
    }
} // namespace SKIRNIR_NAMESPACE
