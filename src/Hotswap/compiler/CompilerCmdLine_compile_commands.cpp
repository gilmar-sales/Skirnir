#include <fstream>
#include <sstream>

#define SIMDJSON_STATIC_REFLECTION 1
#include <simdjson.h>

#include "Skirnir/Hotswap/Util.hpp"
#include "Skirnir/Hotswap/compiler/CompilerCmdLine_compile_commands.hpp"

namespace skr::hotswap
{
    CompilerCmdLine_compile_commands::CompilerCmdLine_compile_commands(
        CompilerConfig* pConfig) : m_pConfig(pConfig)
    {
        std::ifstream compileCommandsFile(
            m_pConfig->compileCommandsJson.string().c_str(),
            std::ios::binary);

        if (!compileCommandsFile.is_open())
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to open compile_commands.json at '"
                         << m_pConfig->compileCommandsJson.u8string()
                         << "'. Set CMAKE_EXPORT_COMPILE_COMMANDS=ON or "
                            "populate Config::compiler.compileCommandsJson."
                         << log::End();
            return;
        }

        std::stringstream buffer;
        buffer << compileCommandsFile.rdbuf();
        const std::string contents = buffer.str();

        simdjson::dom::parser  parser;
        simdjson::dom::element root;
        auto                   error = parser.parse(contents).get(root);
        if (error)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to parse compile_commands.json at '"
                         << m_pConfig->compileCommandsJson.u8string() << "': "
                         << simdjson::error_message(error) << log::End();
            return;
        }

        simdjson::dom::array entries;
        error = root.get_array().get(entries);
        if (error)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "compile_commands.json root is not an array: "
                         << simdjson::error_message(error) << log::End();
            return;
        }

        for (simdjson::dom::element entry : entries)
        {
            simdjson::dom::object obj;
            if (entry.get_object().get(obj) != simdjson::SUCCESS)
            {
                continue;
            }

            std::string_view fileView, commandView;
            if (obj["file"].get(fileView) != simdjson::SUCCESS)
            {
                continue;
            }
            if (obj["command"].get(commandView) != simdjson::SUCCESS)
            {
                continue;
            }

            std::string key    = util::UnixSlashes(std::string(fileView));
            m_CommandsMap[key] = util::UnixSlashes(std::string(commandView));
        }
    }

    bool CompilerCmdLine_compile_commands::GenerateCommandFile(
        const fs::path&         commandFilePath,
        const fs::path&         moduleFilePath,
        const ICompiler::Input& input)
    {
        if (input.sourceFilePaths.empty())
        {
            return false;
        }

        const auto& sourceFile = input.sourceFilePaths.at(0);
        std::string key        = util::UnixSlashes(sourceFile.string());
        auto        it         = m_CommandsMap.find(key);
        if (it == m_CommandsMap.end())
        {
            // Retry with the canonical path (compile_commands.json often stores
            // canonical absolute paths).
            std::error_code error;
            fs::path        canonical = fs::canonical(sourceFile, error);
            if (error.value() == SKR_HOTSWAP_ERROR_SUCCESS)
            {
                key = util::UnixSlashes(canonical.string());
                it  = m_CommandsMap.find(key);
            }
        }
        if (it == m_CommandsMap.end())
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "compile_commands.json has no entry for '"
                         << sourceFile.string()
                         << "'. Add a build entry for this file and try again."
                         << log::End();
            return false;
        }

        // Strip the original `-o <something>` from the command, since we are
        // going to provide our own output path, then wrap every unquoted
        // non-flag value in double quotes so paths/values containing spaces
        // survive the shell round-trip intact.
        std::string command =
            util::QuoteArgs(util::RemoveArg(it->second, "-o"));

        std::ofstream commandFile(commandFilePath.string().c_str());
        if (!commandFile.is_open())
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to open command file " << commandFilePath
                         << log::End(".");
            return false;
        }

        // compile_commands.json entries are compile-only invocations (`-c`).
        // Strip `-c` so the GCC driver also runs the linker and produces a
        // shared library instead of a COFF object file. `-c` has no value, so
        // pass `takesValue=false` to avoid swallowing the source file token.
        command = util::RemoveArg(command, "-c", /*takesValue=*/false);

        // Strip `-x <lang>` (e.g. `-x c++`) too. Without `-c`, the `-x` sticky
        // flag would also force GCC to treat the appended `.a` archive as
        // C++ source, breaking the link step. The `.cpp` extension of the
        // source file already selects the language.
        command = util::RemoveArg(command, "-x", /*takesValue=*/true);

        // Compose: <original command> -shared -fPIC -fvisibility=hidden
        //          -o <moduleFilePath>
        // The compiler driver on each platform will accept the same flags.
        command += " -shared -fPIC -fvisibility=hidden -o \"" +
                   util::UnixSlashes(moduleFilePath.string()) + "\"";

        // Append link information supplied via Hotswapper::AddLibraryDirectory /
        // AddLibrary / AddLinkOption. compile_commands.json only carries the
        // compile invocation, so any libraries required at link time (e.g. the
        // skirnir runtime that defines ModuleSharedState) must be added here.
        for (const auto& libraryDirectory : input.libraryDirectoryPaths)
        {
            command += " -L\"" + util::UnixSlashes(libraryDirectory.string()) + "\"";
        }
        for (const auto& library : input.libraryPaths)
        {
            command += " \"" + util::UnixSlashes(library.string()) + "\"";
        }
        for (const auto& option : input.linkOptions)
        {
            command += " " + option;
        }

        log::Build() << command << log::End();
        commandFile << command;
        return true;
    }

    std::string CompilerCmdLine_compile_commands::GenerateCommand(
        const fs::path& moduleFilePath, const ICompiler::Input& input)
    {
        if (input.sourceFilePaths.empty())
        {
            return "";
        }

        const auto& sourceFile = input.sourceFilePaths.at(0);
        std::string key        = util::UnixSlashes(sourceFile.string());
        auto        it         = m_CommandsMap.find(key);
        if (it == m_CommandsMap.end())
        {
            // Retry with the canonical path (compile_commands.json often stores
            // canonical absolute paths).
            std::error_code error;
            fs::path        canonical = fs::canonical(sourceFile, error);
            if (error.value() == SKR_HOTSWAP_ERROR_SUCCESS)
            {
                key = util::UnixSlashes(canonical.string());
                it  = m_CommandsMap.find(key);
            }
        }
        if (it == m_CommandsMap.end())
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "compile_commands.json has no entry for '"
                         << sourceFile.string()
                         << "'. Add a build entry for this file and try again."
                         << log::End();
            return "";
        }

        // Strip the original `-o <something>` from the command, since we are
        // going to provide our own output path, then wrap every unquoted
        // non-flag value in double quotes so paths/values containing spaces
        // survive the shell round-trip intact.
        std::string command =
            util::QuoteArgs(util::RemoveArg(it->second, "-o"));

        // compile_commands.json entries are compile-only invocations (`-c`).
        // Strip `-c` so the GCC driver also runs the linker and produces a
        // shared library instead of a COFF object file. `-c` has no value, so
        // pass `takesValue=false` to avoid swallowing the source file token.
        command = util::RemoveArg(command, "-c", /*takesValue=*/false);

        // Strip `-x <lang>` (e.g. `-x c++`) too. Without `-c`, the `-x` sticky
        // flag would also force GCC to treat the appended `.a` archive as
        // C++ source, breaking the link step. The `.cpp` extension of the
        // source file already selects the language.
        command = util::RemoveArg(command, "-x", /*takesValue=*/true);

        // The compiler driver on each platform will accept the same flags.
        command += " -shared -fPIC -fvisibility=hidden -o \"" +
                   util::UnixSlashes(moduleFilePath.string()) + "\"";

        // Append link information supplied via Hotswapper::AddLibraryDirectory /
        // AddLibrary / AddLinkOption. compile_commands.json only carries the
        // compile invocation, so any libraries required at link time (e.g. the
        // skirnir runtime that defines ModuleSharedState) must be added here.
        for (const auto& libraryDirectory : input.libraryDirectoryPaths)
        {
            command += " -L\"" + util::UnixSlashes(libraryDirectory.string()) + "\"";
        }
        for (const auto& library : input.libraryPaths)
        {
            command += " \"" + util::UnixSlashes(library.string()) + "\"";
        }
        for (const auto& option : input.linkOptions)
        {
            command += " " + option;
        }

        log::Build() << command << log::End();
        return command;
    }

} // namespace skr::hotswap
