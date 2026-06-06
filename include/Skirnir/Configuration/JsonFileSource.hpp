#pragma once

#include <cstdint>
#include <filesystem>

#include "Skirnir/Configuration/Aliases.hpp"
#include "Skirnir/Configuration/IConfigurationSource.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Parses configuration from a JSON file.
     *
     * To prevent denial-of-service via attacker-controlled paths, the
     * maximum accepted file size is capped at @c maxSize bytes (default
     * 16 MiB). Set @c maxSize to a negative value to disable the cap.
     */
    class JsonFileSource final : public IConfigurationSource
    {
      public:
        static constexpr std::int64_t DefaultMaxSize = 16 * 1024 * 1024;

        explicit JsonFileSource(std::filesystem::path path,
                                std::int64_t          maxSize = DefaultMaxSize);

        simdjson::dom::element Load() override;

      private:
        std::filesystem::path  mPath;
        std::int64_t           mMaxSize;
        std::string            mContent;
        simdjson::dom::parser  mParser;
        simdjson::dom::element mElement;
        bool                   mLoaded = false;
    };
} // namespace SKIRNIR_NAMESPACE
