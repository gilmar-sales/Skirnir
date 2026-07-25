#pragma once

#include <vector>

#include "Skirnir/Hotswap/Filesystem.hpp"

namespace skr::hotswap
{
    class IFileWatcher
    {
      public:
        struct Event
        {
            fs::path filePath;
        };

        virtual ~IFileWatcher() = default;

        virtual bool AddWatch(const fs::path& directoryPath) = 0;
        virtual bool RemoveWatch(const fs::path& directoryPath) = 0;
        virtual void ClearAllWatches() = 0;

        virtual void PollChanges(std::vector<Event>& events) = 0;
    };
} // namespace skr::hotswap
