#include "Skirnir/Configuration.hpp"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <variant>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace SKIRNIR_NAMESPACE
{
    namespace
    {
        // Intermediate merge representation; built from simdjson elements
        // and serialized back to a JSON string for the final parse.
        struct JsonValue;

        using JsonObject = std::map<std::string, std::shared_ptr<JsonValue>>;
        using JsonArray  = std::vector<std::shared_ptr<JsonValue>>;

        struct JsonValue
        {
            std::variant<std::monostate, bool, int64_t, double, std::string,
                         std::shared_ptr<JsonObject>,
                         std::shared_ptr<JsonArray>>
                data;

            static JsonValue Null() { return JsonValue { std::monostate {} }; }

            static JsonValue FromBool(bool b) { return JsonValue { b }; }

            static JsonValue FromInt(int64_t i) { return JsonValue { i }; }

            static JsonValue FromDouble(double d) { return JsonValue { d }; }

            static JsonValue FromString(std::string s)
            {
                return JsonValue { std::move(s) };
            }

            static JsonValue FromObject()
            {
                return JsonValue { std::make_shared<JsonObject>() };
            }

            static JsonValue FromArray()
            {
                return JsonValue { std::make_shared<JsonArray>() };
            }

            bool IsObject() const
            {
                return std::holds_alternative<std::shared_ptr<JsonObject>>(
                    data);
            }
            bool IsArray() const
            {
                return std::holds_alternative<std::shared_ptr<JsonArray>>(data);
            }
            JsonObject& AsObject()
            {
                return *std::get<std::shared_ptr<JsonObject>>(data);
            }
            const JsonObject& AsObject() const
            {
                return *std::get<std::shared_ptr<JsonObject>>(data);
            }
            JsonArray& AsArray()
            {
                return *std::get<std::shared_ptr<JsonArray>>(data);
            }
            const JsonArray& AsArray() const
            {
                return *std::get<std::shared_ptr<JsonArray>>(data);
            }
        };

        std::shared_ptr<JsonValue> MakeNull()
        {
            return std::make_shared<JsonValue>(JsonValue::Null());
        }

        std::shared_ptr<JsonValue> Convert(simdjson::dom::element el)
        {
            simdjson::dom::element_type t = el.type();
            switch (t)
            {
                case simdjson::dom::element_type::NULL_VALUE:
                    return MakeNull();
                case simdjson::dom::element_type::BOOL: {
                    bool v;
                    if (el.get_bool().get(v) != simdjson::SUCCESS)
                        return MakeNull();
                    return std::make_shared<JsonValue>(JsonValue::FromBool(v));
                }
                case simdjson::dom::element_type::INT64: {
                    int64_t v;
                    if (el.get_int64().get(v) != simdjson::SUCCESS)
                        return MakeNull();
                    return std::make_shared<JsonValue>(JsonValue::FromInt(v));
                }
                case simdjson::dom::element_type::UINT64: {
                    uint64_t v;
                    if (el.get_uint64().get(v) != simdjson::SUCCESS)
                        return MakeNull();
                    return std::make_shared<JsonValue>(
                        JsonValue::FromInt(static_cast<int64_t>(v)));
                }
                case simdjson::dom::element_type::DOUBLE: {
                    double v;
                    if (el.get_double().get(v) != simdjson::SUCCESS)
                        return MakeNull();
                    return std::make_shared<JsonValue>(
                        JsonValue::FromDouble(v));
                }
                case simdjson::dom::element_type::STRING: {
                    std::string_view sv;
                    if (el.get_string().get(sv) != simdjson::SUCCESS)
                        return MakeNull();
                    return std::make_shared<JsonValue>(
                        JsonValue::FromString(std::string(sv)));
                }
                case simdjson::dom::element_type::ARRAY: {
                    auto arr =
                        std::make_shared<JsonValue>(JsonValue::FromArray());
                    for (auto item : el.get_array())
                    {
                        arr->AsArray().push_back(Convert(item));
                    }
                    return arr;
                }
                case simdjson::dom::element_type::OBJECT: {
                    auto obj =
                        std::make_shared<JsonValue>(JsonValue::FromObject());
                    for (auto kv : el.get_object())
                    {
                        std::string_view k              = kv.key;
                        obj->AsObject()[std::string(k)] = Convert(kv.value);
                    }
                    return obj;
                }
            }
            return MakeNull();
        }

        // Recursively merge @p src into @p dst, with @p src overriding
        // @p dst on key conflicts (object keys recurse; everything else
        // replaces wholesale).
        void MergeInto(
            std::shared_ptr<JsonValue> dst, std::shared_ptr<JsonValue> src)
        {
            if (!src || !dst)
                return;
            if (!src->IsObject() || !dst->IsObject())
            {
                *dst = *src;
                return;
            }
            for (auto& [k, v] : src->AsObject())
            {
                auto it = dst->AsObject().find(k);
                if (it == dst->AsObject().end())
                {
                    dst->AsObject().emplace(k, v);
                }
                else
                {
                    MergeInto(it->second, v);
                }
            }
        }

        void AppendString(std::string& out, std::string_view s)
        {
            out.push_back('"');
            for (char c : s)
            {
                switch (c)
                {
                    case '"':
                        out += "\\\"";
                        break;
                    case '\\':
                        out += "\\\\";
                        break;
                    case '\b':
                        out += "\\b";
                        break;
                    case '\f':
                        out += "\\f";
                        break;
                    case '\n':
                        out += "\\n";
                        break;
                    case '\r':
                        out += "\\r";
                        break;
                    case '\t':
                        out += "\\t";
                        break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20)
                        {
                            char buf[8];
                            std::snprintf(buf, sizeof(buf), "\\u%04x",
                                          static_cast<unsigned char>(c));
                            out += buf;
                        }
                        else
                        {
                            out.push_back(c);
                        }
                        break;
                }
            }
            out.push_back('"');
        }

        void Serialize(const std::shared_ptr<JsonValue>& v, std::string& out,
                       bool wrapRootInObject = false)
        {
            if (!v)
            {
                if (wrapRootInObject)
                    out += "{}";
                else
                    out += "null";
                return;
            }

            if (v->IsObject())
            {
                if (wrapRootInObject && v->AsObject().empty())
                {
                    out += "{}";
                    return;
                }
                out.push_back('{');
                bool first = true;
                for (auto& [k, child] : v->AsObject())
                {
                    if (!first)
                        out.push_back(',');
                    first = false;
                    AppendString(out, k);
                    out.push_back(':');
                    Serialize(child, out, false);
                }
                out.push_back('}');
            }
            else if (v->IsArray())
            {
                out.push_back('[');
                bool first = true;
                for (auto& child : v->AsArray())
                {
                    if (!first)
                        out.push_back(',');
                    first = false;
                    Serialize(child, out, false);
                }
                out.push_back(']');
            }
            else
            {
                std::visit(
                    [&out](auto&& arg) {
                        using A = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<A, std::monostate>)
                            out += "null";
                        else if constexpr (std::is_same_v<A, bool>)
                            out += arg ? "true" : "false";
                        else if constexpr (std::is_same_v<A, int64_t>)
                            out += std::to_string(arg);
                        else if constexpr (std::is_same_v<A, double>)
                        {
                            if (std::isnan(arg) || std::isinf(arg))
                                out += "null";
                            else
                            {
                                std::ostringstream oss;
                                oss << std::setprecision(17) << arg;
                                out += oss.str();
                            }
                        }
                        else if constexpr (std::is_same_v<A, std::string>)
                            AppendString(out, arg);
                    },
                    v->data);
            }
        }

        // Serialize a simdjson DOM element to a JSON string. Used by
        // GetSection() to produce a sub-tree that owns its own parser.
        std::string SerializeElement(simdjson::dom::element el)
        {
            std::string out;
            // The single-pass DOM API does not allow walking the same
            // element twice, but we only need to walk it once. We collect
            // strings into a vector so the underlying parser's tape remains
            // valid until we are done emitting.
            std::vector<std::string> stringStorage;

            std::function<void(simdjson::dom::element)> write;
            write = [&](simdjson::dom::element e) {
                switch (e.type())
                {
                    case simdjson::dom::element_type::NULL_VALUE:
                        out += "null";
                        break;
                    case simdjson::dom::element_type::BOOL: {
                        bool b;
                        if (e.get_bool().get(b) == simdjson::SUCCESS)
                            out += b ? "true" : "false";
                        else
                            out += "null";
                        break;
                    }
                    case simdjson::dom::element_type::INT64: {
                        int64_t i;
                        if (e.get_int64().get(i) == simdjson::SUCCESS)
                            out += std::to_string(i);
                        else
                            out += "null";
                        break;
                    }
                    case simdjson::dom::element_type::UINT64: {
                        uint64_t u;
                        if (e.get_uint64().get(u) == simdjson::SUCCESS)
                            out += std::to_string(u);
                        else
                            out += "null";
                        break;
                    }
                    case simdjson::dom::element_type::DOUBLE: {
                        double d;
                        if (e.get_double().get(d) == simdjson::SUCCESS)
                        {
                            if (std::isnan(d) || std::isinf(d))
                                out += "null";
                            else
                            {
                                std::ostringstream oss;
                                oss << std::setprecision(17) << d;
                                out += oss.str();
                            }
                        }
                        else
                            out += "null";
                        break;
                    }
                    case simdjson::dom::element_type::STRING: {
                        std::string_view sv;
                        if (e.get_string().get(sv) == simdjson::SUCCESS)
                            AppendString(out, sv);
                        else
                            out += "null";
                        break;
                    }
                    case simdjson::dom::element_type::ARRAY: {
                        out.push_back('[');
                        bool first = true;
                        for (auto item : e.get_array())
                        {
                            if (!first)
                                out.push_back(',');
                            first = false;
                            write(item);
                        }
                        out.push_back(']');
                        break;
                    }
                    case simdjson::dom::element_type::OBJECT: {
                        out.push_back('{');
                        bool first = true;
                        for (auto kv : e.get_object())
                        {
                            if (!first)
                                out.push_back(',');
                            first = false;
                            AppendString(out, kv.key);
                            out.push_back(':');
                            write(kv.value);
                        }
                        out.push_back('}');
                        break;
                    }
                }
            };

            write(el);
            return out;
        }

        simdjson::dom::element ParseOrThrow(simdjson::dom::parser& parser,
                                            const std::string&     src,
                                            std::string_view       what)
        {
            auto result = parser.parse(src);
            if (result.error() != simdjson::SUCCESS)
            {
                throw std::runtime_error(
                    "Skirnir: failed to parse " + std::string(what) +
                    " as JSON (" + simdjson::error_message(result.error()) +
                    ")");
            }
            return result.value();
        }

        std::string ReadFileOrThrow(const std::filesystem::path& path)
        {
            std::ifstream file(path);
            if (!file.is_open())
            {
                throw std::runtime_error(
                    "Skirnir: failed to open config file '" + path.string() +
                    "'");
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        // Build a nested object tree from a flat dotted-key map. The leaf
        // values are coerced: "true"/"false" become bool, fully-parseable
        // integers and doubles become numbers, everything else stays a
        // string. Shared by @c InMemorySource and @c EnvironmentVariablesSource.
        std::shared_ptr<JsonValue> BuildJsonTreeFromFlat(
            const std::map<std::string, std::string>& flat)
        {
            auto root = std::make_shared<JsonValue>(JsonValue::FromObject());
            for (const auto& [k, v] : flat)
            {
                std::shared_ptr<JsonValue> cursor = root;
                std::string_view           key    = k;
                while (true)
                {
                    auto dot = key.find('.');
                    if (dot == std::string_view::npos)
                    {
                        std::string leaf(key);
                        auto&       map = cursor->AsObject();
                        if (v == "true")
                            map[leaf] = std::make_shared<JsonValue>(
                                JsonValue::FromBool(true));
                        else if (v == "false")
                            map[leaf] = std::make_shared<JsonValue>(
                                JsonValue::FromBool(false));
                        else
                        {
                            try
                            {
                                size_t  pos = 0;
                                int64_t i =
                                    std::stoll(std::string(v), &pos);
                                if (pos == v.size())
                                {
                                    map[leaf] = std::make_shared<JsonValue>(
                                        JsonValue::FromInt(i));
                                    break;
                                }
                            }
                            catch (...)
                            {
                            }
                            try
                            {
                                size_t pos = 0;
                                double d =
                                    std::stod(std::string(v), &pos);
                                if (pos == v.size())
                                {
                                    map[leaf] = std::make_shared<JsonValue>(
                                        JsonValue::FromDouble(d));
                                    break;
                                }
                            }
                            catch (...)
                            {
                            }
                            map[leaf] = std::make_shared<JsonValue>(
                                JsonValue::FromString(v));
                        }
                        break;
                    }
                    std::string segment(key.substr(0, dot));
                    auto&       map = cursor->AsObject();
                    auto        it  = map.find(segment);
                    if (it == map.end())
                    {
                        auto child = std::make_shared<JsonValue>(
                            JsonValue::FromObject());
                        map[segment] = child;
                        cursor       = child;
                    }
                    else
                    {
                        if (!it->second->IsObject())
                        {
                            auto child = std::make_shared<JsonValue>(
                                JsonValue::FromObject());
                            it->second = child;
                        }
                        cursor = it->second;
                    }
                    key = key.substr(dot + 1);
                }
            }
            return root;
        }

        std::string BuildJsonFromFlat(
            const std::map<std::string, std::string>& flat)
        {
            std::string out;
            Serialize(BuildJsonTreeFromFlat(flat), out, true);
            return out;
        }

        // Portable enumeration of the process environment. Each entry is
        // the raw "NAME=VALUE" string split on the first '='. Malformed
        // entries (no '=') are skipped.
        std::vector<std::pair<std::string, std::string>> EnumerateEnv()
        {
            std::vector<std::pair<std::string, std::string>> result;
#if defined(_WIN32)
            char* block = ::GetEnvironmentStringsA();
            if (!block)
                return result;
            for (char* p = block; *p; /* advance inside loop */)
            {
                std::string_view entry(p);
                auto             eq = entry.find('=');
                if (eq != std::string_view::npos)
                {
                    result.emplace_back(std::string(entry.substr(0, eq)),
                                        std::string(entry.substr(eq + 1)));
                }
                p += entry.size() + 1; // skip NUL terminator
            }
            ::FreeEnvironmentStringsA(block);
#else
            extern char** environ;
            for (char** p = environ; p && *p; ++p)
            {
                std::string_view entry(*p);
                auto eq = entry.find('=');
                if (eq == std::string_view::npos)
                    continue;
                result.emplace_back(std::string(entry.substr(0, eq)),
                                    std::string(entry.substr(eq + 1)));
            }
#endif
            return result;
        }
    } // namespace

    // ------------------------------------------------------------------
    // Sources
    // ------------------------------------------------------------------

    JsonFileSource::JsonFileSource(std::filesystem::path path) :
        mPath(std::move(path))
    {
    }

    simdjson::dom::element JsonFileSource::Load()
    {
        if (!mLoaded)
        {
            mContent = ReadFileOrThrow(mPath);
            mElement = ParseOrThrow(
                mParser, mContent, "config file '" + mPath.string() + "'");
            mLoaded = true;
        }
        return mElement;
    }

    JsonStringSource::JsonStringSource(std::string json) :
        mContent(std::move(json))
    {
    }

    simdjson::dom::element JsonStringSource::Load()
    {
        if (!mLoaded)
        {
            mElement = ParseOrThrow(mParser, mContent, "JSON string");
            mLoaded  = true;
        }
        return mElement;
    }

    std::string InMemorySource::BuildJson() const
    {
        return BuildJsonFromFlat(mFlat);
    }

    simdjson::dom::element InMemorySource::Load()
    {
        if (mDirty)
        {
            mSerialized = BuildJson();
            mParser     = simdjson::dom::parser {};
            mElement = ParseOrThrow(mParser, mSerialized, "in-memory source");
            mDirty   = false;
        }
        return mElement;
    }

    EnvironmentVariablesSource::EnvironmentVariablesSource(
        std::string prefix) :
        mPrefix(std::move(prefix))
    {
    }

    std::string EnvironmentVariablesSource::BuildJson() const
    {
        std::map<std::string, std::string> flat;
        for (auto& [name, value] : EnumerateEnv())
        {
            if (!mPrefix.empty())
            {
                if (name.size() < mPrefix.size() ||
                    name.compare(0, mPrefix.size(), mPrefix) != 0)
                    continue;
                name.erase(0, mPrefix.size());
            }
            // "__" -> "." so that "DB__HOST" becomes "DB.HOST".
            std::string key;
            key.reserve(name.size());
            for (size_t i = 0; i < name.size(); ++i)
            {
                if (i + 1 < name.size() && name[i] == '_' &&
                    name[i + 1] == '_')
                {
                    key.push_back('.');
                    ++i;
                }
                else
                {
                    key.push_back(name[i]);
                }
            }
            flat[std::move(key)] = std::move(value);
        }
        return BuildJsonFromFlat(flat);
    }

    simdjson::dom::element EnvironmentVariablesSource::Load()
    {
        if (mDirty)
        {
            mSerialized = BuildJson();
            mParser     = simdjson::dom::parser {};
            mElement    = ParseOrThrow(
                mParser, mSerialized, "environment variables source");
            mDirty = false;
        }
        return mElement;
    }

    // ------------------------------------------------------------------
    // ConfigurationOptions
    // ------------------------------------------------------------------

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
                return static_cast<int64_t>(v);
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
        std::string slice  = SerializeElement(*el);
        auto        parser = std::make_unique<simdjson::dom::parser>();
        auto        rootEl =
            ParseOrThrow(*parser, slice, "section '" + std::string(key) + "'");
        return MakeRef<ConfigurationOptions>(std::move(parser), rootEl);
    }

    std::string ConfigurationOptions::Stringify(simdjson::dom::element el)
    {
        return SerializeElement(el);
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
                return SerializeElement(el);
        }
    }

    // ------------------------------------------------------------------
    // ConfigurationBuilder
    // ------------------------------------------------------------------

    ConfigurationBuilder& ConfigurationBuilder::AddJsonFile(
        const std::filesystem::path& path)
    {
        mSources.push_back(MakeRef<JsonFileSource>(path));
        return *this;
    }

    ConfigurationBuilder& ConfigurationBuilder::AddJsonString(
        std::string_view json)
    {
        mSources.push_back(MakeRef<JsonStringSource>(std::string(json)));
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
        auto merged = std::make_shared<JsonValue>(JsonValue::FromObject());
        for (auto& source : mSources)
        {
            simdjson::dom::element root = source->Load();
            // Non-object roots are ignored: a configuration source is
            // expected to provide a top-level object.
            if (!root.is_object())
                continue;
            auto converted = Convert(root);
            MergeInto(merged, converted);
        }

        std::string json;
        Serialize(merged, json, true);

        auto parser = std::make_unique<simdjson::dom::parser>();
        auto rootEl = ParseOrThrow(*parser, json, "merged configuration");
        return MakeRef<ConfigurationOptions>(std::move(parser), rootEl);
    }
} // namespace SKIRNIR_NAMESPACE
