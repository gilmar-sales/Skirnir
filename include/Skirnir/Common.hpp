#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#define SKIRNIR_NAMESPACE skr

#include "ServiceId.hpp"

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T>
using WeakRef = std::weak_ptr<T>;

#ifdef _MSC_VER
    #define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace SKIRNIR_NAMESPACE
{
    template <typename T, typename... TArgs>
        requires(std::is_constructible_v<T, TArgs...>)
    inline Ref<T> MakeRef(TArgs&&... args)
    {
        return std::make_shared<T>(std::forward<TArgs>(args)...);
    }

    /**
     * @brief Performs a static pointer cast on a shared_ptr.
     *
     * @tparam TDest  The target type to cast to
     * @tparam TSource The source type being cast from
     * @param source  The shared_ptr to cast
     * @return        A shared_ptr of type TDest, or nullptr if cast fails
     */
    template <typename TDest, typename TSource>
    inline Ref<TDest> RefCast(const Ref<TSource>& source)
    {
        return std::static_pointer_cast<TDest>(source);
    }

    enum class LifeTime
    {
        Transient,
        Scoped,
        Singleton
    };

    class ServiceCollection;
    class ServiceProvider;
    class ServiceScope;

    using ServiceFactory = std::function<Ref<void>(ServiceProvider&)>;

    struct ServiceDescription
    {
        ServiceId id;
        char*     name;

        bool operator<(const ServiceDescription& other) const
        {
            return id < other.id;
        }

        bool operator==(const ServiceDescription& other) const
        {
            return id == other.id;
        }

        bool operator!=(const ServiceDescription& other) const
        {
            return id != other.id;
        }
    };

    using InternalServiceFactory = std::function<Ref<void>(
        ServiceProvider&, std::set<ServiceDescription>&)>;

    struct ServiceDefinition
    {
        std::function<Ref<void>(
            ServiceProvider&, std::set<ServiceDescription>&)>
                 factory  = nullptr;
        LifeTime lifetime = LifeTime::Transient;
    };

    template <typename TKey, typename TValue>
    class OrderedVectorMap
    {
      public:
        using value_type     = std::pair<TKey, TValue>;
        using container_type = std::vector<value_type>;
        using iterator       = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;

        std::pair<iterator, bool> insert(value_type value)
        {
            const auto it = lowerBound(mItems, value.first);

            if (it != mItems.end() && it->first == value.first)
                return { it, false };

            return { mItems.insert(it, std::move(value)), true };
        }

        template <typename... TArgs>
        std::pair<iterator, bool> emplace(const TKey& key, TArgs&&... args)
        {
            return insert(
                value_type(key, TValue(std::forward<TArgs>(args)...)));
        }

        iterator find(const TKey& key)
        {
            const auto it = lowerBound(mItems, key);

            if (it != mItems.end() && it->first == key)
                return it;

            return mItems.end();
        }

        const_iterator find(const TKey& key) const
        {
            const auto it = lowerBound(mItems, key);

            if (it != mItems.end() && it->first == key)
                return it;

            return mItems.end();
        }

        bool contains(const TKey& key) const
        {
            return find(key) != mItems.end();
        }

        TValue& at(const TKey& key)
        {
            const auto it = find(key);

            if (it == mItems.end())
                throw std::out_of_range("OrderedVectorMap::at: key not found");

            return it->second;
        }

        const TValue& at(const TKey& key) const
        {
            const auto it = find(key);

            if (it == mItems.end())
                throw std::out_of_range("OrderedVectorMap::at: key not found");

            return it->second;
        }

        iterator begin() noexcept { return mItems.begin(); }
        iterator end() noexcept { return mItems.end(); }

        const_iterator begin() const noexcept { return mItems.begin(); }
        const_iterator end() const noexcept { return mItems.end(); }

        bool empty() const noexcept { return mItems.empty(); }
        std::size_t size() const noexcept { return mItems.size(); }

      private:
        container_type mItems;

        static iterator lowerBound(container_type& c, const TKey& key)
        {
            return std::lower_bound(
                c.begin(),
                c.end(),
                key,
                [](const value_type& item, const TKey& k) {
                    return item.first < k;
                });
        }

        static const_iterator lowerBound(const container_type& c,
                                         const TKey&         key)
        {
            return std::lower_bound(
                c.begin(),
                c.end(),
                key,
                [](const value_type& item, const TKey& k) {
                    return item.first < k;
                });
        }
    };

    using ServiceDefinitionMap = OrderedVectorMap<ServiceId, ServiceDefinition>;
    using ServicesCache        = OrderedVectorMap<ServiceId, Ref<void>>;

} // namespace SKIRNIR_NAMESPACE
