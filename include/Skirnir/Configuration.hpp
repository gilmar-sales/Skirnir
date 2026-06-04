#pragma once

#include "Common.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

namespace SKIRNIR_NAMESPACE
{
    class ConfigurationOptions
    {
      public:
        virtual ~ConfigurationOptions() = default;

        /**
         * @brief Gets a configuration value by key path.
         * @param key The dot-separated key path (e.g., "logging.level")
         * @return The value as a string, or nullopt if not found
         */
        virtual std::optional<std::string> GetValue(std::string_view key) const = 0;

        /**
         * @brief Checks if a key exists in the configuration.
         * @param key The dot-separated key path
         * @return true if the key exists
         */
        virtual bool HasKey(std::string_view key) const = 0;
    };

    class JsonConfigurationOptions final : public ConfigurationOptions
    {
      public:
        JsonConfigurationOptions() = default;

        /**
         * @brief Loads configuration from a JSON file.
         * @param path Path to the JSON configuration file
         * @return true if loaded successfully
         */
        bool LoadFromFile(const std::filesystem::path& path);

        /**
         * @brief Loads configuration from a JSON string.
         * @param json The JSON string content
         * @return true if parsed successfully
         */
        bool LoadFromString(std::string_view json);

        std::optional<std::string> GetValue(std::string_view key) const override;
        bool                       HasKey(std::string_view key) const override;

      private:
        std::string mJsonContent;

        static std::optional<std::string> FindValueInJson(
            std::string_view json, std::string_view key);
        static std::string_view TrimWhitespace(std::string_view s);
        static std::string     ExtractStringValue(std::string_view json,
                                                   size_t         startPos);
    };

    /**
     * @brief Configuration builder for fluently constructing configuration.
     */
    class ConfigurationBuilder
    {
      public:
        ConfigurationBuilder() = default;

        /**
         * @brief Loads configuration from a JSON file.
         * @param path Path to the JSON file
         * @return Reference to this builder for chaining
         */
        ConfigurationBuilder& AddJsonFile(const std::filesystem::path& path);

        /**
         * @brief Adds a JSON string as configuration source.
         * @param json JSON string content
         * @return Reference to this builder for chaining
         */
        ConfigurationBuilder& AddJsonString(std::string_view json);

        /**
         * @brief Builds the configuration options.
         * @return A Ref to the created ConfigurationOptions
         */
        Ref<ConfigurationOptions> Build();

      private:
        std::string mJsonContent;
    };
} // namespace SKIRNIR_NAMESPACE