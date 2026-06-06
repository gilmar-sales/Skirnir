#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <ostream>

#include "ILogSink.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Emits records as NDJSON (one JSON object per line) to either
     *        an existing @c std::ostream or a file path.
     */
    class JsonSink final : public ILogSink
    {
      public:
        explicit JsonSink(std::ostream& os);
        explicit JsonSink(std::filesystem::path path);

        void Write(const LogRecord& record) override;
        void Flush() override;

      private:
        std::unique_ptr<std::ofstream> mOwned;
        std::ostream&                  mOs;
        std::mutex                     mMutex;
    };
} // namespace SKIRNIR_NAMESPACE
