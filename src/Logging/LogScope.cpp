#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Logging/LogScope.hpp"
#include "Skirnir/Logging/Logger.hpp"

namespace SKIRNIR_NAMESPACE
{
    LogScope::LogScope(Arc<LoggerOptions> options, std::string name) :
        mOptions(options), mName(std::move(name))
    {
        if (auto opts = mOptions.lock())
        {
            opts->PushScope(mName);
        }
    }

    LogScope::~LogScope()
    {
        if (auto opts = mOptions.lock())
        {
            opts->PopScope();
        }
    }
} // namespace SKIRNIR_NAMESPACE
