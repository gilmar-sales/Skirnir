#include "Skirnir/Hotswap/Config.hpp"
#include "Skirnir/Hotswap/Platform.hpp"
#include "Skirnir/Hotswap/Util.hpp"

#include "Skirnir/Hotswap/Private/CompileCommandsPath.hpp"

namespace skr::hotswap
{
    Config::Flag operator|(Config::Flag lhs, Config::Flag rhs)
    {
        return static_cast<Config::Flag>(static_cast<std::uint64_t>(lhs) |
                                        static_cast<std::uint64_t>(rhs));
    }

    Config::Flag operator|=(Config::Flag& lhs, Config::Flag rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    bool operator&(Config::Flag lhs, Config::Flag rhs)
    {
        return (static_cast<std::uint64_t>(lhs) & static_cast<std::uint64_t>(rhs)) != 0;
    }

    CompilerConfig::CompilerConfig()
    {
        fs::path fallback =
            fs::path(skr::hotswap::detail::kDefaultCompileCommandsJson);
        if (fallback.empty())
        {
            fallback = fs::current_path() / "compile_commands.json";
        }
        compileCommandsJson = fallback;
    }
} // namespace skr::hotswap
