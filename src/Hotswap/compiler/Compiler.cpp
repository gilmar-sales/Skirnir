#include <cassert>

#include "Skirnir/Hotswap/compiler/Compiler.hpp"
#include "Skirnir/Hotswap/Platform.hpp"

namespace skr::hotswap
{
    namespace
    {
        const std::string COMMAND_FILENAME = "cmdfile";
        const std::string MODULE_FILENAME =
            "module." + platform::GetSharedLibraryExtension();
    } // namespace

    Compiler::Compiler(CompilerConfig* pConfig,
                       std::unique_ptr<ICmdShellTask> pInitializeTask,
                       std::unique_ptr<ICompilerCmdLine> pCompilerCmdLine)
        : m_pConfig(pConfig)
        , m_pInitializeTask(std::move(pInitializeTask))
        , m_pCompilerCmdLine(std::move(pCompilerCmdLine))
    {
        m_pCmdShell = platform::CreateCmdShell();
        if (!m_pCmdShell->CreateCmdProcess())
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to create compiler cmd process." << log::End();
            return;
        }
        log::Info() << SKR_HOTSWAP_LOG_PREFIX << "Initializing compiler." << log::End();
        m_pInitializeTask->Start(
            m_pCmdShell.get(), m_pConfig->initializeTimeout,
            [this](ICmdShellTask::Result result) {
                switch (result)
                {
                    case ICmdShellTask::Result::Success:
                        m_bInitialized = true;
                        log::Info() << SKR_HOTSWAP_LOG_PREFIX
                                   << "Compiler has been initialized successfully."
                                   << log::End();
                        break;
                    case ICmdShellTask::Result::Failure:
                        m_bInitializationFailed = true;
                        log::Error() << SKR_HOTSWAP_LOG_PREFIX
                                     << "Failed to initialize Compiler." << log::End();
                        break;
                    default:
                        assert(false);
                }
            });
    }

    bool Compiler::IsInitialized()
    {
        return m_bInitialized;
    }

    bool Compiler::StartBuild(const ICompiler::Input& input)
    {
        if (m_bInitializationFailed)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Compiler failed initialization phase, cannot compile."
                         << log::End();
            return false;
        }
        if (!m_bInitialized)
        {
            log::Info() << SKR_HOTSWAP_LOG_PREFIX
                        << "Compiler is still initializing, skipping compilation."
                        << log::End();
            return false;
        }

        fs::path commandFilePath = input.buildDirectoryPath / COMMAND_FILENAME;
        fs::path moduleFilePath = input.buildDirectoryPath / MODULE_FILENAME;
        auto command = m_pCompilerCmdLine->GenerateCommand(moduleFilePath, input);
        if (!m_pCompilerCmdLine->GenerateCommandFile(commandFilePath, moduleFilePath, input))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX << "Failed to generate command file."
                         << log::End();
            return false;
        }

        m_iCompileOutput = 0;
        m_CompiledModulePath.clear();
        m_CompilingModulePath = moduleFilePath;

        std::string cmd =
            "\"" + m_pConfig->executable.string() + "\" @\"" +
            input.buildDirectoryPath.string() + "/" + COMMAND_FILENAME + "\"";

        m_pCmdShell->StartTask(command, static_cast<int>(CompilerTask::Build));
        return true;
    }

    void Compiler::Update()
    {
        if (m_bInitializationFailed)
        {
            return;
        }
        if (!m_bInitialized)
        {
            m_pInitializeTask->Update();
            return;
        }
        int taskId = -1;
        ICmdShell::TaskState taskState = m_pCmdShell->Update(taskId);

        if (static_cast<CompilerTask>(taskId) == CompilerTask::Build)
        {
            const std::vector<std::string>& output = m_pCmdShell->PeekTaskOutput();
            for (; m_iCompileOutput < output.size(); ++m_iCompileOutput)
            {
                log::Build() << output.at(m_iCompileOutput) << log::End();
            }
        }

        switch (taskState)
        {
            case ICmdShell::TaskState::Running:
            case ICmdShell::TaskState::Idle:
                break;
            case ICmdShell::TaskState::Done:
                HandleTaskComplete(static_cast<CompilerTask>(taskId));
                break;
            case ICmdShell::TaskState::Error:
                log::Error() << SKR_HOTSWAP_LOG_PREFIX
                             << "Compiler shell task '" << taskId
                             << "' resulted in error." << log::End();
                break;
            default:
                assert(false);
        }
    }

    bool Compiler::IsCompiling()
    {
        return !m_CompilingModulePath.empty();
    }

    bool Compiler::HasCompiledModule()
    {
        return !m_CompiledModulePath.empty();
    }

    fs::path Compiler::PopModule()
    {
        fs::path modulePath = m_CompiledModulePath;
        m_CompiledModulePath.clear();
        return modulePath;
    }

    void Compiler::HandleTaskComplete(Compiler::CompilerTask task)
    {
        switch (task)
        {
            case CompilerTask::Build:
                HandleBuildTaskComplete();
                break;
            default:
                assert(false);
        }
    }

    void Compiler::HandleBuildTaskComplete()
    {
        m_CompiledModulePath = m_CompilingModulePath;
        m_CompilingModulePath.clear();
    }
} // namespace skr::hotswap
