#include "LowderCrashReporter.h"

#include "LowUtilVersion.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <DbgHelp.h>
#include <gdiplus.h>
#include <winhttp.h>
#endif

#ifndef LOW_CRASH_REPORT_UPLOAD_URL
#define LOW_CRASH_REPORT_UPLOAD_URL ""
#endif

#ifndef LOW_CRASH_REPORT_UPLOAD_TOKEN
#define LOW_CRASH_REPORT_UPLOAD_TOKEN ""
#endif

namespace Lowder {
  namespace CrashReporter {
    namespace {
      std::string g_ProjectPath = ".";

      static std::string normalize_path(const std::string &p_Path)
      {
#ifdef _WIN32
        char l_Buffer[MAX_PATH];
        DWORD l_Result = GetFullPathNameA(p_Path.c_str(), MAX_PATH,
                                          l_Buffer, nullptr);
        if (l_Result == 0 || l_Result >= MAX_PATH) {
          return p_Path;
        }
        return l_Buffer;
#else
        return p_Path;
#endif
      }

#ifdef _WIN32
      LPTOP_LEVEL_EXCEPTION_FILTER g_PreviousExceptionFilter =
          nullptr;

      const int ID_ACTIONS_EDIT = 1001;
      const int ID_SAVE_BUTTON = 1002;
      const int ID_SKIP_BUTTON = 1003;
      const int ID_NAME_EDIT = 1004;

      struct ReporterWindowState
      {
        std::string reportDirectory;
        std::string logoPath;
        std::string logoTextPath;
        HWND nameControl = nullptr;
        HWND editControl = nullptr;
        HFONT titleFont = nullptr;
        HFONT bodyFont = nullptr;
        HICON windowIcon = nullptr;
        Gdiplus::Image *logoImage = nullptr;
        Gdiplus::Image *logoTextImage = nullptr;
        HBRUSH backgroundBrush = nullptr;
        HBRUSH editBrush = nullptr;
        bool saved = false;
        bool uploaded = false;
      };

      ULONG_PTR g_GdiplusToken = 0;

      static std::string get_last_error_string()
      {
        DWORD l_Error = GetLastError();
        if (!l_Error) {
          return "";
        }

        LPVOID l_Buffer = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, l_Error,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPSTR>(&l_Buffer), 0,
                       nullptr);

        std::string l_Message = l_Buffer
                                    ? static_cast<char *>(l_Buffer)
                                    : "unknown error";
        if (l_Buffer) {
          LocalFree(l_Buffer);
        }
        return l_Message;
      }

      static bool ensure_directory(const std::string &p_Path)
      {
        if (CreateDirectoryA(p_Path.c_str(), nullptr)) {
          return true;
        }

        return GetLastError() == ERROR_ALREADY_EXISTS;
      }

      static std::string join_path(const std::string &p_Left,
                                   const std::string &p_Right)
      {
        if (p_Left.empty()) {
          return p_Right;
        }

        const char l_Last = p_Left[p_Left.size() - 1];
        if (l_Last == '\\' || l_Last == '/') {
          return p_Left + p_Right;
        }

        return p_Left + "/" + p_Right;
      }

      static bool file_exists(const std::string &p_Path)
      {
        DWORD l_Attributes = GetFileAttributesA(p_Path.c_str());
        return l_Attributes != INVALID_FILE_ATTRIBUTES &&
               (l_Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
      }

      static std::wstring widen_path(const std::string &p_Path)
      {
        const int l_Size = MultiByteToWideChar(
            CP_UTF8, 0, p_Path.c_str(), -1, nullptr, 0);
        if (l_Size <= 0) {
          return std::wstring(p_Path.begin(), p_Path.end());
        }

        std::wstring l_Result;
        l_Result.resize(static_cast<size_t>(l_Size));
        MultiByteToWideChar(CP_UTF8, 0, p_Path.c_str(), -1,
                            l_Result.data(), l_Size);
        if (!l_Result.empty() && l_Result.back() == L'\0') {
          l_Result.pop_back();
        }
        return l_Result;
      }

      static std::string find_logo_path()
      {
        const std::string l_Candidates[] = {
            join_path(g_ProjectPath,
                      "data/.editor_images/lowlogo_90.png"),
            join_path(g_ProjectPath,
                      "data/.editor_images/lowlogo_36.png")};

        for (const std::string &i_Path : l_Candidates) {
          if (file_exists(i_Path)) {
            return i_Path;
          }
        }

        return "";
      }

      static std::string find_logo_text_path()
      {
        const std::string l_Path = join_path(
            g_ProjectPath, "data/.editor_images/lowfont_500.png");
        return file_exists(l_Path) ? l_Path : "";
      }

      static HICON load_window_icon()
      {
        HICON l_EmbeddedIcon = static_cast<HICON>(LoadImageA(
            GetModuleHandleA(nullptr), MAKEINTRESOURCEA(1), IMAGE_ICON,
            32, 32, LR_DEFAULTCOLOR));
        if (l_EmbeddedIcon) {
          return l_EmbeddedIcon;
        }

        const std::string l_AppIconPath =
            join_path(g_ProjectPath, "app.ico");
        if (file_exists(l_AppIconPath)) {
          return static_cast<HICON>(
              LoadImageA(nullptr, l_AppIconPath.c_str(), IMAGE_ICON,
                         32, 32, LR_LOADFROMFILE | LR_DEFAULTCOLOR));
        }

        return LoadIcon(nullptr, IDI_ERROR);
      }

      static std::string make_report_directory()
      {
        ensure_directory(g_ProjectPath);

        const std::string l_CrashRoot =
            join_path(g_ProjectPath, "crash_reports");
        ensure_directory(l_CrashRoot);

        SYSTEMTIME l_Time;
        GetLocalTime(&l_Time);

        char l_Name[128];
        std::snprintf(
            l_Name, sizeof(l_Name), "%04u%02u%02u_%02u%02u%02u_%lu",
            l_Time.wYear, l_Time.wMonth, l_Time.wDay, l_Time.wHour,
            l_Time.wMinute, l_Time.wSecond,
            static_cast<unsigned long>(GetCurrentProcessId()));

        const std::string l_ReportDirectory =
            join_path(l_CrashRoot, l_Name);
        ensure_directory(l_ReportDirectory);
        return l_ReportDirectory;
      }

      static void write_text_file(const std::string &p_Path,
                                  const std::string &p_Text)
      {
        FILE *l_File = nullptr;
        fopen_s(&l_File, p_Path.c_str(), "wb");
        if (!l_File) {
          return;
        }

        fwrite(p_Text.data(), 1, p_Text.size(), l_File);
        fclose(l_File);
      }

      static void append_text_file(const std::string &p_Path,
                                   const std::string &p_Text)
      {
        FILE *l_File = nullptr;
        fopen_s(&l_File, p_Path.c_str(), "ab");
        if (!l_File) {
          return;
        }

        fwrite(p_Text.data(), 1, p_Text.size(), l_File);
        fclose(l_File);
      }

      static void copy_file_if_exists(const std::string &p_Source,
                                      const std::string &p_Target)
      {
        CopyFileA(p_Source.c_str(), p_Target.c_str(), FALSE);
      }

      static void delete_file_if_exists(const std::string &p_Path)
      {
        DeleteFileA(p_Path.c_str());
      }

      static void
      delete_report_directory(const std::string &p_ReportDirectory)
      {
        delete_file_if_exists(
            join_path(p_ReportDirectory, "crash.txt"));
        delete_file_if_exists(
            join_path(p_ReportDirectory, "minidump.dmp"));
        delete_file_if_exists(
            join_path(p_ReportDirectory, "low.log"));
        delete_file_if_exists(
            join_path(p_ReportDirectory, "lowerr.log"));
        delete_file_if_exists(
            join_path(p_ReportDirectory, "release_manifest.json"));
        delete_file_if_exists(
            join_path(p_ReportDirectory, "user_name.txt"));
        delete_file_if_exists(join_path(
            p_ReportDirectory, "last_actions_before_crash.txt"));
        RemoveDirectoryA(p_ReportDirectory.c_str());
      }

      static void
      write_minidump(const std::string &p_ReportDirectory,
                     EXCEPTION_POINTERS *p_ExceptionPointers)
      {
        const std::string l_DumpPath =
            join_path(p_ReportDirectory, "minidump.dmp");

        HANDLE l_File = CreateFileA(l_DumpPath.c_str(), GENERIC_WRITE,
                                    0, nullptr, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);

        if (l_File == INVALID_HANDLE_VALUE) {
          append_text_file(join_path(p_ReportDirectory, "crash.txt"),
                           "Failed to create minidump file: " +
                               get_last_error_string() + "\r\n");
          return;
        }

        MINIDUMP_EXCEPTION_INFORMATION l_ExceptionInfo;
        l_ExceptionInfo.ThreadId = GetCurrentThreadId();
        l_ExceptionInfo.ExceptionPointers = p_ExceptionPointers;
        l_ExceptionInfo.ClientPointers = FALSE;

        const BOOL l_Success = MiniDumpWriteDump(
            GetCurrentProcess(), GetCurrentProcessId(), l_File,
            MiniDumpWithIndirectlyReferencedMemory,
            p_ExceptionPointers ? &l_ExceptionInfo : nullptr, nullptr,
            nullptr);

        CloseHandle(l_File);

        if (!l_Success) {
          append_text_file(join_path(p_ReportDirectory, "crash.txt"),
                           "Failed to write minidump: " +
                               get_last_error_string() + "\r\n");
        }
      }

      static std::string
      capture_callstack(EXCEPTION_POINTERS *p_ExceptionPointers)
      {
        if (!p_ExceptionPointers ||
            !p_ExceptionPointers->ContextRecord) {
          return "Callstack unavailable";
        }

        HANDLE l_Process = GetCurrentProcess();
        HANDLE l_Thread = GetCurrentThread();

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        SymInitialize(l_Process, nullptr, TRUE);

        CONTEXT l_Context = *p_ExceptionPointers->ContextRecord;
        STACKFRAME64 l_Frame = {};
        DWORD l_MachineType = 0;

#if defined(_M_X64)
        l_MachineType = IMAGE_FILE_MACHINE_AMD64;
        l_Frame.AddrPC.Offset = l_Context.Rip;
        l_Frame.AddrFrame.Offset = l_Context.Rbp;
        l_Frame.AddrStack.Offset = l_Context.Rsp;
#elif defined(_M_IX86)
        l_MachineType = IMAGE_FILE_MACHINE_I386;
        l_Frame.AddrPC.Offset = l_Context.Eip;
        l_Frame.AddrFrame.Offset = l_Context.Ebp;
        l_Frame.AddrStack.Offset = l_Context.Esp;
#else
        return "Callstack unsupported on this architecture";
#endif

        l_Frame.AddrPC.Mode = AddrModeFlat;
        l_Frame.AddrFrame.Mode = AddrModeFlat;
        l_Frame.AddrStack.Mode = AddrModeFlat;

        std::ostringstream l_Stream;

        for (int i = 0; i < 48; ++i) {
          if (!StackWalk64(l_MachineType, l_Process, l_Thread,
                           &l_Frame, &l_Context, nullptr,
                           SymFunctionTableAccess64,
                           SymGetModuleBase64, nullptr)) {
            break;
          }

          if (l_Frame.AddrPC.Offset == 0) {
            break;
          }

          DWORD64 l_ModuleBase =
              SymGetModuleBase64(l_Process, l_Frame.AddrPC.Offset);
          char l_ModuleName[MAX_PATH] = {};
          if (l_ModuleBase != 0) {
            GetModuleFileNameA(
                reinterpret_cast<HMODULE>(l_ModuleBase), l_ModuleName,
                MAX_PATH);
          }

          char l_SymbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] =
              {};
          SYMBOL_INFO *l_Symbol =
              reinterpret_cast<SYMBOL_INFO *>(l_SymbolBuffer);
          l_Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
          l_Symbol->MaxNameLen = MAX_SYM_NAME;
          DWORD64 l_Displacement = 0;

          l_Stream << "#" << i << " ";
          if (SymFromAddr(l_Process, l_Frame.AddrPC.Offset,
                          &l_Displacement, l_Symbol)) {
            l_Stream << l_Symbol->Name << "+0x" << std::hex
                     << l_Displacement << std::dec;
          } else {
            l_Stream << "0x" << std::hex << l_Frame.AddrPC.Offset
                     << std::dec;
          }

          if (l_ModuleName[0] != '\0') {
            std::string l_Module = l_ModuleName;
            const size_t l_Slash = l_Module.find_last_of("\\/");
            if (l_Slash != std::string::npos) {
              l_Module = l_Module.substr(l_Slash + 1u);
            }
            l_Stream << " (" << l_Module << ")";
          }
          l_Stream << "\r\n";
        }

        SymCleanup(l_Process);
        return l_Stream.str();
      }

      static std::string
      format_exception_info(EXCEPTION_POINTERS *p_ExceptionPointers)
      {
        std::string l_Text;
        l_Text += "LowEngine crash report\r\n";
        l_Text += "Version: ";
        l_Text += LOW_VERSION_YEAR;
        l_Text += ".";
        l_Text += LOW_VERSION_MAJOR;
        l_Text += ".";
        l_Text += LOW_VERSION_MINOR;
        l_Text += "\r\n";
        l_Text += "Project path: " + g_ProjectPath + "\r\n";

        if (p_ExceptionPointers &&
            p_ExceptionPointers->ExceptionRecord) {
          char l_Buffer[256];
          std::snprintf(
              l_Buffer, sizeof(l_Buffer),
              "Exception code: 0x%08lX\r\nException address: "
              "0x%p\r\n",
              static_cast<unsigned long>(
                  p_ExceptionPointers->ExceptionRecord
                      ->ExceptionCode),
              p_ExceptionPointers->ExceptionRecord->ExceptionAddress);
          l_Text += l_Buffer;
        } else {
          l_Text += "Exception code: unavailable\r\n";
          l_Text += "Exception address: unavailable\r\n";
        }

        l_Text += "Callstack:\r\n";
        l_Text += capture_callstack(p_ExceptionPointers);
        l_Text += "End callstack\r\n";

        return l_Text;
      }

      static std::string read_edit_control(HWND p_EditControl);

      static void
      write_user_actions(const std::string &p_ReportDirectory,
                         HWND p_EditControl)
      {
        write_text_file(join_path(p_ReportDirectory,
                                  "last_actions_before_crash.txt"),
                        read_edit_control(p_EditControl));
      }

      static std::string read_text_file(const std::string &p_Path)
      {
        std::ifstream l_File(p_Path, std::ios::binary);
        if (!l_File) {
          return "";
        }

        std::ostringstream l_Stream;
        l_Stream << l_File.rdbuf();
        return l_Stream.str();
      }

      static std::string trim_saved_text(std::string p_Text)
      {
        while (!p_Text.empty() &&
               (p_Text.back() == '\r' || p_Text.back() == '\n' ||
                p_Text.back() == '\t' || p_Text.back() == ' ')) {
          p_Text.pop_back();
        }

        while (!p_Text.empty() &&
               (p_Text.front() == '\r' || p_Text.front() == '\n' ||
                p_Text.front() == '\t' || p_Text.front() == ' ')) {
          p_Text.erase(p_Text.begin());
        }

        return p_Text;
      }

      static std::string read_edit_control(HWND p_EditControl)
      {
        if (!p_EditControl) {
          return "";
        }

        const int l_Length = GetWindowTextLengthA(p_EditControl);
        std::string l_Text;
        l_Text.resize(static_cast<size_t>(l_Length) + 1u);
        GetWindowTextA(p_EditControl, l_Text.data(), l_Length + 1);
        l_Text.resize(static_cast<size_t>(l_Length));
        return l_Text;
      }

      static std::string get_saved_user_name_path()
      {
        return join_path(g_ProjectPath, "crash_reporter_user.txt");
      }

      static std::string load_saved_user_name()
      {
        return trim_saved_text(
            read_text_file(get_saved_user_name_path()));
      }

      static void
      write_user_name(const std::string &p_ReportDirectory,
                      HWND p_EditControl)
      {
        const std::string l_UserName =
            trim_saved_text(read_edit_control(p_EditControl));
        write_text_file(join_path(p_ReportDirectory, "user_name.txt"),
                        l_UserName);
        write_text_file(get_saved_user_name_path(), l_UserName);
      }

      static bool read_binary_file(const std::string &p_Path,
                                   std::vector<char> &p_Out)
      {
        std::ifstream l_File(p_Path,
                             std::ios::binary | std::ios::ate);
        if (!l_File) {
          return false;
        }

        const std::streamsize l_Size = l_File.tellg();
        l_File.seekg(0, std::ios::beg);
        p_Out.resize(static_cast<size_t>(l_Size));
        if (l_Size > 0 && !l_File.read(p_Out.data(), l_Size)) {
          return false;
        }

        return true;
      }

      static std::string
      get_json_string_value(const std::string &p_Text,
                            const char *p_Key)
      {
        const std::string l_Key = std::string("\"") + p_Key + "\"";
        size_t l_Position = p_Text.find(l_Key);
        if (l_Position == std::string::npos) {
          return "";
        }

        l_Position = p_Text.find(':', l_Position + l_Key.size());
        if (l_Position == std::string::npos) {
          return "";
        }

        l_Position = p_Text.find('"', l_Position);
        if (l_Position == std::string::npos) {
          return "";
        }

        const size_t l_Start = l_Position + 1u;
        const size_t l_End = p_Text.find('"', l_Start);
        if (l_End == std::string::npos) {
          return "";
        }

        return p_Text.substr(l_Start, l_End - l_Start);
      }

      static std::string
      get_crash_text_value(const std::string &p_Text,
                           const char *p_Key)
      {
        const std::string l_Key = std::string(p_Key) + ": ";
        size_t l_Position = p_Text.find(l_Key);
        if (l_Position == std::string::npos) {
          return "";
        }

        const size_t l_Start = l_Position + l_Key.size();
        size_t l_End = p_Text.find('\r', l_Start);
        if (l_End == std::string::npos) {
          l_End = p_Text.find('\n', l_Start);
        }
        if (l_End == std::string::npos) {
          l_End = p_Text.size();
        }

        return p_Text.substr(l_Start, l_End - l_Start);
      }

      static std::string
      get_crash_text_section(const std::string &p_Text,
                             const char *p_StartMarker,
                             const char *p_EndMarker)
      {
        size_t l_Start = p_Text.find(p_StartMarker);
        if (l_Start == std::string::npos) {
          return "";
        }
        l_Start += std::strlen(p_StartMarker);

        while (l_Start < p_Text.size() &&
               (p_Text[l_Start] == '\r' || p_Text[l_Start] == '\n')) {
          ++l_Start;
        }

        size_t l_End = p_Text.find(p_EndMarker, l_Start);
        if (l_End == std::string::npos) {
          l_End = p_Text.size();
        }

        return p_Text.substr(l_Start, l_End - l_Start);
      }

      static void append_multipart_field(
          std::vector<char> &p_Body, const std::string &p_Boundary,
          const std::string &p_Name, const std::string &p_Value)
      {
        const std::string l_Header =
            "--" + p_Boundary +
            "\r\nContent-Disposition: form-data; "
            "name=\"" +
            p_Name + "\"\r\n\r\n";
        p_Body.insert(p_Body.end(), l_Header.begin(), l_Header.end());
        p_Body.insert(p_Body.end(), p_Value.begin(), p_Value.end());
        const std::string l_End = "\r\n";
        p_Body.insert(p_Body.end(), l_End.begin(), l_End.end());
      }

      static bool append_multipart_file(
          std::vector<char> &p_Body, const std::string &p_Boundary,
          const std::string &p_FieldName,
          const std::string &p_FileName, const std::string &p_Path)
      {
        std::vector<char> l_FileData;
        if (!read_binary_file(p_Path, l_FileData)) {
          return false;
        }

        const std::string l_Header =
            "--" + p_Boundary +
            "\r\nContent-Disposition: form-data; "
            "name=\"" +
            p_FieldName + "\"; filename=\"" + p_FileName +
            "\"\r\nContent-Type: application/octet-stream\r\n\r\n";
        p_Body.insert(p_Body.end(), l_Header.begin(), l_Header.end());
        p_Body.insert(p_Body.end(), l_FileData.begin(),
                      l_FileData.end());
        const std::string l_End = "\r\n";
        p_Body.insert(p_Body.end(), l_End.begin(), l_End.end());
        return true;
      }

      static bool
      upload_crash_report(const std::string &p_ReportDirectory,
                          std::string &p_Error)
      {
        const std::string l_Url = LOW_CRASH_REPORT_UPLOAD_URL;
        const std::string l_Token = LOW_CRASH_REPORT_UPLOAD_TOKEN;
        if (l_Url.empty() || l_Token.empty()) {
          p_Error = "Crash upload URL or token is empty.";
          return false;
        }

        URL_COMPONENTS l_UrlComponents = {};
        wchar_t l_Host[256] = {};
        wchar_t l_Path[1024] = {};
        l_UrlComponents.dwStructSize = sizeof(l_UrlComponents);
        l_UrlComponents.lpszHostName = l_Host;
        l_UrlComponents.dwHostNameLength =
            sizeof(l_Host) / sizeof(l_Host[0]);
        l_UrlComponents.lpszUrlPath = l_Path;
        l_UrlComponents.dwUrlPathLength =
            sizeof(l_Path) / sizeof(l_Path[0]);

        const std::wstring l_UrlWide = widen_path(l_Url);
        if (!WinHttpCrackUrl(l_UrlWide.c_str(), 0, 0,
                             &l_UrlComponents)) {
          p_Error = "Could not parse crash upload URL.";
          return false;
        }

        const std::string l_Boundary =
            "----LowEngineCrashBoundary7MA4YWxkTrZu0gW";
        std::vector<char> l_Body;

        const std::string l_Manifest = read_text_file(
            join_path(p_ReportDirectory, "release_manifest.json"));
        const std::string l_CrashText =
            read_text_file(join_path(p_ReportDirectory, "crash.txt"));
        const std::string l_UserNote = read_text_file(join_path(
            p_ReportDirectory, "last_actions_before_crash.txt"));
        const std::string l_UserName = read_text_file(
            join_path(p_ReportDirectory, "user_name.txt"));

        std::string l_ReportId = p_ReportDirectory;
        const size_t l_LastSlash = l_ReportId.find_last_of("\\/");
        if (l_LastSlash != std::string::npos) {
          l_ReportId = l_ReportId.substr(l_LastSlash + 1u);
        }

        append_multipart_field(l_Body, l_Boundary, "report_id",
                               l_ReportId);
        append_multipart_field(
            l_Body, l_Boundary, "release_id",
            get_json_string_value(l_Manifest, "release_id"));
        append_multipart_field(
            l_Body, l_Boundary, "version",
            get_json_string_value(l_Manifest, "release_version"));
        append_multipart_field(
            l_Body, l_Boundary, "lowengine_revision",
            get_json_string_value(l_Manifest, "lowengine_revision"));
        append_multipart_field(
            l_Body, l_Boundary, "misteda_revision",
            get_json_string_value(l_Manifest, "misteda_revision"));
        append_multipart_field(
            l_Body, l_Boundary, "exception_code",
            get_crash_text_value(l_CrashText, "Exception code"));
        append_multipart_field(
            l_Body, l_Boundary, "exception_address",
            get_crash_text_value(l_CrashText, "Exception address"));
        append_multipart_field(
            l_Body, l_Boundary, "callstack",
            get_crash_text_section(l_CrashText,
                                   "Callstack:", "End callstack"));
        append_multipart_field(l_Body, l_Boundary, "user_name",
                               l_UserName);
        append_multipart_field(l_Body, l_Boundary, "user_note",
                               l_UserNote);

        const char *l_FileNames[] = {"crash.txt",
                                     "minidump.dmp",
                                     "low.log",
                                     "lowerr.log",
                                     "release_manifest.json",
                                     "user_name.txt",
                                     "last_actions_before_crash.txt"};
        for (const char *i_FileName : l_FileNames) {
          append_multipart_file(
              l_Body, l_Boundary, "report_files[]", i_FileName,
              join_path(p_ReportDirectory, i_FileName));
        }

        const std::string l_EndBoundary =
            "--" + l_Boundary + "--\r\n";
        l_Body.insert(l_Body.end(), l_EndBoundary.begin(),
                      l_EndBoundary.end());

        std::wstring l_HostWide(
            l_Host, l_Host + l_UrlComponents.dwHostNameLength);
        std::wstring l_PathWide(
            l_Path, l_Path + l_UrlComponents.dwUrlPathLength);

        HINTERNET l_Session = WinHttpOpen(
            L"LowEngine Crash Reporter/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);
        if (!l_Session) {
          p_Error = "Could not create WinHTTP session.";
          return false;
        }

        HINTERNET l_Connection = WinHttpConnect(
            l_Session, l_HostWide.c_str(), l_UrlComponents.nPort, 0);
        if (!l_Connection) {
          WinHttpCloseHandle(l_Session);
          p_Error = "Could not connect to crash server.";
          return false;
        }

        const DWORD l_Flags =
            l_UrlComponents.nScheme == INTERNET_SCHEME_HTTPS
                ? WINHTTP_FLAG_SECURE
                : 0;
        HINTERNET l_Request = WinHttpOpenRequest(
            l_Connection, L"POST", l_PathWide.c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            l_Flags);
        if (!l_Request) {
          WinHttpCloseHandle(l_Connection);
          WinHttpCloseHandle(l_Session);
          p_Error = "Could not create crash upload request.";
          return false;
        }

        const std::string l_Header =
            "Content-Type: multipart/form-data; boundary=" +
            l_Boundary + "\r\nX-Misteda-Crash-Token: " + l_Token +
            "\r\n";
        const std::wstring l_HeaderWide =
            std::wstring(l_Header.begin(), l_Header.end());

        BOOL l_Success = WinHttpSendRequest(
            l_Request, l_HeaderWide.c_str(),
            static_cast<DWORD>(l_HeaderWide.size()),
            l_Body.empty() ? nullptr : l_Body.data(),
            static_cast<DWORD>(l_Body.size()),
            static_cast<DWORD>(l_Body.size()), 0);
        if (l_Success) {
          l_Success = WinHttpReceiveResponse(l_Request, nullptr);
        }

        DWORD l_StatusCode = 0;
        DWORD l_StatusSize = sizeof(l_StatusCode);
        if (l_Success) {
          WinHttpQueryHeaders(
              l_Request,
              WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
              WINHTTP_HEADER_NAME_BY_INDEX, &l_StatusCode,
              &l_StatusSize, WINHTTP_NO_HEADER_INDEX);
        }

        WinHttpCloseHandle(l_Request);
        WinHttpCloseHandle(l_Connection);
        WinHttpCloseHandle(l_Session);

        if (!l_Success) {
          p_Error = "Crash upload request failed: " +
                    get_last_error_string();
          return false;
        }

        if (l_StatusCode < 200 || l_StatusCode >= 300) {
          p_Error = "Crash server returned HTTP " +
                    std::to_string(l_StatusCode) + ".";
          return false;
        }

        return true;
      }

      static void center_window_on_monitor(HWND p_Window)
      {
        RECT l_WindowRect;
        GetWindowRect(p_Window, &l_WindowRect);

        HMONITOR l_Monitor =
            MonitorFromWindow(p_Window, MONITOR_DEFAULTTONEAREST);

        MONITORINFO l_MonitorInfo = {};
        l_MonitorInfo.cbSize = sizeof(l_MonitorInfo);
        GetMonitorInfoA(l_Monitor, &l_MonitorInfo);

        const int l_Width = l_WindowRect.right - l_WindowRect.left;
        const int l_Height = l_WindowRect.bottom - l_WindowRect.top;
        const int l_X = l_MonitorInfo.rcWork.left +
                        ((l_MonitorInfo.rcWork.right -
                          l_MonitorInfo.rcWork.left - l_Width) /
                         2);
        const int l_Y = l_MonitorInfo.rcWork.top +
                        ((l_MonitorInfo.rcWork.bottom -
                          l_MonitorInfo.rcWork.top - l_Height) /
                         2);

        SetWindowPos(p_Window, nullptr, l_X, l_Y, 0, 0,
                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
      }

      static LRESULT CALLBACK reporter_window_proc(HWND p_Window,
                                                   UINT p_Message,
                                                   WPARAM p_WParam,
                                                   LPARAM p_LParam)
      {
        ReporterWindowState *l_State =
            reinterpret_cast<ReporterWindowState *>(
                GetWindowLongPtrA(p_Window, GWLP_USERDATA));

        switch (p_Message) {
        case WM_CREATE: {
          CREATESTRUCTA *l_Create =
              reinterpret_cast<CREATESTRUCTA *>(p_LParam);
          l_State = reinterpret_cast<ReporterWindowState *>(
              l_Create->lpCreateParams);
          SetWindowLongPtrA(p_Window, GWLP_USERDATA,
                            reinterpret_cast<LONG_PTR>(l_State));

          l_State->titleFont =
              CreateFontA(24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE,
                          FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                          DEFAULT_PITCH | FF_SWISS, "Segoe UI");
          l_State->bodyFont =
              CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                          DEFAULT_PITCH | FF_SWISS, "Segoe UI");
          l_State->backgroundBrush =
              CreateSolidBrush(RGB(22, 20, 27));
          l_State->editBrush = CreateSolidBrush(RGB(18, 17, 22));

          l_State->windowIcon = load_window_icon();
          if (l_State->windowIcon) {
            SendMessageA(
                p_Window, WM_SETICON, ICON_BIG,
                reinterpret_cast<LPARAM>(l_State->windowIcon));
            SendMessageA(
                p_Window, WM_SETICON, ICON_SMALL,
                reinterpret_cast<LPARAM>(l_State->windowIcon));
          }

          l_State->logoPath = find_logo_path();
          if (!l_State->logoPath.empty()) {
            l_State->logoImage = new Gdiplus::Image(
                widen_path(l_State->logoPath).c_str());
          }
          l_State->logoTextPath = find_logo_text_path();
          if (!l_State->logoTextPath.empty()) {
            l_State->logoTextImage = new Gdiplus::Image(
                widen_path(l_State->logoTextPath).c_str());
          }

          HWND l_Title = CreateWindowExA(
              0, "STATIC", "LowEngine crashed", WS_CHILD | WS_VISIBLE,
              32, 136, 500, 34, p_Window, nullptr, nullptr, nullptr);
          SendMessageA(l_Title, WM_SETFONT,
                       reinterpret_cast<WPARAM>(l_State->titleFont),
                       TRUE);

          HWND l_Text = CreateWindowExA(
              0, "STATIC",
              "A crash report was saved. Tell us what happened right "
              "before the crash so we can reproduce it.",
              WS_CHILD | WS_VISIBLE, 32, 172, 500, 44, p_Window,
              nullptr, nullptr, nullptr);
          SendMessageA(l_Text, WM_SETFONT,
                       reinterpret_cast<WPARAM>(l_State->bodyFont),
                       TRUE);

          HWND l_NameLabel = CreateWindowExA(
              0, "STATIC", "Your name", WS_CHILD | WS_VISIBLE, 32,
              222, 100, 22, p_Window, nullptr, nullptr, nullptr);
          SendMessageA(l_NameLabel, WM_SETFONT,
                       reinterpret_cast<WPARAM>(l_State->bodyFont),
                       TRUE);

          const std::string l_SavedUserName = load_saved_user_name();
          l_State->nameControl = CreateWindowExA(
              WS_EX_CLIENTEDGE, "EDIT", l_SavedUserName.c_str(),
              WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
              126, 218, 260, 28, p_Window,
              reinterpret_cast<HMENU>(
                  static_cast<INT_PTR>(ID_NAME_EDIT)),
              nullptr, nullptr);
          SendMessageA(l_State->nameControl, WM_SETFONT,
                       reinterpret_cast<WPARAM>(l_State->bodyFont),
                       TRUE);

          l_State->editControl = CreateWindowExA(
              WS_EX_CLIENTEDGE, "EDIT", "",
              WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE |
                  ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
              32, 260, 500, 112, p_Window,
              reinterpret_cast<HMENU>(
                  static_cast<INT_PTR>(ID_ACTIONS_EDIT)),
              nullptr, nullptr);
          SendMessageA(l_State->editControl, WM_SETFONT,
                       reinterpret_cast<WPARAM>(l_State->bodyFont),
                       TRUE);

          HWND l_PathText = CreateWindowExA(
              0, "STATIC", l_State->reportDirectory.c_str(),
              WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS, 32, 382, 500,
              20, p_Window, nullptr, nullptr, nullptr);
          SendMessageA(l_PathText, WM_SETFONT,
                       reinterpret_cast<WPARAM>(l_State->bodyFont),
                       TRUE);

          HWND l_SaveButton = CreateWindowExA(
              0, "BUTTON", "Send report",
              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON |
                  BS_OWNERDRAW,
              318, 418, 104, 32, p_Window,
              reinterpret_cast<HMENU>(
                  static_cast<INT_PTR>(ID_SAVE_BUTTON)),
              nullptr, nullptr);
          SendMessageA(l_SaveButton, WM_SETFONT,
                       reinterpret_cast<WPARAM>(l_State->bodyFont),
                       TRUE);

          HWND l_SkipButton = CreateWindowExA(
              0, "BUTTON", "Skip",
              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 432,
              418, 100, 32, p_Window,
              reinterpret_cast<HMENU>(
                  static_cast<INT_PTR>(ID_SKIP_BUTTON)),
              nullptr, nullptr);
          SendMessageA(l_SkipButton, WM_SETFONT,
                       reinterpret_cast<WPARAM>(l_State->bodyFont),
                       TRUE);

          SetFocus(l_SavedUserName.empty() && l_State->nameControl
                       ? l_State->nameControl
                       : l_State->editControl);
          return 0;
        }
        case WM_COMMAND:
          if (LOWORD(p_WParam) == ID_SAVE_BUTTON && l_State) {
            write_user_name(l_State->reportDirectory,
                            l_State->nameControl);
            write_user_actions(l_State->reportDirectory,
                               l_State->editControl);
            l_State->saved = true;
            DestroyWindow(p_Window);
            return 0;
          }

          if (LOWORD(p_WParam) == ID_SKIP_BUTTON) {
            DestroyWindow(p_Window);
            return 0;
          }
          break;
        case WM_CLOSE:
          DestroyWindow(p_Window);
          return 0;
        case WM_DRAWITEM: {
          DRAWITEMSTRUCT *l_DrawItem =
              reinterpret_cast<DRAWITEMSTRUCT *>(p_LParam);
          if (!l_DrawItem || (l_DrawItem->CtlID != ID_SAVE_BUTTON &&
                              l_DrawItem->CtlID != ID_SKIP_BUTTON)) {
            break;
          }

          const bool l_IsSave = l_DrawItem->CtlID == ID_SAVE_BUTTON;
          const bool l_IsPressed =
              (l_DrawItem->itemState & ODS_SELECTED) != 0;
          const bool l_IsFocused =
              (l_DrawItem->itemState & ODS_FOCUS) != 0;

          const COLORREF l_FillColor =
              l_IsSave
                  ? (l_IsPressed ? RGB(13, 150, 163)
                                 : RGB(18, 204, 220))
                  : (l_IsPressed ? RGB(55, 49, 63) : RGB(42, 37, 49));
          const COLORREF l_BorderColor =
              l_IsSave ? RGB(80, 232, 242) : RGB(91, 82, 104);
          const COLORREF l_TextColor =
              l_IsSave ? RGB(12, 14, 18) : RGB(238, 238, 242);

          HBRUSH l_ButtonBrush = CreateSolidBrush(l_FillColor);
          FillRect(l_DrawItem->hDC, &l_DrawItem->rcItem,
                   l_ButtonBrush);
          DeleteObject(l_ButtonBrush);

          HPEN l_ButtonPen =
              CreatePen(PS_SOLID, l_IsFocused ? 2 : 1, l_BorderColor);
          HGDIOBJ l_OldPen =
              SelectObject(l_DrawItem->hDC, l_ButtonPen);
          HGDIOBJ l_OldBrush = SelectObject(
              l_DrawItem->hDC, GetStockObject(NULL_BRUSH));
          Rectangle(l_DrawItem->hDC, l_DrawItem->rcItem.left,
                    l_DrawItem->rcItem.top, l_DrawItem->rcItem.right,
                    l_DrawItem->rcItem.bottom);
          SelectObject(l_DrawItem->hDC, l_OldBrush);
          SelectObject(l_DrawItem->hDC, l_OldPen);
          DeleteObject(l_ButtonPen);

          SetBkMode(l_DrawItem->hDC, TRANSPARENT);
          SetTextColor(l_DrawItem->hDC, l_TextColor);
          const char *l_Label = l_IsSave ? "Send report" : "Skip";
          RECT l_TextRect = l_DrawItem->rcItem;
          DrawTextA(l_DrawItem->hDC, l_Label, -1, &l_TextRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE);
          return TRUE;
        }
        case WM_CTLCOLOREDIT: {
          HDC l_DeviceContext = reinterpret_cast<HDC>(p_WParam);
          SetBkMode(l_DeviceContext, OPAQUE);
          SetTextColor(l_DeviceContext, RGB(238, 238, 242));
          SetBkColor(l_DeviceContext, RGB(18, 17, 22));
          return reinterpret_cast<LRESULT>(
              l_State && l_State->editBrush
                  ? l_State->editBrush
                  : GetStockObject(BLACK_BRUSH));
        }
        case WM_CTLCOLORSTATIC: {
          HDC l_DeviceContext = reinterpret_cast<HDC>(p_WParam);
          SetBkMode(l_DeviceContext, TRANSPARENT);
          SetTextColor(l_DeviceContext, RGB(238, 238, 242));
          return reinterpret_cast<LRESULT>(
              GetStockObject(NULL_BRUSH));
        }
        case WM_ERASEBKGND: {
          HDC l_DeviceContext = reinterpret_cast<HDC>(p_WParam);
          RECT l_Rect;
          GetClientRect(p_Window, &l_Rect);
          FillRect(l_DeviceContext, &l_Rect,
                   l_State && l_State->backgroundBrush
                       ? l_State->backgroundBrush
                       : reinterpret_cast<HBRUSH>(
                             GetStockObject(BLACK_BRUSH)));
          return 1;
        }
        case WM_PAINT: {
          PAINTSTRUCT l_Paint;
          HDC l_DeviceContext = BeginPaint(p_Window, &l_Paint);
          const int l_LogoTextX = 166;
          const int l_LogoTextY = 36;
          const int l_LogoTextWidth = 285;
          int l_LogoTextHeight = 54;
          if (l_State && l_State->logoTextImage &&
              l_State->logoTextImage->GetLastStatus() ==
                  Gdiplus::Ok &&
              l_State->logoTextImage->GetWidth() > 0u) {
            l_LogoTextHeight = static_cast<int>(
                (static_cast<float>(l_LogoTextWidth) /
                 static_cast<float>(
                     l_State->logoTextImage->GetWidth())) *
                static_cast<float>(
                    l_State->logoTextImage->GetHeight()));
          }

          const int l_AccentY = l_LogoTextY + l_LogoTextHeight + 6;
          HPEN l_AccentPen =
              CreatePen(PS_SOLID, 3, RGB(18, 204, 220));
          HGDIOBJ l_OldPen =
              SelectObject(l_DeviceContext, l_AccentPen);
          MoveToEx(l_DeviceContext, l_LogoTextX, l_AccentY, nullptr);
          LineTo(l_DeviceContext, l_LogoTextX + l_LogoTextWidth + 36,
                 l_AccentY);
          SelectObject(l_DeviceContext, l_OldPen);
          DeleteObject(l_AccentPen);

          if (l_State && l_State->logoImage &&
              l_State->logoImage->GetLastStatus() == Gdiplus::Ok) {
            Gdiplus::Graphics l_Graphics(l_DeviceContext);
            l_Graphics.SetInterpolationMode(
                Gdiplus::InterpolationModeHighQualityBicubic);
            l_Graphics.DrawImage(l_State->logoImage, 54, 36, 72, 72);
            if (l_State->logoTextImage &&
                l_State->logoTextImage->GetLastStatus() ==
                    Gdiplus::Ok) {
              l_Graphics.DrawImage(l_State->logoTextImage,
                                   l_LogoTextX, l_LogoTextY,
                                   l_LogoTextWidth, l_LogoTextHeight);
            }
          } else if (l_State && l_State->windowIcon) {
            DrawIconEx(l_DeviceContext, 62, 42, l_State->windowIcon,
                       48, 48, 0, nullptr, DI_NORMAL);
          }
          EndPaint(p_Window, &l_Paint);
          return 0;
        }
        case WM_DESTROY:
          if (l_State) {
            delete l_State->logoImage;
            l_State->logoImage = nullptr;
            delete l_State->logoTextImage;
            l_State->logoTextImage = nullptr;
            if (l_State->titleFont) {
              DeleteObject(l_State->titleFont);
              l_State->titleFont = nullptr;
            }
            if (l_State->bodyFont) {
              DeleteObject(l_State->bodyFont);
              l_State->bodyFont = nullptr;
            }
            if (l_State->windowIcon) {
              DestroyIcon(l_State->windowIcon);
              l_State->windowIcon = nullptr;
            }
            if (l_State->backgroundBrush) {
              DeleteObject(l_State->backgroundBrush);
              l_State->backgroundBrush = nullptr;
            }
            if (l_State->editBrush) {
              DeleteObject(l_State->editBrush);
              l_State->editBrush = nullptr;
            }
          }
          PostQuitMessage(0);
          return 0;
        }

        return DefWindowProcA(p_Window, p_Message, p_WParam,
                              p_LParam);
      }

      static void
      show_reporter_popup(const std::string &p_ReportDirectory)
      {
        ReporterWindowState l_State;
        l_State.reportDirectory = p_ReportDirectory;

        HINSTANCE l_Instance = GetModuleHandleA(nullptr);
        const char *l_ClassName = "LowderCrashReporterWindow";

        WNDCLASSA l_Class = {};
        l_Class.lpfnWndProc = reporter_window_proc;
        l_Class.hInstance = l_Instance;
        l_Class.lpszClassName = l_ClassName;
        l_Class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        l_Class.hbrBackground = nullptr;
        RegisterClassA(&l_Class);

        HWND l_Window = CreateWindowExA(
            WS_EX_TOPMOST, l_ClassName, "LowEngine Crash Reporter",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT,
            CW_USEDEFAULT, 584, 500, nullptr, nullptr, l_Instance,
            &l_State);

        if (!l_Window) {
          append_text_file(
              join_path(p_ReportDirectory, "crash.txt"),
              "Failed to create crash reporter window: " +
                  get_last_error_string() + "\r\n");
          return;
        }

        center_window_on_monitor(l_Window);
        ShowWindow(l_Window, SW_SHOW);
        UpdateWindow(l_Window);

        MSG l_Message;
        while (GetMessageA(&l_Message, nullptr, 0, 0) > 0) {
          TranslateMessage(&l_Message);
          DispatchMessageA(&l_Message);
        }

        if (l_State.saved) {
          std::string l_UploadError;
          l_State.uploaded =
              upload_crash_report(p_ReportDirectory, l_UploadError);

          if (l_State.uploaded) {
            MessageBoxA(nullptr, "Crash report uploaded. Thank you!",
                        "LowEngine Crash Reporter",
                        MB_OK | MB_ICONINFORMATION);
          } else {
            MessageBoxA(nullptr,
                        ("Crash report was saved locally, but upload "
                         "failed:\n" +
                         l_UploadError + "\n\nLocal path:\n" +
                         p_ReportDirectory)
                            .c_str(),
                        "LowEngine Crash Reporter",
                        MB_OK | MB_ICONWARNING);
          }
        } else {
          delete_report_directory(p_ReportDirectory);
        }
      }

      static void
      collect_crash_report(EXCEPTION_POINTERS *p_ExceptionPointers)
      {
        const std::string l_ReportDirectory = make_report_directory();

        write_text_file(join_path(l_ReportDirectory, "crash.txt"),
                        format_exception_info(p_ExceptionPointers));

        write_minidump(l_ReportDirectory, p_ExceptionPointers);

        copy_file_if_exists(join_path(g_ProjectPath, "low.log"),
                            join_path(l_ReportDirectory, "low.log"));
        copy_file_if_exists(
            join_path(g_ProjectPath, "lowerr.log"),
            join_path(l_ReportDirectory, "lowerr.log"));
        copy_file_if_exists(
            join_path(g_ProjectPath, "release_manifest.json"),
            join_path(l_ReportDirectory, "release_manifest.json"));

        show_reporter_popup(l_ReportDirectory);
      }

      static LONG WINAPI unhandled_exception_filter(
          EXCEPTION_POINTERS *p_ExceptionPointers)
      {
        collect_crash_report(p_ExceptionPointers);
        return EXCEPTION_EXECUTE_HANDLER;
      }

      static void terminate_handler()
      {
        collect_crash_report(nullptr);
        std::abort();
      }
#endif
    } // namespace

    void initialize(const std::string &p_ProjectPath)
    {
      g_ProjectPath =
          normalize_path(p_ProjectPath.empty() ? "." : p_ProjectPath);

#ifdef _WIN32
      Gdiplus::GdiplusStartupInput l_Input;
      Gdiplus::GdiplusStartup(&g_GdiplusToken, &l_Input, nullptr);
      g_PreviousExceptionFilter =
          SetUnhandledExceptionFilter(unhandled_exception_filter);
      std::set_terminate(terminate_handler);
#endif
    }

    void shutdown()
    {
#ifdef _WIN32
      SetUnhandledExceptionFilter(g_PreviousExceptionFilter);
      if (g_GdiplusToken) {
        Gdiplus::GdiplusShutdown(g_GdiplusToken);
        g_GdiplusToken = 0;
      }
#endif
    }
  } // namespace CrashReporter
} // namespace Lowder
