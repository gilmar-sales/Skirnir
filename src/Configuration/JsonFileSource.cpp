#include "Skirnir/Configuration/JsonFileSource.hpp"

#include "Detail.hpp"

namespace SKIRNIR_NAMESPACE
{
    JsonFileSource::JsonFileSource(std::filesystem::path path,
                                   std::int64_t          maxSize) :
        mPath(std::move(path)),
        mMaxSize(maxSize)
    {
    }

    simdjson::dom::element JsonFileSource::Load()
    {
        if (!mLoaded)
        {
            mContent = detail::ReadFileOrThrow(mPath, mMaxSize);
            mElement = detail::ParseOrThrow(
                mParser, mContent, "config file '" + mPath.string() + "'");
            mLoaded  = true;
        }
        return mElement;
    }
} // namespace SKIRNIR_NAMESPACE
