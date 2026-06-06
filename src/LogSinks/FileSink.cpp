#include "Skirnir/LogSinks/FileSink.hpp"

#include "Skirnir/LogRecord.hpp"
#include "Detail.hpp"

#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace SKIRNIR_NAMESPACE
{
    FileSink::FileSink(std::filesystem::path path, bool autoFlush) :
        mPath(std::move(path)), mAutoFlush(autoFlush)
    {
#ifdef _WIN32
        std::wstring wpath = mPath.wstring();
        mFile              = _wfopen(wpath.c_str(), L"a");
#else
        mFile = std::fopen(mPath.string().c_str(), "a");
#endif
        if (!mFile)
        {
            throw std::runtime_error(
                "Skirnir: FileSink failed to open '" + mPath.string() + "'");
        }
    }

    FileSink::~FileSink()
    {
        if (mFile)
        {
            std::fclose(mFile);
            mFile = nullptr;
        }
    }

    void FileSink::Write(const LogRecord& r)
    {
        std::ostringstream oss;
        oss << '[' << detail::LevelName(r.level) << "] " << r.timestamp << " '"
            << r.category << "': ";
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
        if (mFile)
        {
            std::fwrite(line.data(), 1, line.size(), mFile);
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
