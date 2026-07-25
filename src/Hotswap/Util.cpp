#include <algorithm>
#include <array>
#include <unordered_set>

#include "Skirnir/Hotswap/Util.hpp"
#include "Skirnir/Hotswap/FsPathHasher.hpp"
#include "Skirnir/Hotswap/Platform.hpp"

namespace skr::hotswap::util
{
    namespace
    {
        const std::unordered_set<std::string> HEADER_EXTENSIONS = {
            ".h", ".hh", ".hpp",
        };

        const std::unordered_set<std::string> SOURCE_EXTENSIONS = {
            ".cpp", ".c", ".cc", ".cxx",
        };
    } // namespace

    bool IsWhitespace(const std::string& str)
    {
        return std::all_of(str.begin(), str.end(), ::isspace);
    }

    std::string Trim(const std::string& str)
    {
        std::string whitespace = " \t\n\v\f\r";
        std::size_t iFirst = str.find_first_not_of(whitespace);
        std::size_t iLast = str.find_last_not_of(whitespace);

        if (iFirst != std::string::npos && iLast != std::string::npos)
        {
            return str.substr(iFirst, iLast - iFirst + 1);
        }
        return "";
    }

    std::string Quote(const std::string& str)
    {
        return "\"" + str + "\"";
    }

    std::string UnixSlashes(const std::string& str)
    {
        std::string replaced = str;
        std::replace(replaced.begin(), replaced.end(), '\\', '/');
        return replaced;
    }

    namespace
    {
        std::size_t SkipWhitespace(const std::string& str, std::size_t i)
        {
            while (i < str.size() && std::isspace(static_cast<unsigned char>(str[i])))
            {
                ++i;
            }
            return i;
        }

        std::size_t SkipToken(const std::string& str, std::size_t i)
        {
            bool inQuotes = false;
            while (i < str.size())
            {
                char c = str[i];
                if (c == '"')
                {
                    inQuotes = !inQuotes;
                    ++i;
                    continue;
                }
                if (!inQuotes && std::isspace(static_cast<unsigned char>(c)))
                {
                    break;
                }
                ++i;
            }
            return i;
        }
    } // namespace

    std::string RemoveArg(const std::string& command,
                      const std::string& arg,
                      bool                takesValue)
    {
        std::string result;
        result.reserve(command.size());

        std::size_t i = 0;
        while (i < command.size())
        {
            std::size_t tokenStart = i;
            i                       = SkipWhitespace(command, i);
            std::size_t gap         = i - tokenStart;
            tokenStart              = i;
            std::size_t tokenEnd    = SkipToken(command, i);
            std::string token       = command.substr(tokenStart, tokenEnd - tokenStart);

            bool dropValue = false;
            if (token == arg)
            {
                dropValue = takesValue;
            }
            else if (token.size() > arg.size() &&
                     token.compare(0, arg.size(), arg) == 0 &&
                     token[arg.size()] == '=')
            {
                // `-o=value` form: token itself is consumed.
            }
            else
            {
                result.append(command, tokenStart - gap, gap + tokenEnd - tokenStart);
            }

            i = tokenEnd;
            if (dropValue)
            {
                i = SkipWhitespace(command, i);
                i = SkipToken(command, i);
            }
        }

        return Trim(result);
    }

    std::string QuoteArgs(const std::string& command)
    {
        std::string result;
        result.reserve(command.size() + 16);

        std::size_t i = 0;
        while (i < command.size())
        {
            std::size_t gap = SkipWhitespace(command, i) - i;
            result.append(command, i, gap);
            i += gap;

            if (i >= command.size())
            {
                break;
            }

            std::size_t tokenStart = i;
            std::size_t tokenEnd    = SkipToken(command, i);
            std::string token       = command.substr(tokenStart, tokenEnd - tokenStart);

            auto alreadyQuoted = [](const std::string& t) {
                return t.size() >= 2 && t.front() == '"' && t.back() == '"';
            };

            if (!alreadyQuoted(token))
            {
                // Flag token: emit as-is (with optional `=value` quoted).
                if (!token.empty() && token[0] == '-')
                {
                    auto eqPos = token.find('=');
                    if (eqPos != std::string::npos && eqPos + 1 < token.size())
                    {
                        std::string_view value(token.c_str() + eqPos + 1,
                                               token.size() - eqPos - 1);
                        if (!alreadyQuoted(std::string(value)))
                        {
                            result.append(token, 0, eqPos + 1);
                            result.push_back('"');
                            result.append(value);
                            result.push_back('"');
                        }
                        else
                        {
                            result.append(token);
                        }
                    }
                    else
                    {
                        result.append(token);
                    }
                }
                else
                {
                    result.push_back('"');
                    result.append(token);
                    result.push_back('"');
                }
            }
            else
            {
                result.append(token);
            }

            i = tokenEnd;
        }

        return result;
    }

    bool IsHeaderFile(const fs::path& filePath)
    {
        fs::path extension = filePath.extension();
        return HEADER_EXTENSIONS.find(extension.string()) != HEADER_EXTENSIONS.end();
    }

    bool IsSourceFile(const fs::path& filePath)
    {
        fs::path extension = filePath.extension();
        return SOURCE_EXTENSIONS.find(extension.string()) != SOURCE_EXTENSIONS.end();
    }

    fs::path GetHotswapIncludePath()
    {
        return fs::path(SKR_HOTSWAP_ROOT_PATH) / "include";
    }

    fs::path GetHotswapSourcePath()
    {
        return fs::path(SKR_HOTSWAP_ROOT_PATH) / "src";
    }

    fs::path GetHotswapExamplesPath()
    {
        return fs::path(SKR_HOTSWAP_ROOT_PATH) / "examples";
    }

    fs::path GetHotswapTestPath()
    {
        return fs::path(SKR_HOTSWAP_ROOT_PATH) / "test";
    }

    fs::path GetHotswapBuildPath()
    {
        return fs::path(SKR_HOTSWAP_BUILD_PATH);
    }

    fs::path GetHotswapBuildExamplesPath()
    {
        return fs::path(SKR_HOTSWAP_BUILD_PATH) / "examples";
    }

    fs::path GetHotswapBuildTestPath()
    {
        return fs::path(SKR_HOTSWAP_BUILD_PATH) / "test";
    }

    void SortFileEvents(const std::vector<IFileWatcher::Event>& events,
                        std::vector<fs::path>& canonicalModifiedFilePaths,
                        std::vector<fs::path>& canonicalRemovedFilePaths)
    {
        canonicalModifiedFilePaths.clear();
        canonicalRemovedFilePaths.clear();

        std::unordered_set<fs::path, FsPathHasher> dedupedModifiedFilePaths;
        std::unordered_set<fs::path, FsPathHasher> dedupedRemovedFilePaths;

        for (const auto& event : events)
        {
            std::error_code error;
            fs::path directoryPath = event.filePath.parent_path();
            fs::path canonicalDirectoryPath = fs::canonical(directoryPath, error);

            if (error.value() == SKR_HOTSWAP_ERROR_FILE_NOT_FOUND)
            {
                log::Warning() << "Directory " << directoryPath
                               << " was removed; hotswap does not support "
                                  "removing directories at runtime."
                               << log::End();
                continue;
            }

            fs::path canonicalFilePath = canonicalDirectoryPath / event.filePath.filename();

            if (fs::exists(canonicalFilePath))
            {
                if (fs::is_regular_file(canonicalFilePath))
                {
                    dedupedModifiedFilePaths.insert(canonicalFilePath);
                }
            }
            else
            {
                dedupedRemovedFilePaths.insert(canonicalFilePath);
            }
        }

        canonicalModifiedFilePaths = std::vector<fs::path>(
            dedupedModifiedFilePaths.begin(), dedupedModifiedFilePaths.end());
        canonicalRemovedFilePaths = std::vector<fs::path>(
            dedupedRemovedFilePaths.begin(), dedupedRemovedFilePaths.end());
    }

    fs::path FindFile(const fs::path& rootPath, const fs::path& name)
    {
        if (!fs::exists(rootPath))
        {
            return fs::path();
        }

        fs::path directory = fs::canonical(rootPath);

        for (const auto& entry : fs::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file() && fs::exists(entry))
            {
                if (entry.path().filename().compare(name) == 0)
                {
                    return entry.path();
                }
            }
        }

        log::Warning() << SKR_HOTSWAP_LOG_PREFIX << "Unable to find file " << name
                       << " within " << directory << log::End();
        return fs::path();
    }
} // namespace skr::hotswap::util
