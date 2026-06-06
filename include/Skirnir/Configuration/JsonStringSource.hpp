#pragma once

#include <string>

#include "Aliases.hpp"
#include "IConfigurationSource.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Parses configuration from a JSON string.
     */
    class JsonStringSource final : public IConfigurationSource
    {
      public:
        explicit JsonStringSource(std::string json);

        simdjson::dom::element Load() override;

      private:
        std::string            mContent;
        simdjson::dom::parser  mParser;
        simdjson::dom::element mElement;
        bool                   mLoaded = false;
    };
} // namespace SKIRNIR_NAMESPACE
