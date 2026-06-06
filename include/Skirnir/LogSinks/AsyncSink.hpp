#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <stop_token>
#include <thread>

#include "../Common.hpp"
#include "ILogSink.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Decorator that forwards records to an inner sink from a
     *        background thread, with a bounded queue.
     *
     * @c Write() never blocks the caller. When the queue is full, the
     *        new record is dropped and a counter is incremented
     *        (exposable via @c DroppedCount).
     */
    class AsyncSink final : public ILogSink
    {
      public:
        explicit AsyncSink(Ref<ILogSink> inner,
                           std::size_t   queueCapacity = 8192);
        ~AsyncSink() override;

        void Write(const LogRecord& record) override;
        void Flush() override;

        std::uint64_t DroppedCount() const noexcept;

      private:
        void WorkerLoop(std::stop_token st);

        Ref<ILogSink>            mInner;
        std::mutex               mMutex;
        std::condition_variable  mCv;
        std::deque<LogRecord>    mQueue;
        std::size_t              mCapacity;
        std::jthread             mWorker;
        bool                     mStopping = false;
        std::atomic<std::uint64_t> mDropped {0};
    };
} // namespace SKIRNIR_NAMESPACE
