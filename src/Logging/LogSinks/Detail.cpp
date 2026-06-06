#include "Detail.hpp"

#include "Skirnir/Logging/LogLevel.hpp"

#include <cstring>
#include <format>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#  include <fcntl.h>
#  include <io.h>
#  include <sys/stat.h>
#  include <windows.h>
#else
#  include <cerrno>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace SKIRNIR_NAMESPACE::detail
{
    const char* LevelName(LogLevel lvl)
    {
        switch (lvl)
        {
            case LogLevel::Debug:
                return "Debug";
            case LogLevel::Trace:
                return "Trace";
            case LogLevel::Information:
                return "Information";
            case LogLevel::Warning:
                return "Warning";
            case LogLevel::Error:
                return "Error";
            case LogLevel::Fatal:
                return "Fatal";
            case LogLevel::None:
                return "None";
        }
        return "?";
    }

    std::string SanitizeForLog(std::string_view s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            switch (c)
            {
                case '\r':
                    out.append("\\r");
                    break;
                case '\n':
                    out.append("\\n");
                    break;
                case '\t':
                    out.append("\\t");
                    break;
                case '\0':
                    out.append("\\0");
                    break;
                case '\b':
                    out.append("\\b");
                    break;
                case '\f':
                    out.append("\\f");
                    break;
                case '\v':
                    out.append("\\v");
                    break;
                case '\x1b':
                    out.append("\\x1b");
                    break;
                default:
                {
                    const auto u = static_cast<unsigned char>(c);
                    if (u < 0x20 || u == 0x7f)
                    {
                        out.append(std::format("\\x{:02x}", u));
                    }
                    else
                    {
                        out.push_back(c);
                    }
                    break;
                }
            }
        }
        return out;
    }

    LogFileOpenResult OpenSecureLogFile(const std::filesystem::path& path)
    {
        LogFileOpenResult r;
#ifdef _WIN32
        // Pre-flight: refuse to open a path that already exists as a
        // reparse point (symlink, junction). This costs a small TOCTOU
        // window on platforms without FILE_FLAG_OPEN_NO_REPARSE_POINT;
        // the post-open check below closes it for the common case.
        {
            const DWORD attrs = ::GetFileAttributesW(path.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES &&
                (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            {
                throw std::runtime_error(
                    "Skirnir: log path is a reparse point: '" +
                    path.string() + "'");
            }
        }
        HANDLE h = ::CreateFileW(
            path.c_str(),
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (h == INVALID_HANDLE_VALUE)
        {
            throw std::runtime_error(
                "Skirnir: failed to open log file '" + path.string() +
                "': " + std::to_string(::GetLastError()));
        }
        BY_HANDLE_FILE_INFORMATION info{};
        if (!::GetFileInformationByHandle(h, &info))
        {
            ::CloseHandle(h);
            throw std::runtime_error(
                "Skirnir: cannot stat log file '" + path.string() + "'");
        }
        const DWORD attrs = info.dwFileAttributes;
        if ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0 ||
            (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0 ||
            (attrs & FILE_ATTRIBUTE_DEVICE) != 0)
        {
            ::CloseHandle(h);
            throw std::runtime_error(
                "Skirnir: log path is not a regular file: '" + path.string() +
                "'");
        }
        const int fd = ::_open_osfhandle(reinterpret_cast<intptr_t>(h),
                                         _O_APPEND | _O_BINARY);
        if (fd < 0)
        {
            ::CloseHandle(h);
            throw std::runtime_error(
                "Skirnir: _open_osfhandle failed for '" + path.string() + "'");
        }
        r.file = ::_fdopen(fd, "ab");
        if (!r.file)
        {
            ::_close(fd);
            throw std::runtime_error(
                "Skirnir: _fdopen failed for '" + path.string() + "'");
        }
        LARGE_INTEGER size {};
        if (::GetFileSizeEx(h, &size))
        {
            r.currentSize = static_cast<std::size_t>(size.QuadPart);
        }
#else
        int flags = O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC;
#  ifdef O_NOFOLLOW
        flags |= O_NOFOLLOW;
#  endif
        const int fd = ::open(path.c_str(), flags, 0640);
        if (fd < 0)
        {
            throw std::runtime_error(
                "Skirnir: failed to open log file '" + path.string() +
                "': " + std::strerror(errno));
        }
        struct stat st {};
        if (::fstat(fd, &st) != 0)
        {
            ::close(fd);
            throw std::runtime_error(
                "Skirnir: cannot stat log file '" + path.string() + "'");
        }
        if (!S_ISREG(st.st_mode))
        {
            ::close(fd);
            throw std::runtime_error(
                "Skirnir: log path is not a regular file: '" + path.string() +
                "'");
        }
        r.file = ::fdopen(fd, "a");
        if (!r.file)
        {
            ::close(fd);
            throw std::runtime_error(
                "Skirnir: fdopen failed for '" + path.string() + "'");
        }
        r.currentSize = static_cast<std::size_t>(st.st_size);
#endif
        return r;
    }

    void RotateLogFile(const std::filesystem::path& path,
                       std::size_t                  maxFiles)
    {
        if (maxFiles == 0)
            return;

        namespace fs = std::filesystem;
        std::error_code ec;

        // Drop the oldest slot, if any.
        fs::path oldest = path;
        oldest += ".";
        oldest += std::to_string(maxFiles);
        fs::remove(oldest, ec);
        ec.clear();

        // Shift older rotations up by one slot: path.(i-1) -> path.i
        for (std::size_t i = maxFiles; i > 1; --i)
        {
            fs::path from = path;
            from += ".";
            from += std::to_string(i - 1);
            fs::path to = path;
            to += ".";
            to += std::to_string(i);
            if (fs::exists(from, ec))
            {
                ec.clear();
                fs::rename(from, to, ec);
            }
            ec.clear();
        }

        // Move current log to .1.
        fs::path first = path;
        first += ".1";
        fs::rename(path, first, ec);
    }
} // namespace SKIRNIR_NAMESPACE::detail
