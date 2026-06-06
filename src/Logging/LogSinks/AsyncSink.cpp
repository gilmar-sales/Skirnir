#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Logging/LogRecord.hpp"
#include "Skirnir/Logging/LogSinks/AsyncSink.hpp"

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace SKIRNIR_NAMESPACE
{
    AsyncSink::AsyncSink(Arc<ILogSink> inner, std::size_t queueCapacity) :
        mInner(std::move(inner)), mCapacity(queueCapacity ? queueCapacity : 1)
    {
        if (!mInner)
        {
            throw std::runtime_error(
                "Skirnir: AsyncSink requires a non-null inner sink");
        }
        mWorker = std::jthread([this](std::stop_token st) { WorkerLoop(st); });
    }

    AsyncSink::~AsyncSink()
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mStopping = true;
        }
        mCv.notify_all();
        if (mWorker.joinable())
            mWorker.join();
        if (mInner)
            mInner->Flush();
    }

    void AsyncSink::Write(const LogRecord& r)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mStopping)
        {
            mDropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        if (mQueue.size() >= mCapacity)
        {
            mDropped.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        mQueue.push_back(r);
        lock.unlock();
        mCv.notify_one();
    }

    void AsyncSink::Flush()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCv.wait(lock, [this] { return mQueue.empty(); });
        if (mInner)
            mInner->Flush();
    }

    std::uint64_t AsyncSink::DroppedCount() const noexcept
    {
        return mDropped.load(std::memory_order_relaxed);
    }

    void AsyncSink::WorkerLoop(std::stop_token st)
    {
        while (true)
        {
            LogRecord record;
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mCv.wait(lock,
                         [this, &st] { return mStopping || !mQueue.empty(); });
                if (mStopping && mQueue.empty())
                    return;
                if (mQueue.empty())
                {
                    if (st.stop_requested())
                        return;
                    continue;
                }
                record = std::move(mQueue.front());
                mQueue.pop_front();
            }
            // Notify Flush() waiters that the queue shrank.
            mCv.notify_all();
            try
            {
                mInner->Write(record);
            }
            catch (...)
            {
                // Sinks must not throw; swallow defensively.
            }
        }
    }
} // namespace SKIRNIR_NAMESPACE
