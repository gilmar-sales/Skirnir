#ifdef WIN32
    #include "Skirnir/Hotswap/cmd-shell/CmdShell_win32.hpp"
    #include "Skirnir/Hotswap/Util.hpp"

namespace skr::hotswap
{
    namespace
    {
        const std::string TASK_COMPLETION_KEY =
            "__skr_task_complete(c4c2c81b-0a1f-4ad7-bd50-b6e8e2a0d5ee)__";
    } // namespace

    CmdShell::~CmdShell()
    {
        if (m_hProcess != INVALID_HANDLE_VALUE)
        {
            if (!TerminateProcess(m_hProcess, EXIT_SUCCESS))
            {
                log::Warning() << SKR_HOTSWAP_LOG_PREFIX
                               << "Failed to terminate cmd process. "
                               << log::LastOsError() << log::End();
            }
        }
    }

    bool CmdShell::CreateCmdProcess()
    {
        SECURITY_ATTRIBUTES securityAttrs;
        securityAttrs.nLength              = sizeof(SECURITY_ATTRIBUTES);
        securityAttrs.lpSecurityDescriptor = nullptr;
        securityAttrs.bInheritHandle       = true;

        HANDLE hStdoutWrite = INVALID_HANDLE_VALUE;
        if (!CreatePipe(&m_hStdoutRead, &hStdoutWrite, &securityAttrs, 0))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to create stdout pipe. "
                         << log::LastOsError() << log::End();
            return false;
        }
        HANDLE hStdinRead = INVALID_HANDLE_VALUE;
        if (!CreatePipe(&hStdinRead, &m_hStdinWrite, &securityAttrs, 0))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to create stdin pipe." << log::LastOsError()
                         << log::End();
            return false;
        }

        PROCESS_INFORMATION processInfo;
        ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

        STARTUPINFOW startupInfo;
        ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
        startupInfo.cb         = sizeof(STARTUPINFO);
        startupInfo.hStdError  = hStdoutWrite;
        startupInfo.hStdOutput = hStdoutWrite;
        startupInfo.hStdInput  = hStdinRead;
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;

        std::wstring cmdLine = L"cmd /q /k @PROMPT $";

        std::string_view  narrow     = "D:/dev/Skirnir/build";
        std::wstring workingDir = std::wstring(narrow.begin(), narrow.end());
        bool         bSuccess =
            (CreateProcessW(NULL, const_cast<wchar_t*>(cmdLine.c_str()), NULL,
                            NULL, true, 0, NULL, workingDir.c_str(),
                            &startupInfo, &processInfo) != 0);
        if (!bSuccess)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to create cmd process. "
                         << log::LastOsError() << log::End();
            return false;
        }
        m_hProcess = processInfo.hProcess;
        CloseHandle(processInfo.hThread);
        CloseHandle(hStdoutWrite);
        CloseHandle(hStdinRead);
        return true;
    }

    void CmdShell::StartTask(const std::string& command, int taskId)
    {
        Clear();
        bool bSuccess = SendCommand(command);
        bSuccess &= SendCommand("echo \"" + TASK_COMPLETION_KEY + "\"");
        m_TaskState = bSuccess ? TaskState::Running : TaskState::Error;
        m_TaskId    = taskId;
    }

    void CmdShell::CancelTask()
    {
        m_TaskState = TaskState::Cancelled;
    }

    void CmdShell::Clear()
    {
        m_TaskId = -1;
        m_TaskOutput.clear();
        m_LeftoverCmdOutput.clear();
        m_TaskState = TaskState::Idle;
    }

    CmdShell::TaskState CmdShell::Update(int& taskId)
    {
        taskId = m_TaskId;
        if (m_TaskState == TaskState::Error)
        {
            m_TaskState = TaskState::Idle;
            return TaskState::Error;
        }
        if (m_TaskState == TaskState::Cancelled)
        {
            m_TaskState = TaskState::Idle;
            return TaskState::Cancelled;
        }
        if (m_TaskState == TaskState::Idle)
        {
            return TaskState::Idle;
        }

        bool bDoneReading = false;
        do
        {
            std::string line;
            if (!ReadOutputLine(line))
            {
                m_TaskState = TaskState::Idle;
                return TaskState::Error;
            }
            if (line.empty())
            {
                bDoneReading = true;
            }
            else
            {
                if (line.size() >= 2 && line.at(line.size() - 2) == '\r' &&
                    line.at(line.size() - 1) == '\n')
                {
                    line.pop_back();
                    line.pop_back();
                }
                m_TaskOutput.push_back(line);
            }
        } while (!bDoneReading);

        int iCompletionKey = -1;
        for (std::size_t i = 0; i < m_TaskOutput.size(); ++i)
        {
            if (m_TaskOutput.at(i).find(TASK_COMPLETION_KEY) !=
                std::string::npos)
            {
                iCompletionKey = static_cast<int>(i);
                break;
            }
        }
        if (iCompletionKey != -1)
        {
            m_TaskOutput.resize(iCompletionKey);
            m_TaskState = TaskState::Idle;

            return TaskState::Done;
        }
        return m_TaskState;
    }

    const std::vector<std::string>& CmdShell::PeekTaskOutput()
    {
        return m_TaskOutput;
    }

    bool CmdShell::SendCommand(const std::string& command)
    {
        std::string newlineCommand = command + "\n";
        if (m_hProcess == INVALID_HANDLE_VALUE)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Command process is not running." << log::End();
            return false;
        }
        int         offset        = 0;
        DWORD       nBytesToWrite = static_cast<DWORD>(newlineCommand.size());
        const char* pStr          = newlineCommand.c_str();
        while (nBytesToWrite > 0)
        {
            DWORD nBytesWritten = 0;
            if (!WriteFile(m_hStdinWrite, pStr + offset, nBytesToWrite,
                           &nBytesWritten, NULL))
            {
                log::Error() << SKR_HOTSWAP_LOG_PREFIX
                             << "Failed to write to cmd process. "
                             << log::LastOsError() << log::End();
                return false;
            }
            offset += nBytesWritten;
            nBytesToWrite -= nBytesWritten;
        }
        return true;
    }

    bool CmdShell::ReadOutputLine(std::string& output)
    {
        output.clear();
        if (m_hProcess == INVALID_HANDLE_VALUE)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Command process is not running." << log::End();
            return false;
        }
        std::size_t iNewline = m_LeftoverCmdOutput.find("\n");
        if (iNewline == std::string::npos)
        {
            DWORD nBytesAvailable = 0;
            if (!PeekNamedPipe(m_hStdoutRead, NULL, 0, NULL, &nBytesAvailable,
                               NULL))
            {
                log::Error()
                    << SKR_HOTSWAP_LOG_PREFIX << "Failed to peek cmd pipe. "
                    << log::LastOsError() << log::End();
                return false;
            }
            if (nBytesAvailable > 0)
            {
                DWORD nBytesRead = 0;
                if (!ReadFile(m_hStdoutRead, m_ReadBuffer.data(),
                              static_cast<DWORD>(m_ReadBuffer.size()),
                              &nBytesRead, NULL))
                {
                    log::Error() << SKR_HOTSWAP_LOG_PREFIX
                                 << "Failed to read from cmd process. "
                                 << log::LastOsError() << log::End();
                    return false;
                }
                m_LeftoverCmdOutput +=
                    std::string(m_ReadBuffer.data(), nBytesRead);
            }
        }
        iNewline = m_LeftoverCmdOutput.find("\n");
        if (iNewline != std::string::npos)
        {
            output              = m_LeftoverCmdOutput.substr(0, iNewline + 1);
            m_LeftoverCmdOutput = m_LeftoverCmdOutput.substr(iNewline + 1);
        }
        return true;
    }
} // namespace skr::hotswap
#endif