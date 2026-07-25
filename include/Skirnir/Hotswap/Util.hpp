#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>

#include "Skirnir/Hotswap/Log.hpp"
#include "Skirnir/Hotswap/Platform.hpp"
#include "Skirnir/Hotswap/file-watcher/IFileWatcher.hpp"
#include "Skirnir/Hotswap/FsPathHasher.hpp"

namespace skr::hotswap::util
{
    bool IsWhitespace(const std::string& str);
    std::string Trim(const std::string& str);
    std::string Quote(const std::string& str);
    std::string UnixSlashes(const std::string& str);

    // Removes all occurrences of `-arg` (and the value that follows it) from a
    // shell-like command string. The following value may be quoted with double
    // quotes; whitespace inside quotes is preserved. If `takesValue` is false,
    // only the flag token itself is removed and the following token is left
    // intact (use this for switches like `-c`/`/c` that have no argument).
    std::string RemoveArg(const std::string& command,
                          const std::string& arg,
                          bool                takesValue = true);

    // Walks a shell-like command string and wraps every value token (any token
    // that does not start with `-` and is not already double-quoted) in double
    // quotes. Flag tokens (`-I`, `-D`, etc.) are emitted as-is. Tokens already
    // enclosed in double quotes are emitted unchanged.
    std::string QuoteArgs(const std::string& command);

    bool IsHeaderFile(const fs::path& filePath);
    bool IsSourceFile(const fs::path& filePath);

    fs::path GetHotswapIncludePath();
    fs::path GetHotswapSourcePath();
    fs::path GetHotswapExamplesPath();
    fs::path GetHotswapTestPath();
    fs::path GetHotswapBuildPath();
    fs::path GetHotswapBuildExamplesPath();
    fs::path GetHotswapBuildTestPath();

    fs::path FindFile(const fs::path& rootPath, const fs::path& name);

    void SortFileEvents(const std::vector<IFileWatcher::Event>& events,
                        std::vector<fs::path>& canonicalModifiedFilePaths,
                        std::vector<fs::path>& canonicalRemovedFilePaths);

    template <typename T, typename THasher = std::hash<T>>
    void Deduplicate(std::vector<T>& input)
    {
        std::unordered_set<T, THasher> seen;
        input.erase(std::remove_if(input.begin(), input.end(), [&seen](const T& val) {
            if (seen.find(val) != seen.end())
            {
                return true;
            }
            seen.insert(val);
            return false;
        }), input.end());
    }
} // namespace skr::hotswap::util
