#include "Skirnir/Configuration/ConfigurationBuilder.hpp"

#include "Skirnir/Configuration/ConfigurationOptions.hpp"
#include "Skirnir/Configuration/EnvironmentVariablesSource.hpp"
#include "Skirnir/Configuration/InMemorySource.hpp"
#include "Skirnir/Configuration/JsonFileSource.hpp"
#include "Skirnir/Configuration/JsonStringSource.hpp"

#include "Detail.hpp"

namespace SKIRNIR_NAMESPACE
{
    ConfigurationBuilder& ConfigurationBuilder::AddJsonFile(
        const std::filesystem::path& path)
    {
        mSources.push_back(MakeRef<JsonFileSource>(path));
        return *this;
    }

    ConfigurationBuilder& ConfigurationBuilder::AddJsonString(
        std::string_view json)
    {
        mSources.push_back(MakeRef<JsonStringSource>(
            std::string(json.data(), json.size())));
        return *this;
    }

    ConfigurationBuilder& ConfigurationBuilder::AddSource(
        Ref<IConfigurationSource> source)
    {
        if (source)
            mSources.push_back(std::move(source));
        return *this;
    }

    ConfigurationBuilder& ConfigurationBuilder::AddInMemory(
        std::initializer_list<std::pair<std::string, std::string>> entries)
    {
        mSources.push_back(MakeRef<InMemorySource>(entries));
        return *this;
    }

    ConfigurationBuilder& ConfigurationBuilder::AddEnvironmentVariables(
        std::string prefix)
    {
        mSources.push_back(
            MakeRef<EnvironmentVariablesSource>(std::move(prefix)));
        return *this;
    }

    Ref<ConfigurationOptions> ConfigurationBuilder::Build()
    {
        // Merge all sources into a single intermediate object tree. Sources
        // must stay alive for the duration of this merge so the underlying
        // parsers do not go out from under us.
        auto merged = std::make_shared<detail::JsonValue>(
            detail::JsonValue::FromObject());
        for (auto& source : mSources)
        {
            simdjson::dom::element root = source->Load();
            // Non-object roots are ignored: a configuration source is
            // expected to provide a top-level object.
            if (!root.is_object())
                continue;
            auto converted = detail::Convert(root);
            detail::MergeInto(merged, converted);
        }

        std::string json;
        detail::Serialize(merged, json, true);

        auto parser = std::make_unique<simdjson::dom::parser>();
        auto rootEl = detail::ParseOrThrow(
            *parser, json, "merged configuration");
        return MakeRef<ConfigurationOptions>(std::move(parser), rootEl);
    }
} // namespace SKIRNIR_NAMESPACE
