#include "Skirnir/Configuration/JsonFileSource.hpp"

#include "Detail.hpp"

namespace SKIRNIR_NAMESPACE
{
    JsonFileSource::JsonFileSource(std::filesystem::path path) :
        mPath(std::move(path))
    {
    }

    simdjson::dom::element JsonFileSource::Load()
    {
        if (!mLoaded)
        {
            mContent = detail::ReadFileOrThrow(mPath);
            mElement = detail::ParseOrThrow(
                mParser, mContent, "config file '" + mPath.string() + "'");
            mLoaded  = true;
        }
        return mElement;
    }
} // namespace SKIRNIR_NAMESPACE
