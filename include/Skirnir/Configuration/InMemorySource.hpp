#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <utility>

#include "Aliases.hpp"
#include "IConfigurationSource.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief In-memory source with string overrides, useful for tests and
     *        environment-style overrides.
     */
    class InMemorySource final : public IConfigurationSource
    {
      public:
        InMemorySource() = default;

        InMemorySource(
            std::initializer_list<std::pair<std::string, std::string>> entries)
        {
            for (const auto& [k, v] : entries)
            {
                mFlat[k] = v;
            }
        }

        /**
         * @brief Sets a flat key=value entry. Dotted keys become nested
         *        objects (e.g. "logging.level" -> {"logging": {"level": ...}}).
         */
        InMemorySource& Set(std::string key, std::string value)
        {
            mFlat[std::move(key)] = std::move(value);
            mDirty                = true;
            return *this;
        }

        simdjson::dom::element Load() override;

      private:
        std::map<std::string, std::string> mFlat;
        simdjson::dom::parser              mParser;
        simdjson::dom::element             mElement;
        std::string                        mSerialized;
        bool                               mDirty = true;

        std::string BuildJson() const;
    };
} // namespace SKIRNIR_NAMESPACE
