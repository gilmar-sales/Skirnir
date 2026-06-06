#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include "Namespace.hpp"

namespace SKIRNIR_NAMESPACE
{
    namespace detail
    {
        struct ArcControlBlock
        {
            std::atomic<std::size_t> strong { 0 };
            std::atomic<std::size_t> weak   { 0 };
            void*                    payload = nullptr;
            void (*dispose)(void*) noexcept            = nullptr;
            void (*destroy)(ArcControlBlock*) noexcept = nullptr;

            void increment_strong() noexcept
            {
                strong.fetch_add(1, std::memory_order_relaxed);
            }

            void increment_weak() noexcept
            {
                weak.fetch_add(1, std::memory_order_relaxed);
            }

            void release_strong() noexcept
            {
                if (strong.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    dispose(payload);
                    release_weak();
                }
            }

            void release_weak() noexcept
            {
                if (weak.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    destroy(this);
                }
            }

            bool try_increment_strong() noexcept
            {
                std::size_t current =
                    strong.load(std::memory_order_relaxed);
                while (current != 0)
                {
                    if (strong.compare_exchange_weak(
                            current, current + 1,
                            std::memory_order_relaxed,
                            std::memory_order_relaxed))
                    {
                        return true;
                    }
                }
                return false;
            }
        };

        template <typename T>
        inline void arc_dispose_destroy_in_place(void* payload) noexcept
        {
            static_cast<T*>(payload)->~T();
        }

        template <typename T>
        inline void arc_destroy_combined(ArcControlBlock* cb) noexcept
        {
            ::operator delete(cb);
        }
    } // namespace detail

    template <typename T>
    class WeakArc;

    template <typename T>
    class enable_arc_from_this;

    template <typename TDest, typename TSource>
    struct ArcCastAccess;

    template <typename T>
    class Arc
    {
      public:
        using element_type = T;

        constexpr Arc() noexcept = default;

        Arc(const Arc& other) noexcept
            : mPtr(other.mPtr), mCtrl(other.mCtrl)
        {
            if (mCtrl)
                mCtrl->increment_strong();
        }

        template <typename U>
            requires(std::is_convertible_v<U*, T*>)
        Arc(const Arc<U>& other) noexcept
        {
            auto tmp = ArcCastAccess<T, U>::cast(other);
            swap(tmp);
        }

        Arc(Arc&& other) noexcept : mPtr(other.mPtr), mCtrl(other.mCtrl)
        {
            other.mPtr  = nullptr;
            other.mCtrl = nullptr;
        }

        constexpr Arc(std::nullptr_t) noexcept
        {
        }

        ~Arc()
        {
            if (mCtrl)
                mCtrl->release_strong();
        }

        Arc& operator=(const Arc& other) noexcept
        {
            if (this == &other)
                return *this;
            Arc tmp(other);
            swap(tmp);
            return *this;
        }

        Arc& operator=(Arc&& other) noexcept
        {
            Arc tmp(std::move(other));
            swap(tmp);
            return *this;
        }

        Arc(T* ptr, detail::ArcControlBlock* ctrl) noexcept
            : mPtr(ptr), mCtrl(ctrl)
        {
        }

        T* get() const noexcept { return mPtr; }

        explicit operator bool() const noexcept { return mPtr != nullptr; }

        void reset() noexcept { Arc().swap(*this); }

        void swap(Arc& other) noexcept
        {
            std::swap(mPtr, other.mPtr);
            std::swap(mCtrl, other.mCtrl);
        }

        std::size_t use_count() const noexcept
        {
            return mCtrl ? mCtrl->strong.load(std::memory_order_relaxed) : 0;
        }

        bool unique() const noexcept { return use_count() == 1; }

        template <bool Enable = !std::is_void_v<T>>
            requires Enable
        auto& operator*() const noexcept
        {
            return *static_cast<T*>(mPtr);
        }

        template <bool Enable = !std::is_void_v<T>>
            requires Enable
        auto operator->() const noexcept
        {
            return static_cast<T*>(mPtr);
        }

      private:
        template <typename>
        friend class Arc;

        template <typename U, typename... UArgs>
        friend Arc<U> MakeArc(UArgs&&... args);

        template <typename>
        friend class WeakArc;

        template <typename>
        friend class enable_arc_from_this;

        template <typename TDest, typename TSource>
        friend struct ArcCastAccess;

        T*                       mPtr  = nullptr;
        detail::ArcControlBlock* mCtrl = nullptr;
    };

    template <typename T>
    void swap(Arc<T>& lhs, Arc<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <typename T, typename U>
    bool operator==(const Arc<T>& lhs, const Arc<U>& rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    template <typename T, typename U>
    bool operator!=(const Arc<T>& lhs, const Arc<U>& rhs) noexcept
    {
        return lhs.get() != rhs.get();
    }

    template <typename T>
    bool operator==(const Arc<T>& lhs, std::nullptr_t) noexcept
    {
        return !lhs;
    }

    template <typename T>
    bool operator==(std::nullptr_t, const Arc<T>& rhs) noexcept
    {
        return !rhs;
    }

    template <typename T>
    bool operator!=(const Arc<T>& lhs, std::nullptr_t) noexcept
    {
        return static_cast<bool>(lhs);
    }

    template <typename T>
    bool operator!=(std::nullptr_t, const Arc<T>& rhs) noexcept
    {
        return static_cast<bool>(rhs);
    }

    template <typename T, typename U>
    bool operator<(const Arc<T>& lhs, const Arc<U>& rhs) noexcept
    {
        return std::less<const void*>()(
            static_cast<const void*>(lhs.get()),
            static_cast<const void*>(rhs.get()));
    }

    template <typename T, typename U>
    bool operator<=(const Arc<T>& lhs, const Arc<U>& rhs) noexcept
    {
        return !(rhs < lhs);
    }

    template <typename T, typename U>
    bool operator>(const Arc<T>& lhs, const Arc<U>& rhs) noexcept
    {
        return rhs < lhs;
    }

    template <typename T, typename U>
    bool operator>=(const Arc<T>& lhs, const Arc<U>& rhs) noexcept
    {
        return !(lhs < rhs);
    }

    template <typename CharT, typename Traits, typename T>
    std::basic_ostream<CharT, Traits>& operator<<(
        std::basic_ostream<CharT, Traits>& os, const Arc<T>& arc)
    {
        os << arc.get();
        return os;
    }

    template <typename T>
    struct is_arc : std::false_type
    {
    };

    template <typename T>
    struct is_arc<Arc<T>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_arc_v = is_arc<T>::value;

    template <typename T, typename... TArgs>
        requires(std::is_constructible_v<T, TArgs...>)
    inline Arc<T> MakeArc(TArgs&&... args)
    {
        void* mem = ::operator new(sizeof(detail::ArcControlBlock) + sizeof(T));

        auto* cb = ::new (mem) detail::ArcControlBlock();
        T*    obj =
            ::new (reinterpret_cast<char*>(mem) +
                   sizeof(detail::ArcControlBlock))
                T(std::forward<TArgs>(args)...);

        if constexpr (std::is_base_of_v<enable_arc_from_this<T>, T>)
        {
            static_cast<enable_arc_from_this<T>*>(obj)
                ->_skr_attach_control_block(obj, cb);
        }

        cb->payload = obj;
        cb->dispose = &detail::arc_dispose_destroy_in_place<T>;
        cb->destroy = &detail::arc_destroy_combined<T>;
        cb->strong.store(1, std::memory_order_relaxed);
        cb->weak.store(1, std::memory_order_relaxed);

        return Arc<T>(obj, cb);
    }

    template <typename TDest, typename TSource>
    struct ArcCastAccess
    {
        static Arc<TDest> cast(const Arc<TSource>& source) noexcept
        {
            Arc<TDest> result;
            if (source.mCtrl)
            {
                result.mPtr  = static_cast<TDest*>(source.mPtr);
                result.mCtrl = source.mCtrl;
                result.mCtrl->increment_strong();
            }
            return result;
        }
    };

    template <typename TDest, typename TSource>
    inline Arc<TDest> ArcCast(const Arc<TSource>& source) noexcept
    {
        return ArcCastAccess<TDest, TSource>::cast(source);
    }

    template <typename T>
    class WeakArc
    {
      public:
        constexpr WeakArc() noexcept = default;

        WeakArc(const Arc<T>& arc) noexcept
            : mPtr(arc.mPtr), mCtrl(arc.mCtrl)
        {
            if (mCtrl)
                mCtrl->increment_weak();
        }

        WeakArc(const WeakArc& other) noexcept
            : mPtr(other.mPtr), mCtrl(other.mCtrl)
        {
            if (mCtrl)
                mCtrl->increment_weak();
        }

        WeakArc(WeakArc&& other) noexcept
            : mPtr(other.mPtr), mCtrl(other.mCtrl)
        {
            other.mPtr  = nullptr;
            other.mCtrl = nullptr;
        }

        ~WeakArc()
        {
            if (mCtrl)
                mCtrl->release_weak();
        }

        WeakArc& operator=(const Arc<T>& arc) noexcept
        {
            WeakArc tmp(arc);
            swap(tmp);
            return *this;
        }

        WeakArc& operator=(const WeakArc& other) noexcept
        {
            if (this == &other)
                return *this;
            WeakArc tmp(other);
            swap(tmp);
            return *this;
        }

        WeakArc& operator=(WeakArc&& other) noexcept
        {
            WeakArc tmp(std::move(other));
            swap(tmp);
            return *this;
        }

        void swap(WeakArc& other) noexcept
        {
            std::swap(mPtr, other.mPtr);
            std::swap(mCtrl, other.mCtrl);
        }

        void reset() noexcept { WeakArc().swap(*this); }

        std::size_t use_count() const noexcept
        {
            return mCtrl ? mCtrl->strong.load(std::memory_order_relaxed) : 0;
        }

        bool expired() const noexcept { return use_count() == 0; }

        Arc<T> lock() const noexcept
        {
            if (mCtrl && mCtrl->try_increment_strong())
                return Arc<T>(mPtr, mCtrl);
            return Arc<T>();
        }

      private:
        T*                        mPtr  = nullptr;
        detail::ArcControlBlock*  mCtrl = nullptr;
    };

    template <typename T>
    void swap(WeakArc<T>& lhs, WeakArc<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <typename T>
    class enable_arc_from_this
    {
      public:
        constexpr enable_arc_from_this() noexcept = default;

        enable_arc_from_this(const enable_arc_from_this&) noexcept
        {
        }

        enable_arc_from_this& operator=(
            const enable_arc_from_this&) noexcept
        {
            return *this;
        }

        ~enable_arc_from_this() = default;

        Arc<T> shared_from_this()
        {
            if (!mCtrl)
                return Arc<T>();
            Arc<T> result;
            result.mPtr  = static_cast<T*>(mSelf);
            result.mCtrl = mCtrl;
            mCtrl->increment_strong();
            return result;
        }

        Arc<T> shared_from_this() const
        {
            if (!mCtrl)
                return Arc<T>();
            Arc<T> result;
            result.mPtr  = static_cast<T*>(mSelf);
            result.mCtrl = mCtrl;
            mCtrl->increment_strong();
            return result;
        }

      public:
        template <typename U>
        friend class Arc;

        void _skr_attach_control_block(T*                       self,
                                       detail::ArcControlBlock* cb) noexcept
        {
            mSelf = self;
            mCtrl = cb;
        }

      private:
        T*                       mSelf = nullptr;
        detail::ArcControlBlock* mCtrl = nullptr;
    };
} // namespace SKIRNIR_NAMESPACE
