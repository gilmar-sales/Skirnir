#ifndef WIN32
    #include <fcntl.h>
    #include <poll.h>
    #include <signal.h>
    #include <sys/types.h>
    #include <unistd.h>

    #include <cassert>
    #include <cstdio>

    #include "Skirnir/Hotswap/cmd-shell/CmdShell_unix.hpp"

namespace skr::hotswap
{
    namespace
    {
        const std::string TASK_COMPLETION_KEY =
            "\"__skr_task_complete(c4c2c81b-0a1f-4ad7-bd50-b6e8e2a0d5ee)__\"";
    } // namespace

    CmdShell::~CmdShell()
    {
        if (m_ShellPid != -1)
        {
            if (kill(m_ShellPid, SIGKILL) == -1)
            {
                log::Warning() << SKR_HOTSWAP_LOG_PREFIX
                               << "Failed to terminate CmdShell process. "
                               << log::LastOsError() << log::End();
            }
        }
    }

    bool CmdShell::CreateCmdProcess()
    {
        if (pipe(m_ReadPipe) == -1)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to create read pipe." << log::End();
            return false;
        }
        if (pipe(m_WritePipe) == -1)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to create read pipe." << log::End();
            return false;
        }
        m_ShellPid = fork();
        if (m_ShellPid == -1)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to fork subprocess." << log::End();
            return false;
        }
        if (m_ShellPid == 0)
        {
            dup2(m_WritePipe[0], STDIN_FILENO);
            dup2(m_ReadPipe[1], STDOUT_FILENO);
            dup2(m_ReadPipe[1], STDERR_FILENO);
            execl("/bin/sh", "sh", nullptr);
            exit(1);
        }
        close(m_WritePipe[0]);
        close(m_ReadPipe[1]);
        return true;
    }

    void CmdShell::StartTask(const std::string& command, int taskId)
    {
        Clear();
        bool bSuccess = SendCommand(command);
        bSuccess &= SendCommand("echo '\n" + TASK_COMPLETION_KEY + "'");
        m_TaskState = bSuccess ? TaskState::Running : TaskState::Error;
        m_TaskId    = taskId;
        m_TaskOutput.clear();
        m_LeftoverCmdOutput.clear();
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

    ICmdShell::TaskState CmdShell::Update(int& taskId)
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
                if (!line.empty() && line.back() == '\n')
                {
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
        if (m_ShellPid == -1)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "CmdShell process is not running." << log::End();
            return false;
        }
        ssize_t     offset        = 0;
        ssize_t     nBytesToWrite = static_cast<ssize_t>(newlineCommand.size());
        const char* pStr          = newlineCommand.c_str();
        while (nBytesToWrite > 0)
        {
            ssize_t nBytesWritten =
                write(m_WritePipe[1], pStr + offset, nBytesToWrite);
            if (nBytesWritten == -1)
            {
                log::Error() << SKR_HOTSWAP_LOG_PREFIX
                             << "Failed to write to CmdShell process. "
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
        std::size_t iNewline = m_LeftoverCmdOutput.find("\n");
        if (iNewline == std::string::npos)
        {
            const int     nFds = 1;
            struct pollfd fds[nFds];
            fds->fd     = m_ReadPipe[0];
            fds->events = POLLIN;
            int ret     = poll(fds, nFds, 0);
            if (ret > 0)
            {
                ssize_t nBytesRead =
                    read(fds[0].fd, m_ReadBuffer.data(), m_ReadBuffer.size());
                if (nBytesRead == -1)
                {
                    log::Error() << SKR_HOTSWAP_LOG_PREFIX
                                 << "Failed to read from CmdShell process. "
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