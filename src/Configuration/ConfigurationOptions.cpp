#include "Skirnir/Configuration/ConfigurationOptions.hpp"

#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <variant>

#include "Detail.hpp"

namespace SKIRNIR_NAMESPACE
{
    std::vector<std::string_view> ConfigurationOptions::SplitKey(
        std::string_view key)
    {
        std::vector<std::string_view> parts;
        while (!key.empty())
        {
            auto dot = key.find('.');
            if (dot == std::string_view::npos)
            {
                parts.push_back(key);
                break;
            }
            parts.push_back(key.substr(0, dot));
            key = key.substr(dot + 1);
        }
        return parts;
    }

    std::optional<simdjson::dom::element> ConfigurationOptions::Navigate(
        std::string_view key) const
    {
        auto parts = SplitKey(key);
        if (parts.empty())
            return mElement;

        simdjson::dom::element cursor = mElement;
        for (auto segment : parts)
        {
            if (!cursor.is_object())
                return std::nullopt;
            auto obj = cursor.get_object();
            for (auto kv : obj)
            {
                if (kv.key == segment)
                {
                    cursor = kv.value;
                    goto next;
                }
            }
            return std::nullopt;
        next:;
        }
        return cursor;
    }

    std::optional<std::string> ConfigurationOptions::GetValue(
        std::string_view key) const
    {
        auto el = Navigate(key);
        if (!el)
            return std::nullopt;
        if (el->is_null())
            return std::nullopt;
        return StringifyRaw(*el);
    }

    bool ConfigurationOptions::HasKey(std::string_view key) const
    {
        return Navigate(key).has_value();
    }

    bool ConfigurationOptions::GetBool(std::string_view key,
                                       bool             defaultValue) const
    {
        auto el = Navigate(key);
        if (!el || !el->is_bool())
            return defaultValue;
        bool v      = defaultValue;
        std::ignore = el->get_bool().get(v);
        return v;
    }

    int64_t ConfigurationOptions::GetInt(std::string_view key,
                                         int64_t          defaultValue) const
    {
        auto el = Navigate(key);
        if (!el)
            return defaultValue;
        if (el->is_int64())
        {
            int64_t v = defaultValue;
            if (el->get_int64().get(v) == simdjson::SUCCESS)
                return v;
        }
        if (el->is_uint64())
        {
            uint64_t v = 0;
            if (el->get_uint64().get(v) == simdjson::SUCCESS)
            {
                constexpr uint64_t kMax =
                    static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
                if (v > kMax)
                    return defaultValue;
                return static_cast<int64_t>(v);
            }
        }
        if (el->is_double())
        {
            double v = 0.0;
            if (el->get_double().get(v) == simdjson::SUCCESS)
                return static_cast<int64_t>(v);
        }
        return defaultValue;
    }

    double ConfigurationOptions::GetDouble(std::string_view key,
                                           double           defaultValue) const
    {
        auto el = Navigate(key);
        if (!el)
            return defaultValue;
        if (el->is_number())
        {
            double v = defaultValue;
            if (el->get_double().get(v) == simdjson::SUCCESS)
                return v;
        }
        return defaultValue;
    }

    std::string ConfigurationOptions::GetString(
        std::string_view key, std::string_view defaultValue) const
    {
        auto el = Navigate(key);
        if (!el)
            return std::string(defaultValue);
        if (el->is_string())
        {
            std::string_view sv = defaultValue;
            if (el->get_string().get(sv) == simdjson::SUCCESS)
                return std::string(sv);
        }
        if (el->is_null())
            return std::string(defaultValue);
        return Stringify(*el);
    }

    std::vector<std::string> ConfigurationOptions::GetArray(
        std::string_view key) const
    {
        std::vector<std::string> result;
        auto                     el = Navigate(key);
        if (!el || !el->is_array())
            return result;
        for (auto item : el->get_array())
        {
            if (item.is_string())
            {
                std::string_view sv;
                if (item.get_string().get(sv) == simdjson::SUCCESS)
                    result.emplace_back(sv);
            }
            else
            {
                result.emplace_back(Stringify(item));
            }
        }
        return result;
    }

    Ref<ConfigurationOptions> ConfigurationOptions::GetSection(
        std::string_view key) const
    {
        auto el = Navigate(key);
        if (!el)
            return MakeRef<ConfigurationOptions>();
        if (!el->is_object())
            return MakeRef<ConfigurationOptions>();

        // Serialize the sub-tree and parse it into a fresh parser so this
        // child ConfigurationOptions can outlive the parent parser.
        std::string slice  = detail::SerializeElement(*el);
        auto        parser = std::make_unique<simdjson::dom::parser>();
        auto        rootEl = detail::ParseOrThrow(
            *parser, slice, "section '" + std::string(key) + "'");
        return MakeRef<ConfigurationOptions>(std::move(parser), rootEl);
    }

    std::string ConfigurationOptions::Stringify(simdjson::dom::element el)
    {
        return detail::SerializeElement(el);
    }

    std::string ConfigurationOptions::StringifyRaw(simdjson::dom::element el)
    {
        // GetValue semantics: for strings, return the unquoted text; for
        // booleans, "true"/"false"; for numbers, the literal number; for
        // null, empty; for objects/arrays, the JSON representation.
        switch (el.type())
        {
            case simdjson::dom::element_type::NULL_VALUE:
                return {};
            case simdjson::dom::element_type::BOOL: {
                bool b;
                if (el.get_bool().get(b) == simdjson::SUCCESS)
                    return b ? "true" : "false";
                return {};
            }
            case simdjson::dom::element_type::INT64: {
                int64_t i;
                if (el.get_int64().get(i) == simdjson::SUCCESS)
                    return std::to_string(i);
                return {};
            }
            case simdjson::dom::element_type::UINT64: {
                uint64_t u;
                if (el.get_uint64().get(u) == simdjson::SUCCESS)
                    return std::to_string(u);
                return {};
            }
            case simdjson::dom::element_type::DOUBLE: {
                double d;
                if (el.get_double().get(d) == simdjson::SUCCESS)
                {
                    if (std::isnan(d) || std::isinf(d))
                        return {};
                    std::ostringstream oss;
                    oss << std::setprecision(17) << d;
                    return oss.str();
                }
                return {};
            }
            case simdjson::dom::element_type::STRING: {
                std::string_view sv;
                if (el.get_string().get(sv) == simdjson::SUCCESS)
                    return std::string(sv);
                return {};
            }
            default:
                return detail::SerializeElement(el);
        }
    }
} // namespace SKIRNIR_NAMESPACE
