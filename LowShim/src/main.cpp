#include <yaml-cpp/yaml.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

// Deliberately std + yaml-cpp only, no LowUtil, so the shim stays
// stable across engine versions. The registry format is also written
// by LowCli/src/LowCliEngineRegistry.cpp - keep both in sync.

namespace fs = std::filesystem;

namespace Low {
  namespace Shim {
#ifdef _WIN32
    static const char *g_CliName = "low.exe";
#else
    static const char *g_CliName = "low";
#endif

    static fs::path to_path(const std::string &p_Utf8)
    {
      return fs::path(std::u8string(
          reinterpret_cast<const char8_t *>(p_Utf8.c_str()),
          p_Utf8.size()));
    }

    static std::string to_utf8(const fs::path &p_Path)
    {
      const std::u8string l_Path = p_Path.generic_u8string();
      return std::string(
          reinterpret_cast<const char *>(l_Path.data()),
          l_Path.size());
    }

    static void fail(const std::string &p_Message)
    {
      fprintf(stderr, "low: %s\n", p_Message.c_str());
    }

    static bool load_yaml(const fs::path &p_File, YAML::Node &p_Out)
    {
      std::error_code l_Error;
      if (!fs::exists(p_File, l_Error)) {
        return false;
      }
      try {
        std::ifstream l_Stream(p_File, std::ios::binary);
        if (!l_Stream) {
          return false;
        }
        p_Out = YAML::Load(l_Stream);
        return true;
      } catch (const std::exception &) {
        return false;
      }
    }

    // Scalar() returns the raw text, so engine: 2026.6 stays "2026.6"
    static std::string read_scalar(const YAML::Node &p_Root,
                                   const char *p_Key)
    {
      const YAML::Node l_Value = p_Root[p_Key];
      if (l_Value && l_Value.IsScalar()) {
        return l_Value.Scalar();
      }
      return "";
    }

    static fs::path get_registry_path()
    {
#ifdef _WIN32
      const wchar_t *l_AppData = _wgetenv(L"LOCALAPPDATA");
      if (!l_AppData || !*l_AppData) {
        return {};
      }
      return fs::path(l_AppData) / "LowEngine" / "engines.yaml";
#else
      const char *l_DataHome = getenv("XDG_DATA_HOME");
      if (l_DataHome && *l_DataHome) {
        return fs::path(l_DataHome) / "LowEngine" / "engines.yaml";
      }
      const char *l_Home = getenv("HOME");
      if (!l_Home || !*l_Home) {
        return {};
      }
      return fs::path(l_Home) / ".local/share/LowEngine/engines.yaml";
#endif
    }

    // Empty id resolves the registry's default engine
    static fs::path resolve_registry_id(std::string p_Id)
    {
      YAML::Node l_Root;
      if (!load_yaml(get_registry_path(), l_Root)) {
        fail("no engine registry found, run Setup.bat in an engine");
        return {};
      }
      // Files from before the version key are version 1
      const std::string l_Version = read_scalar(l_Root, "version");
      if (!l_Version.empty() && l_Version != "1") {
        fail("the engine registry has format version " + l_Version +
             ", run Setup.bat of a newer engine to update this shim");
        return {};
      }
      if (p_Id.empty()) {
        p_Id = read_scalar(l_Root, "default");
        if (p_Id.empty()) {
          fail("engine registry has no default engine");
          return {};
        }
      }
      const YAML::Node l_Engines = l_Root["engines"];
      if (!l_Engines || !l_Engines.IsMap() || !l_Engines[p_Id] ||
          !l_Engines[p_Id].IsScalar()) {
        fail("engine '" + p_Id + "' is not registered");
        return {};
      }
      return to_path(l_Engines[p_Id].Scalar());
    }

    static fs::path find_project_root(const fs::path &p_Start)
    {
      std::error_code l_Error;
      fs::path l_Dir = fs::absolute(p_Start, l_Error);
      if (l_Error) {
        return {};
      }
      while (true) {
        if (fs::exists(l_Dir / "project.yaml", l_Error)) {
          return l_Dir;
        }
        if (l_Dir == l_Dir.parent_path()) {
          return {};
        }
        l_Dir = l_Dir.parent_path();
      }
    }

    // Mirrors LowCli's global -p/--project so `low -p ../x ...`
    // resolves the engine of that project, not the cwd's
    static fs::path
    find_project_argument(const std::vector<std::string> &p_Args)
    {
      for (size_t i = 0; i < p_Args.size(); ++i) {
        const std::string &i_Arg = p_Args[i];
        if ((i_Arg == "-p" || i_Arg == "--project") &&
            i + 1 < p_Args.size()) {
          return to_path(p_Args[i + 1]);
        }
        if (i_Arg.rfind("--project=", 0) == 0) {
          return to_path(i_Arg.substr(10));
        }
      }
      std::error_code l_Error;
      return fs::current_path(l_Error);
    }

    static fs::path
    resolve_engine(const std::vector<std::string> &p_Args)
    {
#ifdef _WIN32
      const wchar_t *l_EnvEngine = _wgetenv(L"LOW_ENGINE");
      if (l_EnvEngine && *l_EnvEngine) {
        return fs::path(l_EnvEngine);
      }
#else
      const char *l_EnvEngine = getenv("LOW_ENGINE");
      if (l_EnvEngine && *l_EnvEngine) {
        return fs::path(l_EnvEngine);
      }
#endif

      const fs::path l_Project =
          find_project_root(find_project_argument(p_Args));
      if (l_Project.empty()) {
        return resolve_registry_id("");
      }

      YAML::Node l_User;
      if (load_yaml(l_Project / ".user" / "user.yaml", l_User)) {
        const std::string l_EngineOverride =
            read_scalar(l_User, "engine_override");
        if (!l_EngineOverride.empty()) {
          return to_path(l_EngineOverride);
        }
      }

      YAML::Node l_ProjectConfig;
      if (!load_yaml(l_Project / "project.yaml", l_ProjectConfig)) {
        fail("could not read " + to_utf8(l_Project / "project.yaml"));
        return {};
      }
      const std::string l_Engine =
          read_scalar(l_ProjectConfig, "engine");
      if (l_Engine.empty()) {
        fail("project.yaml has no engine: entry");
        return {};
      }
      if (l_Engine == "bundled") {
        return l_Project / "engine";
      }
      return resolve_registry_id(l_Engine);
    }

    static fs::path get_self_path()
    {
#ifdef _WIN32
      std::wstring l_Buffer(MAX_PATH, L'\0');
      while (true) {
        const DWORD l_Length = GetModuleFileNameW(
            nullptr, l_Buffer.data(), (DWORD)l_Buffer.size());
        if (l_Length == 0) {
          return {};
        }
        if (l_Length < l_Buffer.size()) {
          l_Buffer.resize(l_Length);
          return fs::path(l_Buffer);
        }
        l_Buffer.resize(l_Buffer.size() * 2);
      }
#else
      std::error_code l_Error;
      return fs::read_symlink("/proc/self/exe", l_Error);
#endif
    }

#ifdef _WIN32
    // argv[0] rules: quoted up to the next quote, otherwise up to
    // whitespace, no escapes
    static const wchar_t *
    skip_program_name(const wchar_t *p_CommandLine)
    {
      const wchar_t *l_It = p_CommandLine;
      if (*l_It == L'"') {
        ++l_It;
        while (*l_It && *l_It != L'"') {
          ++l_It;
        }
        if (*l_It) {
          ++l_It;
        }
      } else {
        while (*l_It && *l_It != L' ' && *l_It != L'\t') {
          ++l_It;
        }
      }
      while (*l_It == L' ' || *l_It == L'\t') {
        ++l_It;
      }
      return l_It;
    }

    // A handler function instead of SetConsoleCtrlHandler(nullptr,
    // TRUE): the nullptr variant is inherited and low.exe would
    // ignore Ctrl+C too
    static BOOL WINAPI ignore_ctrl(DWORD p_Type)
    {
      return p_Type == CTRL_C_EVENT || p_Type == CTRL_BREAK_EVENT;
    }

    static int run(const fs::path &p_Exe)
    {
      // Forward the rest of the command line verbatim, so quoting
      // survives exactly as typed
      std::wstring l_CommandLine = L"\"" + p_Exe.wstring() + L"\"";
      const wchar_t *l_Rest = skip_program_name(GetCommandLineW());
      if (*l_Rest) {
        l_CommandLine += L" ";
        l_CommandLine += l_Rest;
      }

      SetConsoleCtrlHandler(ignore_ctrl, TRUE);

      // Kills low.exe (and cmake/node below it) if the shim is killed
      HANDLE l_Job = CreateJobObjectW(nullptr, nullptr);
      if (l_Job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION l_Limits = {};
        l_Limits.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(l_Job,
                                JobObjectExtendedLimitInformation,
                                &l_Limits, sizeof(l_Limits));
      }

      STARTUPINFOW l_Startup = {};
      l_Startup.cb = sizeof(l_Startup);
      PROCESS_INFORMATION l_Process = {};
      if (!CreateProcessW(p_Exe.c_str(), l_CommandLine.data(),
                          nullptr, nullptr, TRUE, CREATE_SUSPENDED,
                          nullptr, nullptr, &l_Startup, &l_Process)) {
        fail("could not start " + to_utf8(p_Exe));
        if (l_Job) {
          CloseHandle(l_Job);
        }
        return 1;
      }
      if (l_Job) {
        AssignProcessToJobObject(l_Job, l_Process.hProcess);
      }
      ResumeThread(l_Process.hThread);

      WaitForSingleObject(l_Process.hProcess, INFINITE);
      DWORD l_ExitCode = 1;
      GetExitCodeProcess(l_Process.hProcess, &l_ExitCode);

      CloseHandle(l_Process.hThread);
      CloseHandle(l_Process.hProcess);
      if (l_Job) {
        CloseHandle(l_Job);
      }
      return static_cast<int>(l_ExitCode);
    }
#else
    static int run(const fs::path &p_Exe, char **p_Argv)
    {
      // exec replaces the shim, so exit code + signals just work
      p_Argv[0] = const_cast<char *>(p_Exe.c_str());
      execv(p_Exe.c_str(), p_Argv);
      fail("could not start " + p_Exe.string());
      return 1;
    }
#endif
  } // namespace Shim
} // namespace Low

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
  using namespace Low::Shim;

  std::vector<std::string> l_Args;
  for (int i = 1; i < argc; ++i) {
    l_Args.push_back(to_utf8(fs::path(argv[i])));
  }

  const fs::path l_Engine = resolve_engine(l_Args);
  if (l_Engine.empty()) {
    return 1;
  }

  const fs::path l_Exe = l_Engine / "bin" / g_CliName;
  std::error_code l_Error;
  if (!fs::exists(l_Exe, l_Error)) {
    fail("engine at " + to_utf8(l_Engine) + " has no bin/" +
         g_CliName + ", run Setup.bat there");
    return 1;
  }
  if (fs::equivalent(l_Exe, get_self_path(), l_Error)) {
    fail("resolved engine points back at the shim itself");
    return 1;
  }

#ifdef _WIN32
  return run(l_Exe);
#else
  return run(l_Exe, argv);
#endif
}
