#include "Detail.hpp"
#include "Skirnir/Logging/LogRecord.hpp"
#include "Skirnir/Logging/LogSinks/FileSink.hpp"

#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace SKIRNIR_NAMESPACE
{
    FileSink::FileSink(std::filesystem::path path, bool autoFlush) :
        mPath(std::move(path)), mAutoFlush(autoFlush)
    {
        OpenLocked();
    }

    FileSink::FileSink(std::filesystem::path path,
                       FileSinkOptions         options,
                       bool                    autoFlush) :
        mPath(std::move(path)),
        mOptions([&] {
            FileSinkOptions o = options;
            if (o.maxBytes > 0 && o.maxFiles == 0)
                o.maxFiles = 1; // sane default
            return o;
        }()),
        mAutoFlush(autoFlush)
    {
        if (mOptions.rotateOnOpen && mOptions.maxBytes > 0)
        {
            std::error_code ec;
            std::filesystem::remove(mPath, ec);
            // Best-effort: any pre-existing rotated copies are kept.
        }
        OpenLocked();
    }

    FileSink::~FileSink()
    {
        if (mFile)
        {
            std::fclose(mFile);
            mFile = nullptr;
        }
    }

    void FileSink::OpenLocked()
    {
        // mMutex is intentionally not held here: the constructor runs
        // before any other thread can see the object, and Write() takes
        // mMutex before touching mFile.
        auto opened = detail::OpenSecureLogFile(mPath);
        mFile        = opened.file;
        mCurrentSize = opened.currentSize;
    }

    void FileSink::Write(const LogRecord& r)
    {
        std::ostringstream oss;
        oss << '[' << detail::LevelName(r.level) << "] " << r.timestamp << " '"
            << detail::SanitizeForLog(r.category) << "': ";
        if (!r.scopes.empty())
        {
            oss << '[';
            for (std::size_t i = 0; i < r.scopes.size(); ++i)
            {
                if (i)
                    oss << '/';
                oss << detail::SanitizeForLog(r.scopes[i]);
            }
            oss << "] ";
        }
        oss << detail::SanitizeForLog(r.message) << '\n';
        std::string line = oss.str();

        std::lock_guard<std::mutex> lock(mMutex);
        if (!mFile)
            return;

        if (mOptions.maxBytes > 0 &&
            mCurrentSize + line.size() > mOptions.maxBytes)
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
                // Stay closed; subsequent Write() calls will be a no-op
                // until the sink is replaced.
            }
        }

        if (mFile)
        {
            std::fwrite(line.data(), 1, line.size(), mFile);
            mCurrentSize += line.size();
            if (mAutoFlush)
                std::fflush(mFile);
        }
    }

    void FileSink::Flush()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mFile)
            std::fflush(mFile);
    }
} // namespace SKIRNIR_NAMESPACE
