#include "Skirnir/Logger.hpp"
#include "Skirnir/Configuration.hpp"

#include <algorithm>
#include <cstring>

namespace SKIRNIR_NAMESPACE
{
    static LogLevel ParseLogLevel(std::string_view level)
    {
        if (level == "Debug" || level == "debug" || level == "DEBUG")
            return LogLevel::Debug;
        if (level == "Trace" || level == "trace" || level == "TRACE")
            return LogLevel::Trace;
        if (level == "Information" || level == "Info" ||
            level == "information" || level == "INFO")
            return LogLevel::Information;
        if (level == "Warning" || level == "warn" || level == "WARNING")
            return LogLevel::Warning;
        if (level == "Error" || level == "error" || level == "ERROR")
            return LogLevel::Error;
        if (level == "Fatal" || level == "fatal" || level == "FATAL")
            return LogLevel::Fatal;
        if (level == "None" || level == "none" || level == "NONE")
            return LogLevel::None;

        return LogLevel::Information;
    }

    void LoggerOptions::ConfigureFrom(Ref<ConfigurationOptions> config,
                                      std::string_view          path)
    {
        if (!config)
            return;

        // Load default log level
        if (auto levelValue = config->GetValue(path); levelValue)
        {
            logLevel = ParseLogLevel(*levelValue);
        }

        // Load namespace-specific log levels
        // Look for pattern: logging.namespaces.{namespace}.level
        if (auto namespacesValue = config->GetValue("logging.namespaces");
            namespacesValue)
        {
            // Parse the namespaces JSON object
            std::string_view nsJson = *namespacesValue;
            size_t           pos    = 0;

            while (pos < nsJson.size())
            {
                // Find a key
                size_t keyStart = nsJson.find('"', pos);
                if (keyStart == std::string_view::npos)
                    break;

                size_t keyEnd = nsJson.find('"', keyStart + 1);
                if (keyEnd == std::string_view::npos)
                    break;

                std::string_view namespaceName =
                    nsJson.substr(keyStart + 1, keyEnd - keyStart - 1);

                // Find the value
                size_t colonPos = nsJson.find(':', keyEnd);
                if (colonPos == std::string_view::npos)
                    break;

                size_t valueStart = colonPos + 1;
                while (
                    valueStart < nsJson.size() &&
                    (nsJson[valueStart] == ' ' || nsJson[valueStart] == '\t'))
                {
                    valueStart++;
                }

                if (valueStart >= nsJson.size())
                    break;

                if (nsJson[valueStart] == '"')
                {
                    size_t valueEnd = nsJson.find('"', valueStart + 1);
                    if (valueEnd != std::string_view::npos)
                    {
                        std::string_view levelStr =
                            nsJson.substr(valueStart + 1,
                                          valueEnd - valueStart - 1);
                        mLogLevels[std::string(namespaceName)] =
                            ParseLogLevel(levelStr);
                        pos = valueEnd + 1;
                    }
                    else
                    {
                        pos = keyEnd + 1;
                    }
                }
                else
                {
                    pos = keyEnd + 1;
                }
            }
        }
    }

} // namespace SKIRNIR_NAMESPACE