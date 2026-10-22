#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace Low {
  namespace Launcher {
    static void show_error(const std::wstring &p_Message)
    {
      MessageBoxW(nullptr, p_Message.c_str(), L"LowEngine",
                  MB_ICONERROR | MB_OK);
    }

    static fs::path get_self_path()
    {
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
    }

    // Only bundled projects for now. Registered engines and
    // engine_override (see LowShim) slot in here later
    static fs::path resolve_engine(const fs::path &p_ProjectRoot)
    {
      return p_ProjectRoot / "engine";
    }

    // A clone without git-lfs leaves small text files in place of
    // the binaries
    static bool is_lfs_pointer(const fs::path &p_File)
    {
      std::error_code l_Error;
      const uintmax_t l_Size = fs::file_size(p_File, l_Error);
      if (l_Error || l_Size > 1024) {
        return false;
      }

      static const char *l_Signature = "version https://git-lfs";
      const size_t l_SignatureLength = strlen(l_Signature);

      std::ifstream l_Stream(p_File, std::ios::binary);
      std::string l_Head(l_SignatureLength, '\0');
      l_Stream.read(l_Head.data(), l_SignatureLength);
      return l_Stream &&
             l_Head.compare(0, l_SignatureLength, l_Signature) == 0;
    }

    // argv[0] rules: quoted up to the next quote, otherwise up to
    // whitespace, no escapes. Same as in LowShim
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

    static bool start(const fs::path &p_Exe,
                      const fs::path &p_WorkingDirectory)
    {
      // Forward the rest of the command line verbatim, so quoting
      // survives exactly as typed
      std::wstring l_CommandLine = L"\"" + p_Exe.wstring() + L"\"";
      const wchar_t *l_Rest = skip_program_name(GetCommandLineW());
      if (*l_Rest) {
        l_CommandLine += L" ";
        l_CommandLine += l_Rest;
      }

      STARTUPINFOW l_Startup = {};
      l_Startup.cb = sizeof(l_Startup);
      PROCESS_INFORMATION l_Process = {};
      if (!CreateProcessW(p_Exe.c_str(), l_CommandLine.data(),
                          nullptr, nullptr, FALSE, 0, nullptr,
                          p_WorkingDirectory.c_str(), &l_Startup,
                          &l_Process)) {
        return false;
      }

      // Lowder runs on its own, the launcher is done
      CloseHandle(l_Process.hThread);
      CloseHandle(l_Process.hProcess);
      return true;
    }
  } // namespace Launcher
} // namespace Low

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
  using namespace Low::Launcher;

  const fs::path l_SelfPath = get_self_path();
  if (l_SelfPath.empty()) {
    show_error(L"Could not determine the location of the launcher.");
    return 1;
  }

  const fs::path l_ProjectRoot = l_SelfPath.parent_path();
  const fs::path l_Engine = resolve_engine(l_ProjectRoot);
  const fs::path l_Lowder = l_Engine / "bin" / "lowder.exe";

  std::error_code l_Error;
  if (!fs::exists(l_Lowder, l_Error)) {
    show_error(L"Could not find the engine:\n" + l_Lowder.wstring() +
               L"\n\nMake sure the project was pulled completely.");
    return 1;
  }
  if (is_lfs_pointer(l_Lowder)) {
    show_error(L"The engine binaries are Git LFS placeholders.\n\n"
               L"Install Git LFS and run 'git lfs pull' in the "
               L"project folder.");
    return 1;
  }

  if (!start(l_Lowder, l_ProjectRoot)) {
    show_error(L"Could not start " + l_Lowder.wstring() + L".");
    return 1;
  }

  return 0;
}
