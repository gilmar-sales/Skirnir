#include "Skirnir/Configuration.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace SKIRNIR_NAMESPACE
{
    bool JsonConfigurationOptions::LoadFromFile(
        const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            return false;
        }

        std::ifstream file(path);
        if (!file.is_open())
        {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        mJsonContent = buffer.str();

        return !mJsonContent.empty();
    }

    bool JsonConfigurationOptions::LoadFromString(std::string_view json)
    {
        mJsonContent = std::string(json);
        return !mJsonContent.empty();
    }

    std::optional<std::string> JsonConfigurationOptions::GetValue(
        std::string_view key) const
    {
        return FindValueInJson(mJsonContent, key);
    }

    bool JsonConfigurationOptions::HasKey(std::string_view key) const
    {
        return FindValueInJson(mJsonContent, key).has_value();
    }

    std::optional<std::string> JsonConfigurationOptions::FindValueInJson(
        std::string_view json, std::string_view key)
    {
        // Parse dot-separated key path
        size_t dotPos = key.find('.');

        std::string_view currentKey = (dotPos == std::string_view::npos)
                                          ? key
                                          : key.substr(0, dotPos);
        std::string_view remainingKey =
            (dotPos == std::string_view::npos) ? std::string_view()
                                               : key.substr(dotPos + 1);

        // Find the key in JSON - look for "key":
        std::string searchPattern =
            "\"" + std::string(currentKey) + "\""; // NOLINT
        size_t keyPos = json.find(searchPattern);

        if (keyPos == std::string_view::npos)
        {
            return std::nullopt;
        }

        // Find the colon after the key
        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string_view::npos)
        {
            return std::nullopt;
        }

        // Skip whitespace and find the value
        size_t valueStart = colonPos + 1;
        while (valueStart < json.size() &&
               (json[valueStart] == ' ' || json[valueStart] == '\t' ||
                json[valueStart] == '\n' || json[valueStart] == '\r'))
        {
            valueStart++;
        }

        if (valueStart >= json.size())
        {
            return std::nullopt;
        }

        // Handle string values
        if (json[valueStart] == '"')
        {
            return ExtractStringValue(json, valueStart + 1);
        }

        // Handle non-string values (numbers, booleans, null, objects, arrays)
        size_t valueEnd = valueStart;
        int braceCount = 0;
        int bracketCount = 0;
        bool inString = false;

        while (valueEnd < json.size())
        {
            char c = json[valueEnd];

            if (c == '"' && (valueEnd == 0 || json[valueEnd - 1] != '\\'))
            {
                inString = !inString;
            }
            else if (!inString)
            {
                if (c == '{')
                {
                    braceCount++;
                }
                else if (c == '}')
                {
                    if (braceCount == 0)
                    {
                        // Reached end of object, don't include }
                        break;
                    }
                    braceCount--;
                }
                else if (c == '[')
                {
                    bracketCount++;
                }
                else if (c == ']')
                {
                    if (bracketCount == 0)
                    {
                        // End of array, don't include ]
                        break;
                    }
                    bracketCount--;
                }
                else if ((c == ',' || c == ' ' || c == '\t' || c == '\n' ||
                          c == '\r') &&
                         braceCount == 0 && bracketCount == 0)
                {
                    break;
                }
                else if (c == '}' && braceCount == 0 && bracketCount == 0)
                {
                    // End of object reached
                    break;
                }
            }

            valueEnd++;
        }

        std::string_view value = json.substr(valueStart, valueEnd - valueStart);
        value = TrimWhitespace(value);

        // Handle trailing closing brace that wasn't part of the value
        if (!value.empty() && value.back() == '}')
        {
            value = value.substr(0, value.size() - 1);
            value = TrimWhitespace(value);
        }

        // If there's a remaining key and we have an object, recurse
        if (!remainingKey.empty() && !remainingKey.ends_with('.') &&
            !value.empty() && value.front() == '{')
        {
            // Find the opening brace
            size_t objStart = json.find('{', valueStart);
            if (objStart != std::string_view::npos)
            {
                // Find matching closing brace
                int depth = 1;
                size_t objEnd = objStart + 1;
                while (objEnd < json.size() && depth > 0)
                {
                    if (json[objEnd] == '{')
                        depth++;
                    else if (json[objEnd] == '}')
                        depth--;
                    objEnd++;
                }

                std::string_view subObject =
                    json.substr(objStart, objEnd - objStart);
                return FindValueInJson(subObject, remainingKey);
            }
        }

        // Handle boolean values
        if (value == "true")
            return std::string("true");
        if (value == "false")
            return std::string("false");

        // Handle null
        if (value == "null")
            return std::nullopt;

        // Return the value as string
        return std::string(value);
    }

    std::string_view JsonConfigurationOptions::TrimWhitespace(
        std::string_view s)
    {
        while (!s.empty() &&
               (s.front() == ' ' || s.front() == '\t' || s.front() == '\n' ||
                s.front() == '\r'))
        {
            s = s.substr(1);
        }

        while (!s.empty() &&
               (s.back() == ' ' || s.back() == '\t' || s.back() == '\n' ||
                s.back() == '\r'))
        {
            s = s.substr(0, s.size() - 1);
        }

        return s;
    }

    std::string JsonConfigurationOptions::ExtractStringValue(
        std::string_view json, size_t startPos)
    {
        std::string result;
        bool        escaped = false;

        for (size_t i = startPos; i < json.size(); i++)
        {
            char c = json[i];

            if (escaped)
            {
                switch (c)
                {
                    case 'n':
                        result += '\n';
                        break;
                    case 't':
                        result += '\t';
                        break;
                    case 'r':
                        result += '\r';
                        break;
                    case '\\':
                        result += '\\';
                        break;
                    case '"':
                        result += '"';
                        break;
                    default:
                        result += c;
                        break;
                }
                escaped = false;
            }
            else if (c == '\\')
            {
                escaped = true;
            }
            else if (c == '"')
            {
                break;
            }
            else
            {
                result += c;
            }
        }

        return result;
    }

    ConfigurationBuilder& ConfigurationBuilder::AddJsonFile(
        const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            mJsonContent = buffer.str();
        }
        return *this;
    }

    ConfigurationBuilder& ConfigurationBuilder::AddJsonString(
        std::string_view json)
    {
        mJsonContent = std::string(json);
        return *this;
    }

    Ref<ConfigurationOptions> ConfigurationBuilder::Build()
    {
        auto config = MakeRef<JsonConfigurationOptions>();
        config->LoadFromString(mJsonContent);
        return config;
    }
} // namespace SKIRNIR_NAMESPACE