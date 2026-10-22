#include "LowCliProcess.h"

#include "LowCliEvents.h"
#include "LowCliOutput.h"

#include <EASTL/algorithm.h>

#include <mutex>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace Low {
  namespace Cli {
    namespace Process {
      class LineSplitter
      {
      public:
        explicit LineSplitter(
            const Util::Function<void(Util::StringView)> &p_Callback)
            : m_Callback(p_Callback)
        {
        }

        void feed(const char *p_Data, size_t p_Size)
        {
          m_Buffer.append(p_Data, p_Size);

          size_t l_Start = 0;
          size_t l_End = m_Buffer.find('\n', l_Start);
          while (l_End != Util::String::npos) {
            emit_line(l_Start, l_End);
            l_Start = l_End + 1;
            l_End = m_Buffer.find('\n', l_Start);
          }
          m_Buffer.erase(0, l_Start);
        }

        void flush()
        {
          if (!m_Buffer.empty()) {
            emit_line(0, m_Buffer.size());
            m_Buffer.clear();
          }
        }

      private:
        void emit_line(size_t p_Start, size_t p_End)
        {
          if (p_End > p_Start && m_Buffer[p_End - 1] == '\r') {
            --p_End;
          }
          if (m_Callback) {
            m_Callback(Util::StringView(m_Buffer.data() + p_Start,
                                        p_End - p_Start));
          }
        }

        const Util::Function<void(Util::StringView)> &m_Callback;
        Util::String m_Buffer;
      };

#ifdef _WIN32
      HANDLE g_Job = nullptr;
      std::mutex g_JobMutex;

      static std::wstring to_wide(Util::StringView p_Text)
      {
        if (p_Text.empty()) {
          return std::wstring();
        }
        const int l_Size = MultiByteToWideChar(
            CP_UTF8, 0, p_Text.data(), static_cast<int>(p_Text.size()),
            nullptr, 0);
        std::wstring l_Result(l_Size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, p_Text.data(),
                            static_cast<int>(p_Text.size()),
                            l_Result.data(), l_Size);
        return l_Result;
      }

      // Quoting rules of CommandLineToArgvW / the MSVC runtime
      static void append_quoted(std::wstring &p_CommandLine,
                                const std::wstring &p_Argument)
      {
        if (!p_Argument.empty() &&
            p_Argument.find_first_of(L" \t\"") == std::wstring::npos) {
          p_CommandLine += p_Argument;
          return;
        }

        p_CommandLine += L'"';
        size_t l_Backslashes = 0;
        for (wchar_t i_Char : p_Argument) {
          if (i_Char == L'\\') {
            ++l_Backslashes;
            continue;
          }
          if (i_Char == L'"') {
            p_CommandLine.append(l_Backslashes * 2 + 1, L'\\');
          } else {
            p_CommandLine.append(l_Backslashes, L'\\');
          }
          l_Backslashes = 0;
          p_CommandLine += i_Char;
        }
        p_CommandLine.append(l_Backslashes * 2, L'\\');
        p_CommandLine += L'"';
      }

      static HANDLE get_job()
      {
        std::lock_guard<std::mutex> l_Lock(g_JobMutex);
        if (!g_Job) {
          g_Job = CreateJobObjectW(nullptr, nullptr);
          JOBOBJECT_EXTENDED_LIMIT_INFORMATION l_Info = {};
          l_Info.BasicLimitInformation.LimitFlags =
              JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
          SetInformationJobObject(g_Job,
                                  JobObjectExtendedLimitInformation,
                                  &l_Info, sizeof(l_Info));
        }
        return g_Job;
      }

      static void read_pipe(HANDLE p_Pipe,
                            const Util::Function<void(Util::StringView)>
                                &p_Callback)
      {
        LineSplitter l_Splitter(p_Callback);
        char l_Buffer[4096];
        DWORD l_Read = 0;
        while (ReadFile(p_Pipe, l_Buffer, sizeof(l_Buffer), &l_Read,
                        nullptr) &&
               l_Read > 0) {
          l_Splitter.feed(l_Buffer, l_Read);
        }
        l_Splitter.flush();
      }

      int run(const Options &p_Options)
      {
        std::wstring l_CommandLine;
        append_quoted(l_CommandLine, to_wide(p_Options.executable));
        for (const Util::String &i_Argument : p_Options.arguments) {
          l_CommandLine += L' ';
          append_quoted(l_CommandLine, to_wide(i_Argument));
        }

        SECURITY_ATTRIBUTES l_Security = {};
        l_Security.nLength = sizeof(l_Security);
        l_Security.bInheritHandle = TRUE;

        HANDLE l_OutRead = nullptr;
        HANDLE l_OutWrite = nullptr;
        HANDLE l_ErrRead = nullptr;
        HANDLE l_ErrWrite = nullptr;
        CreatePipe(&l_OutRead, &l_OutWrite, &l_Security, 0);
        CreatePipe(&l_ErrRead, &l_ErrWrite, &l_Security, 0);
        SetHandleInformation(l_OutRead, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(l_ErrRead, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW l_Startup = {};
        l_Startup.cb = sizeof(l_Startup);
        l_Startup.dwFlags = STARTF_USESTDHANDLES;
        l_Startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        l_Startup.hStdOutput = l_OutWrite;
        l_Startup.hStdError = l_ErrWrite;

        const std::wstring l_WorkingDirectory =
            to_wide(p_Options.workingDirectory);

        PROCESS_INFORMATION l_Process = {};
        const BOOL l_Started = CreateProcessW(
            nullptr, l_CommandLine.data(), nullptr, nullptr, TRUE,
            CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT, nullptr,
            l_WorkingDirectory.empty() ? nullptr
                                       : l_WorkingDirectory.c_str(),
            &l_Startup, &l_Process);

        // Our copies of the write ends have to be closed, otherwise the
        // reads below never see the end of the stream
        CloseHandle(l_OutWrite);
        CloseHandle(l_ErrWrite);

        if (!l_Started) {
          CloseHandle(l_OutRead);
          CloseHandle(l_ErrRead);
          return -1;
        }

        AssignProcessToJobObject(get_job(), l_Process.hProcess);
        ResumeThread(l_Process.hThread);
        CloseHandle(l_Process.hThread);

        std::thread l_ErrThread(
            [&]() { read_pipe(l_ErrRead, p_Options.onStderrLine); });
        read_pipe(l_OutRead, p_Options.onStdoutLine);
        l_ErrThread.join();

        CloseHandle(l_OutRead);
        CloseHandle(l_ErrRead);

        WaitForSingleObject(l_Process.hProcess, INFINITE);
        DWORD l_ExitCode = 0;
        GetExitCodeProcess(l_Process.hProcess, &l_ExitCode);
        CloseHandle(l_Process.hProcess);

        return static_cast<int>(l_ExitCode);
      }

      void set_environment_variable(Util::StringView p_Name,
                                    Util::StringView p_Value)
      {
        SetEnvironmentVariableW(to_wide(p_Name).c_str(),
                                to_wide(p_Value).c_str());
      }

      void kill_all()
      {
        std::lock_guard<std::mutex> l_Lock(g_JobMutex);
        if (g_Job) {
          TerminateJobObject(g_Job, 130);
        }
      }
#else
      Util::List<pid_t> g_Children;
      std::mutex g_ChildrenMutex;

      int run(const Options &p_Options)
      {
        int l_OutPipe[2];
        int l_ErrPipe[2];
        if (pipe(l_OutPipe) != 0 || pipe(l_ErrPipe) != 0) {
          return -1;
        }

        const pid_t l_Pid = fork();
        if (l_Pid < 0) {
          return -1;
        }

        if (l_Pid == 0) {
          // Own process group so kill_all() can take down grandchildren
          setpgid(0, 0);
          dup2(l_OutPipe[1], STDOUT_FILENO);
          dup2(l_ErrPipe[1], STDERR_FILENO);
          close(l_OutPipe[0]);
          close(l_OutPipe[1]);
          close(l_ErrPipe[0]);
          close(l_ErrPipe[1]);

          if (!p_Options.workingDirectory.empty() &&
              chdir(p_Options.workingDirectory.c_str()) != 0) {
            _exit(127);
          }

          Util::List<char *> l_Argv;
          l_Argv.push_back(const_cast<char *>(p_Options.executable.c_str()));
          for (const Util::String &i_Argument : p_Options.arguments) {
            l_Argv.push_back(const_cast<char *>(i_Argument.c_str()));
          }
          l_Argv.push_back(nullptr);

          execvp(p_Options.executable.c_str(), l_Argv.data());
          _exit(127);
        }

        {
          std::lock_guard<std::mutex> l_Lock(g_ChildrenMutex);
          g_Children.push_back(l_Pid);
        }

        close(l_OutPipe[1]);
        close(l_ErrPipe[1]);

        LineSplitter l_OutSplitter(p_Options.onStdoutLine);
        LineSplitter l_ErrSplitter(p_Options.onStderrLine);

        pollfd l_Fds[2] = {{l_OutPipe[0], POLLIN, 0},
                           {l_ErrPipe[0], POLLIN, 0}};
        int l_Open = 2;
        char l_Buffer[4096];
        while (l_Open > 0) {
          if (poll(l_Fds, 2, -1) < 0) {
            if (errno == EINTR) {
              continue;
            }
            break;
          }
          for (int i = 0; i < 2; ++i) {
            if (l_Fds[i].fd < 0 || l_Fds[i].revents == 0) {
              continue;
            }
            const ssize_t l_Read = read(l_Fds[i].fd, l_Buffer,
                                        sizeof(l_Buffer));
            if (l_Read > 0) {
              (i == 0 ? l_OutSplitter : l_ErrSplitter)
                  .feed(l_Buffer, static_cast<size_t>(l_Read));
            } else {
              close(l_Fds[i].fd);
              l_Fds[i].fd = -1;
              --l_Open;
            }
          }
        }
        l_OutSplitter.flush();
        l_ErrSplitter.flush();

        int l_Status = 0;
        waitpid(l_Pid, &l_Status, 0);

        {
          std::lock_guard<std::mutex> l_Lock(g_ChildrenMutex);
          g_Children.erase(
              eastl::remove(g_Children.begin(), g_Children.end(), l_Pid),
              g_Children.end());
        }

        if (WIFEXITED(l_Status)) {
          return WEXITSTATUS(l_Status);
        }
        return 128 + WTERMSIG(l_Status);
      }

      void set_environment_variable(Util::StringView p_Name,
                                    Util::StringView p_Value)
      {
        setenv(Util::String(p_Name.data(), p_Name.size()).c_str(),
               Util::String(p_Value.data(), p_Value.size()).c_str(), 1);
      }

      void kill_all()
      {
        // No lock: may run inside a signal handler
        for (pid_t i_Pid : g_Children) {
          kill(-i_Pid, SIGTERM);
        }
      }
#endif

      EventRunResult run_with_events(const Options &p_Options)
      {
        EventRunResult l_Result;

        Options l_Options = p_Options;
        l_Options.onStdoutLine = [](Util::StringView p_Line) {
          Event l_Event;
          if (Events::parse(p_Line, l_Event)) {
            Output::emit(l_Event);
          } else if (!p_Line.empty()) {
            Output::detail(p_Line);
          }
        };
        l_Options.onStderrLine = [&l_Result](Util::StringView p_Line) {
          l_Result.stderrLines.push_back(
              Util::String(p_Line.data(), p_Line.size()));
          Output::debug(p_Line);
        };

        l_Result.exitCode = run(l_Options);
        return l_Result;
      }
    } // namespace Process
  } // namespace Cli
} // namespace Low
