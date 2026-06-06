#include "Detail.hpp"
#include "Skirnir/Logging/LogRecord.hpp"
#include "Skirnir/Logging/LogSinks/JsonSink.hpp"

#define SIMDJSON_STATIC_REFLECTION 1
#include <simdjson.h>

#include <cstring>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace SKIRNIR_NAMESPACE
{
    JsonSink::JsonSink(std::ostream& os) : mOs(&os)
    {
    }

    JsonSink::JsonSink(std::filesystem::path path) :
        mPath(std::move(path))
    {
        auto opened = detail::OpenSecureLogFile(mPath);
        mFile        = opened.file;
        mCurrentSize = opened.currentSize;
    }

    JsonSink::JsonSink(std::filesystem::path path, FileSinkOptions options) :
        mPath(std::move(path)),
        mOptions([&] {
            FileSinkOptions o = options;
            if (o.maxBytes > 0 && o.maxFiles == 0)
                o.maxFiles = 1;
            return o;
        }())
    {
        if (mOptions.rotateOnOpen && mOptions.maxBytes > 0)
        {
            std::error_code ec;
            std::filesystem::remove(mPath, ec);
        }
        auto opened = detail::OpenSecureLogFile(mPath);
        mFile        = opened.file;
        mCurrentSize = opened.currentSize;
    }

    JsonSink::~JsonSink()
    {
        if (mFile)
        {
            std::fclose(mFile);
            mFile = nullptr;
        }
    }

    void JsonSink::Write(const LogRecord& r)
    {
        auto json = simdjson::to_json(r);

        if (!json.has_value())
            return;

        const std::string_view line = json.value();
        // simdjson::to_json does not include the trailing newline that
        // NDJSON expects; append it here.
        const std::size_t      totalSize = line.size() + 1;

        std::lock_guard<std::mutex> lock(mMutex);

        if (mFile)
        {
            if (mOptions.maxBytes > 0 &&
                mCurrentSize + totalSize > mOptions.maxBytes)
            {
                std::fclose(mFile);
                mFile = nullptr;
                detail::RotateLogFile(mPath, mOptions.maxFiles);
                try
                {
                    auto reopened = detail::OpenSecureLogFile(mPath);
                    mFile        = reopened.file;
                    mCurrentSize = reopened.currentSize;
                }
                catch (...)
                {
                }
            }

            if (mFile)
            {
                if (line.size() > 0)
                    std::fwrite(line.data(), 1, line.size(), mFile);
                std::fputc('\n', mFile);
                mCurrentSize += totalSize;
            }
        }
        else if (mOs)
        {
            (*mOs) << line << '\n';
        }
    }

    void JsonSink::Flush()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mFile)
            std::fflush(mFile);
        else if (mOs)
            mOs->flush();
    }
} // namespace SKIRNIR_NAMESPACE
