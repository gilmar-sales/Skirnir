#pragma once

#include "Common.hpp"
#include "Reflection.hpp"

#define SIMDJSON_STATIC_REFLECTION 1
#include <simdjson.h>

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace SKIRNIR_NAMESPACE
{
    using ConfigurationElement = simdjson::dom::element;
    using ConfigurationObject  = simdjson::dom::object;
    using ConfigurationArray   = simdjson::dom::array;

    /**
     * @brief Source of configuration data, producing a parsed JSON tree.
     */
    class IConfigurationSource
    {
      public:
        virtual ~IConfigurationSource() = default;

        /**
         * @brief Parse and return the root element of this source.
         *
         * The source owns the underlying parser; the returned element is
         * valid only while this source is alive. @c Load() may be called
         * more than once; subsequent calls should return the cached result.
         */
        virtual simdjson::dom::element Load() = 0;
    };

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

    /**
     *
     * Looks up keys case-sensitively in the underlying JSON object and
     * converts the value to the requested type. Missing or type-mismatched
     * keys are reported as failures and leave the destination untouched.
     */
    class JsonObjectReader
    {
      public:
        explicit JsonObjectReader(const simdjson::dom::object& obj) : mObj(obj)
        {
        }

        /** @brief Returns true if @p key is present in the object. */
        bool Contains(std::string_view key) const
        {
            for (auto kv : mObj)
            {
                if (kv.key == key)
                    return true;
            }
            return false;
        }

        /**
         * @brief Tries to read @p key into @p out. Returns true on success.
         */
        template <typename T>
        bool TryGet(std::string_view key, T& out) const
        {
            for (auto kv : mObj)
            {
                if (kv.key == key)
                {
                    return Assign(out, kv.value);
                }
            }
            return false;
        }

      private:
        template <typename T>
        static bool Assign(T& field, simdjson::dom::element el);

        template <typename T>
        static constexpr bool is_vector_of_string_v = false;

        template <typename Alloc>
        static constexpr bool
            is_vector_of_string_v<std::vector<std::string, Alloc>> = true;

        const simdjson::dom::object& mObj;
    };

    template <typename T>
    bool JsonObjectReader::Assign(T& field, simdjson::dom::element el)
    {
        using U = std::remove_cvref_t<T>;

        if constexpr (std::is_same_v<U, bool>)
        {
            if (!el.is_bool())
                return false;
            bool v = false;
            if (el.get_bool().get(v) != simdjson::SUCCESS)
                return false;
            field = v;
            return true;
        }
        else if constexpr (std::is_integral_v<U>)
        {
            if (el.is_int64())
            {
                int64_t v = 0;
                if (el.get_int64().get(v) != simdjson::SUCCESS)
                    return false;
                field = static_cast<U>(v);
                return true;
            }
            if (el.is_uint64())
            {
                uint64_t v = 0;
                if (el.get_uint64().get(v) != simdjson::SUCCESS)
                    return false;
                field = static_cast<U>(v);
                return true;
            }
            if (el.is_double())
            {
                double v = 0.0;
                if (el.get_double().get(v) != simdjson::SUCCESS)
                    return false;
                field = static_cast<U>(v);
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<U, double>)
        {
            if (!el.is_number())
                return false;
            double v = 0.0;
            if (el.get_double().get(v) != simdjson::SUCCESS)
                return false;
            field = v;
            return true;
        }
        else if constexpr (std::is_same_v<U, std::string>)
        {
            if (el.is_string())
            {
                std::string_view sv;
                if (el.get_string().get(sv) != simdjson::SUCCESS)
                    return false;
                field = std::string(sv);
                return true;
            }
            return false;
        }
        else if constexpr (is_vector_of_string_v<U>)
        {
            if (!el.is_array())
                return false;
            for (auto item : el.get_array())
            {
                if (!item.is_string())
                    return false;
                std::string_view sv;
                if (item.get_string().get(sv) != simdjson::SUCCESS)
                    return false;
                field.emplace_back(sv);
            }
            return true;
        }
        else
        {
            // Nested reflected struct.
            if (!el.is_object())
                return false;
            U                nested {};
            JsonObjectReader nestedReader(el.get_object());

            template for (constexpr auto member : std::define_static_array(
                              std::meta::nonstatic_data_members_of(
                                  ^^U,
                                  std::meta::access_context::current())))
            {
                nestedReader.TryGet(std::meta::identifier_of(member),
                                    nested.[:member:]);
            }
            field = std::move(nested);
            return true;
        }
    }

    /**
     * @brief Strongly-typed view over a configuration tree.
     *
     * Each instance owns the parser that backs its element, so a sub-section
     * can outlive the configuration it was derived from.
     */
    class ConfigurationOptions
    {
      public:
        ConfigurationOptions() = default;

        // --- Compatibility API
        // -------------------------------------------------

        /**
         * @brief Gets a configuration value by dot-separated key path.
         * @return Stringified JSON value, or nullopt if not found.
         */
        std::optional<std::string> GetValue(std::string_view key) const;

        /**
         * @brief Checks if a dot-separated key path exists.
         */
        bool HasKey(std::string_view key) const;

        // --- Typed accessors
        // ---------------------------------------------------

        bool    GetBool(std::string_view key, bool defaultValue = false) const;
        int64_t GetInt(std::string_view key, int64_t defaultValue = 0) const;
        double GetDouble(std::string_view key, double defaultValue = 0.0) const;
        std::string GetString(std::string_view key,
                              std::string_view defaultValue = "") const;
        std::vector<std::string> GetArray(std::string_view key) const;

        /**
         * @brief Returns a sub-section as a new ConfigurationOptions that
         *        owns its own parser.
         */
        Ref<ConfigurationOptions> GetSection(std::string_view key) const;

        /**
         * @brief Strongly-typed binding of a JSON sub-object into T.
         *
         * T must be default-constructible
         */
        template <typename T>
        T Bind(std::string_view section = "") const
        {
            T                   result {};
            ConfigurationObject obj;

            if (section.empty())
            {
                if (!mElement.is_object())
                    return result;
                obj = mElement.get_object();
            }
            else
            {
                auto sub = Navigate(section);
                if (!sub || !sub->is_object())
                    return result;
                obj = sub->get_object();
            }

            JsonObjectReader reader(obj);

            template for (constexpr auto member : std::define_static_array(
                              std::meta::nonstatic_data_members_of(
                                  ^^T,
                                  std::meta::access_context::current())))
            {
                reader.TryGet(std::meta::identifier_of(member),
                              result.[:member:]);
            }
            return result;
        }

        // --- Internal helpers (used by Bind / etc.)
        // ---------------------------

        simdjson::dom::element Root() const { return mElement; }

        /**
         * @brief Invokes @p fn(key, value) for each member of a sub-object.
         * @param section Dot-separated path to a sub-object.
         * @param fn      Callback receiving the member name and its
         *                simdjson::dom::element.
         */
        template <typename Fn>
        void ForEachMember(std::string_view section, Fn&& fn) const
        {
            auto sub = Navigate(section);
            if (!sub || !sub->is_object())
                return;
            for (auto kv : sub->get_object())
            {
                std::string_view k = kv.key;
                fn(k, kv.value);
            }
        }

        // --- Internal: builder / section construction -----------------------

        /// @cond INTERNAL
      public:
        ConfigurationOptions(std::unique_ptr<simdjson::dom::parser> parser,
                             simdjson::dom::element                 element) :
            mParser(std::move(parser)), mElement(element)
        {
        }
        /// @endcond

      private:
        std::unique_ptr<simdjson::dom::parser> mParser;
        simdjson::dom::element                 mElement;

        std::optional<simdjson::dom::element> Navigate(
            std::string_view key) const;

        static std::vector<std::string_view> SplitKey(std::string_view key);

        static std::string Stringify(simdjson::dom::element el);
        static std::string StringifyRaw(simdjson::dom::element el);

        friend class ConfigurationBuilder;
    };

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
        ConfigurationBuilder& AddSource(Ref<IConfigurationSource> source);
        ConfigurationBuilder& AddInMemory(
            std::initializer_list<std::pair<std::string, std::string>> entries);

        Ref<ConfigurationOptions> Build();

      private:
        std::vector<Ref<IConfigurationSource>> mSources;
    };
} // namespace SKIRNIR_NAMESPACE
