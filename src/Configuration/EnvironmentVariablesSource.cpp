#include "Skirnir/Configuration/EnvironmentVariablesSource.hpp"

#include <map>
#include <utility>

#include "Detail.hpp"

namespace SKIRNIR_NAMESPACE
{
    EnvironmentVariablesSource::EnvironmentVariablesSource(
        std::string prefix) :
        mPrefix(std::move(prefix))
    {
    }

    std::string EnvironmentVariablesSource::BuildJson() const
    {
        std::map<std::string, std::string> flat;
        for (auto& [name, value] : detail::EnumerateEnv(mPrefix))
        {
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
        return detail::BuildJsonFromFlat(flat);
    }

    simdjson::dom::element EnvironmentVariablesSource::Load()
    {
        if (mDirty)
        {
            mSerialized = BuildJson();
            mParser     = simdjson::dom::parser {};
            mElement    = detail::ParseOrThrow(
                mParser, mSerialized, "environment variables source");
            mDirty      = false;
        }
        return mElement;
    }
} // namespace SKIRNIR_NAMESPACE
