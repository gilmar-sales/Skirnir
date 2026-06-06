#pragma once

#include <filesystem>

#include "Aliases.hpp"
#include "IConfigurationSource.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Parses configuration from a JSON file.
     */
    class JsonFileSource final : public IConfigurationSource
    {
      public:
        explicit JsonFileSource(std::filesystem::path path);

        simdjson::dom::element Load() override;

      private:
        std::filesystem::path  mPath;
        std::string            mContent;
        simdjson::dom::parser  mParser;
        simdjson::dom::element mElement;
        bool                   mLoaded = false;
    };
} // namespace SKIRNIR_NAMESPACE
