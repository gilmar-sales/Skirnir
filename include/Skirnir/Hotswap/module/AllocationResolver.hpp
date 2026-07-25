#pragma once

#include <cstdint>
#include <type_traits>

#include "Skirnir/Hotswap/module/IAllocator.hpp"
#include "Skirnir/Hotswap/module/ModuleSharedState.hpp"
#include "Skirnir/Hotswap/module/Constructors.hpp"

namespace skr::hotswap
{
    // Member detector idiom: if T contains an skr_ClassTracker member, it has been
    // registered for tracking with SKR_TRACK and should be constructed through a
    // skr::hotswap Constructor.
    template <typename T>
    class IsTracked
    {
        struct Fallback
        {
            int skr_ClassTracker;
        };

        struct Derived : T, Fallback
        {
        };

        using NoType = char[1];
        using YesType = char[2];

        template <typename U, U>
        struct Check;

        template <typename U>
        static NoType& Test(Check<int Fallback::*, &U::skr_ClassTracker>*);

        template <typename U>
        static YesType& Test(...);

      public:
        enum
        {
            no = (sizeof(Test<Derived>(0)) == sizeof(NoType))
        };
        enum
        {
            yes = (sizeof(Test<Derived>(0)) == sizeof(YesType))
        };
    };

    class AllocationResolver
    {
      public:
        template <typename T>
        typename std::enable_if<IsTracked<T>::yes, void>::type
        Allocate(AllocationInfo& info)
        {
            const char* pKey = decltype(T::skr_ClassKey)().ToString();
            auto constructorIt = ModuleSharedState::s_pConstructorsByKey->find(pKey);
            if (constructorIt != ModuleSharedState::s_pConstructorsByKey->end())
            {
                info = constructorIt->second->Allocate();
            }
            else
            {
                info = AllocationInfo();
            }
        }

        template <typename T>
        typename std::enable_if<IsTracked<T>::no, void>::type
        Allocate(AllocationInfo& info)
        {
            if (ModuleSharedState::s_pAllocator != nullptr)
            {
                std::uint64_t size = sizeof(typename std::aligned_storage<sizeof(T)>::type);
                info = ModuleSharedState::s_pAllocator->Skr_Allocate(size);
                new (info.pMemory) T;
            }
            else
            {
                info = AllocationInfo();
                info.pMemory = reinterpret_cast<std::uint8_t*>(new T());
            }
        }

        template <typename T>
        T* Allocate()
        {
            AllocationInfo info;
            Allocate<T>(info);
            return reinterpret_cast<T*>(info.pMemory);
        }
    };
} // namespace skr::hotswap
