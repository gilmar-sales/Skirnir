#pragma once

#include <cstdint>
#include <limits>

namespace skr::hotswap
{
    struct AllocationInfo
    {
        std::uint64_t id = (std::numeric_limits<std::uint64_t>::max)();
        std::uint8_t* pMemory = nullptr;
    };

    class IAllocator
    {
      public:
        virtual ~IAllocator() = default;

        // Called when an object is first constructed.
        virtual AllocationInfo Skr_Allocate(std::uint64_t size) = 0;

        // Called when an object is undergoing a runtime swap, and a new object is
        // being constructed to replace an old implementation.
        virtual AllocationInfo Skr_AllocateSwap(std::uint64_t previousId, std::uint64_t size) = 0;

        // Called when an object is freed during a runtime swap, and should return
        // the old object's id.
        virtual std::uint64_t Skr_FreeSwap(std::uint8_t* pMemory) = 0;
    };
} // namespace skr::hotswap
