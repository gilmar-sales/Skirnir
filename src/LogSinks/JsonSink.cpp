#include "Skirnir/LogSinks/JsonSink.hpp"

#include "Skirnir/LogRecord.hpp"
#include "Detail.hpp"

#define SIMDJSON_STATIC_REFLECTION 1
#include <simdjson.h>

#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace SKIRNIR_NAMESPACE
{
    JsonSink::JsonSink(std::ostream& os) : mOs(os)
    {
    }

    JsonSink::JsonSink(std::filesystem::path path) :
        mOwned(std::make_unique<std::ofstream>(path)), mOs(*mOwned)
    {
        if (!mOwned->is_open())
            throw std::runtime_error(
                "Skirnir: JsonSink failed to open output file");
    }

    void JsonSink::Write(const LogRecord& r)
    {
        auto json = simdjson::to_json(r);

        if (json.has_value())
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mOs << json.value() << '\n';
        }
    }

    void JsonSink::Flush()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mOs.flush();
    }
} // namespace SKIRNIR_NAMESPACE
