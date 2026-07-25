#pragma once

#include "Skirnir/Hotswap/compiler/ICompilerCmdLine.hpp"
#include "Skirnir/Hotswap/cmd-shell/ICmdShell.hpp"
#include "Skirnir/Hotswap/cmd-shell/ICmdShellTask.hpp"
#include "Skirnir/Hotswap/Config.hpp"

namespace skr::hotswap
{
    class Compiler : public ICompiler
    {
      public:
        Compiler(CompilerConfig* pConfig,
                 std::unique_ptr<ICmdShellTask> pInitializeTask,
                 std::unique_ptr<ICompilerCmdLine> pCompilerCmdLine);

        bool IsInitialized() override;

        bool StartBuild(const Input& input) override;
        void Update() override;

        bool IsCompiling() override;

        bool HasCompiledModule() override;
        fs::path PopModule() override;

      private:
        enum class CompilerTask
        {
            Build,
        };

        CompilerConfig* m_pConfig = nullptr;

        bool m_bInitialized = false;
        bool m_bInitializationFailed = false;

        std::size_t m_iCompileOutput = 0;
        fs::path m_CompilingModulePath;
        fs::path m_CompiledModulePath;

        std::unique_ptr<ICmdShell> m_pCmdShell;
        std::unique_ptr<ICmdShellTask> m_pInitializeTask;
        std::unique_ptr<ICompilerCmdLine> m_pCompilerCmdLine;

        void HandleTaskComplete(CompilerTask task);
        void HandleBuildTaskComplete();
    };
} // namespace skr::hotswap
