#pragma once

#include <array>
#include <cstdint>

namespace skr::hotswap::compile_time
{
    // Simple holder of constexpr integral, allowing Stringlen() calls to be cached.
    template <int Len>
    struct KeylenCache
    {
        static_assert(Len <= 128, "Exceeds maximum key length of 128 bytes.");
        static constexpr int len = Len;
    };

    // String stored in integrals; each byte in each uint64_t corresponds to a char.
    // Maximum key length is capped at 128 (16 * sizeof(uint64_t)).
    template <std::uint64_t S1, std::uint64_t S2, std::uint64_t S3, std::uint64_t S4,
              std::uint64_t S5, std::uint64_t S6, std::uint64_t S7, std::uint64_t S8,
              std::uint64_t S9, std::uint64_t S10, std::uint64_t S11, std::uint64_t S12,
              std::uint64_t S13, std::uint64_t S14, std::uint64_t S15, std::uint64_t S16>
    struct String
    {
        constexpr static std::array<std::uint64_t, 17> raw = {
            S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11, S12, S13, S14, S15, S16, 0
        };

        const char* ToString() const
        {
            return reinterpret_cast<const char*>(raw.data());
        }
    };

    template <std::uint64_t S1, std::uint64_t S2, std::uint64_t S3, std::uint64_t S4,
              std::uint64_t S5, std::uint64_t S6, std::uint64_t S7, std::uint64_t S8,
              std::uint64_t S9, std::uint64_t S10, std::uint64_t S11, std::uint64_t S12,
              std::uint64_t S13, std::uint64_t S14, std::uint64_t S15, std::uint64_t S16>
    constexpr std::array<std::uint64_t, 17>
        String<S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11, S12, S13, S14, S15, S16>::raw;

    constexpr int Strlen(const char* pStr)
    {
        return (*pStr == 0) ? 0 : 1 + Strlen(pStr + 1);
    }

    constexpr std::uint64_t StringSegmentToIntegral(const char* pStr, int iStartByte, int iByte, int len)
    {
        return (iByte >= 8 || iStartByte + iByte >= len)
                   ? 0
                   : StringSegmentToIntegral(pStr, iStartByte, iByte + 1, len) +
                         (static_cast<std::uint64_t>(pStr[iStartByte + iByte]) << (iByte * 8u));
    }

    constexpr std::uint64_t StringSegmentToIntegral(const char* pStr, int iUint64, int len)
    {
        return StringSegmentToIntegral(pStr, iUint64 * 8, 0, len);
    }
} // namespace skr::hotswap::compile_time
