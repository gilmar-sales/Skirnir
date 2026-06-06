#include "Skirnir/Configuration/JsonStringSource.hpp"

#include "Detail.hpp"

namespace SKIRNIR_NAMESPACE
{
    JsonStringSource::JsonStringSource(std::string json) :
        mContent(std::move(json))
    {
    }

    simdjson::dom::element JsonStringSource::Load()
    {
        if (!mLoaded)
        {
            mElement = detail::ParseOrThrow(mParser, mContent, "JSON string");
            mLoaded  = true;
        }
        return mElement;
    }
} // namespace SKIRNIR_NAMESPACE
