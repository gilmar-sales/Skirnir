#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Configuration/JsonObjectReader.hpp"

namespace SKIRNIR_NAMESPACE
{
    class ConfigurationBuilder;

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
         * @return Stringified JSON value, or nullopt if not found or the
         *         value is JSON null. An empty @p key returns the
         *         stringified root element (or nullopt if the root is
         *         null).
         */
        std::optional<std::string> GetValue(std::string_view key) const;

        /**
         * @brief Checks if a dot-separated key path exists.
         *
         * An empty @p key always returns true unless the root is null.
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
        Arc<ConfigurationOptions> GetSection(std::string_view key) const;

        /**
         * @brief Strongly-typed binding of a JSON sub-object into T.
         *
         * T must be default-constructible
         */
        template <typename T>
        Arc<T> Bind(std::string_view section = "") const
        {
            auto                result = MakeArc<T>();
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
                constexpr auto name = std::meta::identifier_of(member);
                if constexpr (name.size() > 0)
                {
                    reader.TryGet(name, result->[:member:]);
                }
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
} // namespace SKIRNIR_NAMESPACE
