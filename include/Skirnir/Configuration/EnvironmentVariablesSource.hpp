#pragma once

#include <string>

#include "Aliases.hpp"
#include "IConfigurationSource.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Source backed by process environment variables.
     *
     * If @p prefix is non-empty, only variables whose name starts with the
     * prefix are loaded and the prefix is stripped from the resulting key.
     * Double underscores ("__") in the variable name are translated to dots
     * (".") so nested sections can be expressed, e.g.
     *   @c SKIRNIR_LOGGING__LEVEL=Information  becomes
     *   @c { "logging": { "level": "Information" } }.
     *
     * Values are coerced: "true"/"false" become booleans, parseable integers
     * and doubles become numbers, everything else stays a string. This
     * matches the behavior of @c InMemorySource.
     */
    class EnvironmentVariablesSource final : public IConfigurationSource
    {
      public:
        explicit EnvironmentVariablesSource(std::string prefix = {});

        simdjson::dom::element Load() override;

      private:
        std::string            mPrefix;
        simdjson::dom::parser  mParser;
        simdjson::dom::element mElement;
        std::string            mSerialized;
        bool                   mDirty = true;

        std::string BuildJson() const;
    };
} // namespace SKIRNIR_NAMESPACE
