#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Configuration/IConfigurationSource.hpp"

namespace SKIRNIR_NAMESPACE
{
    class ConfigurationOptions;

    /**
     * @brief Fluent builder that merges multiple configuration sources.
     *
     * Later sources override earlier ones. Object keys are merged
     * recursively; non-object values are replaced wholesale.
     */
    class ConfigurationBuilder
    {
      public:
        ConfigurationBuilder() = default;

        ConfigurationBuilder& AddJsonFile(const std::filesystem::path& path);
        ConfigurationBuilder& AddJsonString(std::string_view json);
        ConfigurationBuilder& AddSource(Arc<IConfigurationSource> source);
        ConfigurationBuilder& AddInMemory(
            std::initializer_list<std::pair<std::string, std::string>> entries);
        ConfigurationBuilder& AddEnvironmentVariables(std::string prefix = {});

        Arc<ConfigurationOptions> Build();

      private:
        std::vector<Arc<IConfigurationSource>> mSources;
    };
} // namespace SKIRNIR_NAMESPACE
