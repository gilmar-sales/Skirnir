#include "Skirnir/Hotswap/ProtectedFunction.hpp"

#if defined(SKR_HOTSWAP_PLATFORM_WIN32)

    #include <windows.h>

#endif

namespace skr::hotswap
{
    ProtectedFunction::Result ProtectedFunction::Call(
        const std::function<void()>& cb)
    {
        try
        {
            cb();
            return Result::Success;
        }
        catch (const std::exception&)
        {
            return Result::Exception;
        }
    }
} // namespace skr::hotswap
