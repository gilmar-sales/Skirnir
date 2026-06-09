#pragma once

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Configuration.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <locale>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
#endif

namespace SKIRNIR_NAMESPACE::detail
{
    extern "C" char** environ;

    // Intermediate merge representation; built from simdjson elements
    // and serialized back to a JSON string for the final parse.
    struct JsonValue;

    using JsonObject = std::map<std::string, Arc<JsonValue>>;
    using JsonArray  = std::vector<Arc<JsonValue>>;

    struct JsonValue
    {
        std::variant<std::monostate, bool, int64_t, double, std::string,
                     Arc<JsonObject>,
                     Arc<JsonArray>>
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
            return JsonValue { skr::MakeArc<JsonObject>() };
        }

        static JsonValue FromArray()
        {
            return JsonValue { skr::MakeArc<JsonArray>() };
        }

        bool IsObject() const
        {
            return std::holds_alternative<Arc<JsonObject>>(
                data);
        }
        bool IsArray() const
        {
            return std::holds_alternative<Arc<JsonArray>>(data);
        }
        JsonObject& AsObject()
        {
            return *std::get<Arc<JsonObject>>(data);
        }
        const JsonObject& AsObject() const
        {
            return *std::get<Arc<JsonObject>>(data);
        }
        JsonArray& AsArray()
        {
            return *std::get<Arc<JsonArray>>(data);
        }
        const JsonArray& AsArray() const
        {
            return *std::get<Arc<JsonArray>>(data);
        }
    };

    inline Arc<JsonValue> MakeNull()
    {
        return skr::MakeArc<JsonValue>(JsonValue::Null());
    }

    inline Arc<JsonValue> Convert(simdjson::dom::element el)
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
                return skr::MakeArc<JsonValue>(JsonValue::FromBool(v));
            }
            case simdjson::dom::element_type::INT64: {
                int64_t v;
                if (el.get_int64().get(v) != simdjson::SUCCESS)
                    return MakeNull();
                return skr::MakeArc<JsonValue>(JsonValue::FromInt(v));
            }
            case simdjson::dom::element_type::UINT64: {
                uint64_t v;
                if (el.get_uint64().get(v) != simdjson::SUCCESS)
                    return MakeNull();
                return skr::MakeArc<JsonValue>(
                    JsonValue::FromInt(static_cast<int64_t>(v)));
            }
            case simdjson::dom::element_type::DOUBLE: {
                double v;
                if (el.get_double().get(v) != simdjson::SUCCESS)
                    return MakeNull();
                return skr::MakeArc<JsonValue>(
                    JsonValue::FromDouble(v));
            }
            case simdjson::dom::element_type::STRING: {
                std::string_view sv;
                if (el.get_string().get(sv) != simdjson::SUCCESS)
                    return MakeNull();
                return skr::MakeArc<JsonValue>(
                    JsonValue::FromString(std::string(sv)));
            }
            case simdjson::dom::element_type::ARRAY: {
                auto arr =
                    skr::MakeArc<JsonValue>(JsonValue::FromArray());
                for (auto item : el.get_array())
                {
                    arr->AsArray().push_back(Convert(item));
                }
                return arr;
            }
            case simdjson::dom::element_type::OBJECT: {
                auto obj =
                    skr::MakeArc<JsonValue>(JsonValue::FromObject());
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
    inline void MergeInto(
        Arc<JsonValue> dst, Arc<JsonValue> src)
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

    inline void AppendString(std::string& out, std::string_view s)
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

    inline void Serialize(const Arc<JsonValue>& v,
                          std::string&                       out,
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
    inline std::string SerializeElement(simdjson::dom::element el)
    {
        std::string out;
        // The single-pass DOM API does not allow walking the same
        // element twice, so we accumulate the output into a single
        // string in a single pass. The caller must keep the backing
        // parser alive until the returned string is no longer needed.

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

    inline std::string SanitizeForError(std::string_view s,
                                        std::size_t       maxLen = 256)
    {
        std::string out;
        out.reserve(std::min(s.size(), maxLen) + 8);
        std::size_t i = 0;
        for (; i < s.size() && out.size() < maxLen; ++i)
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c < 0x20 || c == 0x7f)
            {
                if (out.size() + 6 > maxLen)
                    break;
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                out.append(buf);
            }
            else
            {
                out.push_back(static_cast<char>(c));
            }
        }
        if (i < s.size())
            out.append("...");
        return out;
    }

    inline simdjson::dom::element ParseOrThrow(simdjson::dom::parser& parser,
                                               const std::string&     src,
                                               std::string_view       what)
    {
        auto result = parser.parse(src);
        if (result.error() != simdjson::SUCCESS)
        {
            throw std::runtime_error(
                "Skirnir: failed to parse " + SanitizeForError(what) +
                " as JSON (" + simdjson::error_message(result.error()) +
                ")");
        }
        return result.value();
    }

    inline std::string ReadFileOrThrow(const std::filesystem::path& path,
                                      std::int64_t                  maxSize)
    {
        const std::string safePath = SanitizeForError(path.string());
        if (maxSize >= 0)
        {
            std::error_code ec;
            auto status = std::filesystem::status(path, ec);
            if (!ec && !std::filesystem::is_regular_file(status))
            {
                throw std::runtime_error(
                    "Skirnir: config file '" + safePath +
                    "' is not a regular file");
            }
            auto size = std::filesystem::file_size(path, ec);
            if (!ec && size > static_cast<std::uintmax_t>(maxSize))
            {
                throw std::runtime_error(
                    "Skirnir: config file '" + safePath +
                    "' exceeds maximum allowed size");
            }
        }
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error(
                "Skirnir: failed to open config file '" + safePath + "'");
        }
        if (maxSize < 0)
        {
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }

        constexpr std::size_t kChunk = 64 * 1024;
        std::string          buffer;
        buffer.reserve(kChunk);
        char                 chunk[kChunk];
        while (file)
        {
            file.read(chunk, static_cast<std::streamsize>(kChunk));
            auto got = static_cast<std::size_t>(file.gcount());
            if (got == 0)
                break;
            if (buffer.size() + got >
                static_cast<std::size_t>(maxSize) + 1)
            {
                throw std::runtime_error(
                    "Skirnir: config file '" + safePath +
                    "' exceeds maximum allowed size");
            }
            buffer.append(chunk, got);
        }
        return buffer;
    }

    // Build a nested object tree from a flat dotted-key map. The leaf
    // values are coerced: "true"/"false" become bool, fully-parseable
    // integers and doubles become numbers, everything else stays a
    // string. Shared by @c InMemorySource and @c
    // EnvironmentVariablesSource.
    inline Arc<JsonValue> BuildJsonTreeFromFlat(
        const std::map<std::string, std::string>& flat)
    {
        auto makeVal = [](JsonValue v) -> Arc<JsonValue> {
            void* mem = ::operator new(
                sizeof(skr::detail::ArcControlBlock) + sizeof(JsonValue));
            auto* cb = ::new (mem) skr::detail::ArcControlBlock();
            auto* obj = ::new (
                reinterpret_cast<char*>(mem) +
                sizeof(skr::detail::ArcControlBlock))
                JsonValue(std::move(v));
            cb->payload = obj;
            cb->dispose =
                &skr::detail::arc_dispose_destroy_in_place<JsonValue>;
            cb->destroy =
                &skr::detail::arc_aligned_destroy<JsonValue>;
            cb->strong.store(1, std::memory_order_relaxed);
            cb->weak.store(1, std::memory_order_relaxed);
            return Arc<JsonValue>(obj, cb);
        };
        auto root = makeVal(JsonValue::FromObject());
        for (const auto& [k, v] : flat)
        {
            Arc<JsonValue> cursor = root;
            std::string_view           key    = k;
            while (true)
            {
                auto dot = key.find('.');
                if (dot == std::string_view::npos)
                {
                    std::string leaf(key);
                    auto&       map = cursor->AsObject();
                    if (v == "true")
                        map[leaf] = makeVal(
                            JsonValue::FromBool(true));
                    else if (v == "false")
                        map[leaf] = makeVal(
                            JsonValue::FromBool(false));
                    else
                    {
                        try
                        {
                            size_t  pos = 0;
                            int64_t i   = std::stoll(v, &pos);
                            if (pos == v.size())
                            {
                                map[leaf] = makeVal(
                                    JsonValue::FromInt(i));
                                break;
                            }
                        }
                        catch (...)
                        {
                        }
                        try
                        {
                            const char* begin = v.c_str();
                            char*       end   = nullptr;
                            double      d     = ::strtod(begin, &end);
                            if (end != begin && end == begin + v.size())
                            {
                                map[leaf] = makeVal(
                                    JsonValue::FromDouble(d));
                                break;
                            }
                        }
                        catch (...)
                        {
                        }
                        map[leaf] = makeVal(
                            JsonValue::FromString(v));
                    }
                    break;
                }

                if (dot == 0)
                {
                    key = key.substr(1);
                    continue;
                }

                std::string segment(key.substr(0, dot));
                auto&       map = cursor->AsObject();
                auto        it  = map.find(segment);
                if (it == map.end())
                {
                    auto child = makeVal(
                        JsonValue::FromObject());
                    map[segment] = child;
                    cursor       = child;
                }
                else
                {
                    if (!it->second->IsObject())
                    {
                        auto child = makeVal(
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

    inline std::string BuildJsonFromFlat(
        const std::map<std::string, std::string>& flat)
    {
        std::string out;
        Serialize(BuildJsonTreeFromFlat(flat), out, true);
        return out;
    }

    inline std::vector<std::pair<std::string, std::string>> EnumerateEnv(
        std::string_view prefix)
    {
        std::vector<std::pair<std::string, std::string>> result;
        auto prependPrefixMatch = [&](std::string_view name) -> bool {
            if (prefix.empty())
                return true;
            return name.size() >= prefix.size() &&
                   name.compare(0, prefix.size(), prefix) == 0;
        };
#if defined(_WIN32)
        char* block = ::GetEnvironmentStringsA();
        if (!block)
            return result;
        for (char* p = block; *p; /* advance inside loop */)
        {
            std::string_view entry(p);
            auto             eq = entry.find('=');

            if (eq != std::string_view::npos && eq > 0 &&
                prependPrefixMatch(entry.substr(0, eq)))
            {
                std::string_view name = entry.substr(0, eq);
                if (!prefix.empty())
                    name.remove_prefix(prefix.size());
                result.emplace_back(std::string(name),
                                    std::string(entry.substr(eq + 1)));
            }
            p += entry.size() + 1; // skip NUL terminator
        }
        ::FreeEnvironmentStringsA(block);
#else
        for (char** p = environ; p && *p; ++p)
        {
            std::string_view entry(*p);
            auto             eq = entry.find('=');
            if (eq == std::string_view::npos)
                continue;
            if (!prependPrefixMatch(entry.substr(0, eq)))
                continue;
            std::string_view name = entry.substr(0, eq);
            if (!prefix.empty())
                name.remove_prefix(prefix.size());
            result.emplace_back(std::string(name),
                                std::string(entry.substr(eq + 1)));
        }
#endif
        return result;
    }
} // namespace SKIRNIR_NAMESPACE::detail
