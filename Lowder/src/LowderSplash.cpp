#include "LowderSplash.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#include <vector>

#pragma comment(lib, "Gdiplus.lib")
#endif

namespace Lowder {
  namespace Splash {
#ifdef _WIN32
    namespace {
      constexpr int SPLASH_WIDTH = 560;
      constexpr int SPLASH_HEIGHT = 280;

      std::thread g_Thread;
      std::atomic<bool> g_Running{false};
      std::atomic<bool> g_Ready{false};
      DWORD g_ThreadId = 0;
      HWND g_Window = nullptr;
      Gdiplus::Image *g_Logo = nullptr;
      Gdiplus::Image *g_TextLogo = nullptr;
      ULONG_PTR g_GdiplusToken = 0;
      std::mutex g_StatusMutex;
      std::string g_Status = "Starting LowEngine...";

      std::string get_status()
      {
        std::lock_guard<std::mutex> l_Lock(g_StatusMutex);
        return g_Status;
      }

      std::wstring widen(const std::string &p_String)
      {
        if (p_String.empty()) {
          return L"";
        }

        int l_Length = MultiByteToWideChar(
            CP_UTF8, 0, p_String.c_str(), -1, nullptr, 0);
        std::wstring l_Result(l_Length, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, p_String.c_str(), -1,
                            l_Result.data(), l_Length);
        if (!l_Result.empty() && l_Result.back() == L'\0') {
          l_Result.pop_back();
        }
        return l_Result;
      }

      bool file_exists(const std::string &p_Path)
      {
        DWORD l_Attributes = GetFileAttributesA(p_Path.c_str());
        return l_Attributes != INVALID_FILE_ATTRIBUTES &&
               (l_Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
      }

      std::string join_path(const std::string &p_Left,
                            const std::string &p_Right)
      {
        if (p_Left.empty()) {
          return p_Right;
        }
        char l_Last = p_Left[p_Left.size() - 1];
        if (l_Last == '\\' || l_Last == '/') {
          return p_Left + p_Right;
        }
        return p_Left + "\\" + p_Right;
      }

      void
      add_image_candidates(const std::string &p_Base,
                           const char *p_FileName,
                           std::vector<std::string> &p_Candidates)
      {
        std::string l_Current = p_Base;
        for (int i = 0; i < 8 && !l_Current.empty(); ++i) {
          p_Candidates.push_back(
              join_path(join_path(l_Current, "data\\.editor_images"),
                        p_FileName));
          p_Candidates.push_back(join_path(
              join_path(l_Current, "misteda\\data\\.editor_images"),
              p_FileName));

          size_t l_Pos = l_Current.find_last_of("\\/");
          if (l_Pos == std::string::npos) {
            break;
          }
          l_Current = l_Current.substr(0, l_Pos);
        }
      }

      std::string find_image_path(const char *p_FileName)
      {
        std::vector<std::string> l_Candidates;

        char l_CurrentDirectory[MAX_PATH] = {};
        if (GetCurrentDirectoryA(MAX_PATH, l_CurrentDirectory) > 0) {
          add_image_candidates(l_CurrentDirectory, p_FileName,
                               l_Candidates);
        }

        char l_ModulePath[MAX_PATH] = {};
        if (GetModuleFileNameA(nullptr, l_ModulePath, MAX_PATH) > 0) {
          std::string l_ModuleDirectory = l_ModulePath;
          size_t l_Pos = l_ModuleDirectory.find_last_of("\\/");
          if (l_Pos != std::string::npos) {
            l_ModuleDirectory = l_ModuleDirectory.substr(0, l_Pos);
            add_image_candidates(l_ModuleDirectory, p_FileName,
                                 l_Candidates);
          }
        }

        for (const std::string &i_Path : l_Candidates) {
          if (file_exists(i_Path)) {
            return i_Path;
          }
        }
        return "";
      }

      Gdiplus::Image *load_image(const char *p_FileName)
      {
        std::string l_Path = find_image_path(p_FileName);
        if (l_Path.empty()) {
          return nullptr;
        }

        Gdiplus::Image *l_Image =
            Gdiplus::Image::FromFile(widen(l_Path).c_str());
        if (l_Image && l_Image->GetLastStatus() != Gdiplus::Ok) {
          delete l_Image;
          return nullptr;
        }
        return l_Image;
      }

      void paint_splash(HWND p_Window)
      {
        PAINTSTRUCT l_Paint;
        HDC l_Hdc = BeginPaint(p_Window, &l_Paint);
        RECT l_Client;
        GetClientRect(p_Window, &l_Client);

        HDC l_MemoryDc = CreateCompatibleDC(l_Hdc);
        HBITMAP l_BackBuffer = CreateCompatibleBitmap(
            l_Hdc, l_Client.right - l_Client.left,
            l_Client.bottom - l_Client.top);
        HGDIOBJ l_PreviousBitmap =
            SelectObject(l_MemoryDc, l_BackBuffer);

        HBRUSH l_Background = CreateSolidBrush(RGB(24, 22, 27));
        FillRect(l_MemoryDc, &l_Client, l_Background);
        DeleteObject(l_Background);

        HPEN l_BorderPen = CreatePen(PS_SOLID, 1, RGB(86, 77, 94));
        HGDIOBJ l_PreviousPen = SelectObject(l_MemoryDc, l_BorderPen);
        HGDIOBJ l_PreviousBrush =
            SelectObject(l_MemoryDc, GetStockObject(NULL_BRUSH));
        Rectangle(l_MemoryDc, 0, 0, SPLASH_WIDTH, SPLASH_HEIGHT);
        SelectObject(l_MemoryDc, l_PreviousBrush);
        SelectObject(l_MemoryDc, l_PreviousPen);
        DeleteObject(l_BorderPen);

        SetBkMode(l_MemoryDc, TRANSPARENT);
        SetTextColor(l_MemoryDc, RGB(245, 245, 248));

        Gdiplus::Graphics l_Graphics(l_MemoryDc);
        l_Graphics.SetInterpolationMode(
            Gdiplus::InterpolationModeHighQualityBicubic);
        l_Graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        if (g_Logo && g_Logo->GetLastStatus() == Gdiplus::Ok) {
          l_Graphics.DrawImage(g_Logo, 48, 70, 88, 88);
        }

        int l_LinePos = 128;

        if (g_TextLogo &&
            g_TextLogo->GetLastStatus() == Gdiplus::Ok) {
          const int l_MaxTextWidth = SPLASH_WIDTH - 212;
          const int l_MaxTextHeight = 48;
          const float l_SourceWidth =
              static_cast<float>(g_TextLogo->GetWidth());
          const float l_SourceHeight =
              static_cast<float>(g_TextLogo->GetHeight());
          if (l_SourceWidth > 0.0f && l_SourceHeight > 0.0f) {
#if 0 
            float l_Scale = l_MaxTextHeight / l_SourceHeight;
            if (l_SourceWidth * l_Scale > l_MaxTextWidth) {
              l_Scale = l_MaxTextWidth / l_SourceWidth;
            }
            const int l_TextWidth =
                static_cast<int>(l_SourceWidth * l_Scale);
            const int l_TextHeight =
                static_cast<int>(l_SourceHeight * l_Scale);
            l_Graphics.DrawImage(g_TextLogo, 164,
                                 82 - (l_TextHeight / 2), l_TextWidth,
                                 l_TextHeight);
            l_LinePos = 128;
#else
            const float l_Scale = 0.55f;

            const int l_TextWidth =
                static_cast<int>(l_SourceWidth * l_Scale);
            const int l_TextHeight =
                static_cast<int>(l_SourceHeight * l_Scale);
            l_Graphics.DrawImage(g_TextLogo, 160, 80, l_TextWidth,
                                 l_TextHeight);
            l_LinePos = 150;
#endif
          }
        }

        HPEN l_AccentPen = CreatePen(PS_SOLID, 3, RGB(2, 173, 191));
        l_PreviousPen = SelectObject(l_MemoryDc, l_AccentPen);
        MoveToEx(l_MemoryDc, 164, l_LinePos, nullptr);
        LineTo(l_MemoryDc, SPLASH_WIDTH - 50, l_LinePos);
        SelectObject(l_MemoryDc, l_PreviousPen);
        DeleteObject(l_AccentPen);

        SetTextColor(l_MemoryDc, RGB(190, 184, 198));
        HFONT l_StatusFont =
            CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        HGDIOBJ l_PreviousFont =
            SelectObject(l_MemoryDc, l_StatusFont);
        std::string l_Status = get_status();
        RECT l_StatusRect{50, 200, SPLASH_WIDTH - 50, 240};
        DrawTextA(l_MemoryDc, l_Status.c_str(), -1, &l_StatusRect,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER |
                      DT_END_ELLIPSIS);
        SelectObject(l_MemoryDc, l_PreviousFont);
        DeleteObject(l_StatusFont);

        BitBlt(l_Hdc, 0, 0, l_Client.right - l_Client.left,
               l_Client.bottom - l_Client.top, l_MemoryDc, 0, 0,
               SRCCOPY);

        SelectObject(l_MemoryDc, l_PreviousBitmap);
        DeleteObject(l_BackBuffer);
        DeleteDC(l_MemoryDc);

        EndPaint(p_Window, &l_Paint);
      }

      LRESULT CALLBACK splash_window_proc(HWND p_Window,
                                          UINT p_Message,
                                          WPARAM p_WParam,
                                          LPARAM p_LParam)
      {
        switch (p_Message) {
        case WM_ERASEBKGND:
          return 1;
        case WM_PAINT:
          paint_splash(p_Window);
          return 0;
        case WM_CLOSE:
          DestroyWindow(p_Window);
          return 0;
        case WM_DESTROY:
          KillTimer(p_Window, 1);
          g_Window = nullptr;
          PostQuitMessage(0);
          return 0;
        default:
          return DefWindowProc(p_Window, p_Message, p_WParam,
                               p_LParam);
        }
      }

      void splash_thread()
      {
        g_ThreadId = GetCurrentThreadId();
        Gdiplus::GdiplusStartupInput l_GdiplusInput;
        Gdiplus::GdiplusStartup(&g_GdiplusToken, &l_GdiplusInput,
                                nullptr);

        g_Logo = load_image("lowlogo_90.png");
        g_TextLogo = load_image("lowfont_500.png");

        HINSTANCE l_Instance = GetModuleHandle(nullptr);
        const char *l_ClassName = "LowEngineSplashWindow";

        WNDCLASSA l_Class = {};
        l_Class.lpfnWndProc = splash_window_proc;
        l_Class.hInstance = l_Instance;
        l_Class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        l_Class.lpszClassName = l_ClassName;
        RegisterClassA(&l_Class);

        int l_ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        int l_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
        int l_X = (l_ScreenWidth - SPLASH_WIDTH) / 2;
        int l_Y = (l_ScreenHeight - SPLASH_HEIGHT) / 2;

        g_Window = CreateWindowExA(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW, l_ClassName,
            "LowEngine", WS_POPUP, l_X, l_Y, SPLASH_WIDTH,
            SPLASH_HEIGHT, nullptr, nullptr, l_Instance, nullptr);

        if (g_Window) {
          ShowWindow(g_Window, SW_SHOWNORMAL);
          UpdateWindow(g_Window);
        }

        g_Ready = true;

        MSG l_Message;
        while (g_Running &&
               GetMessage(&l_Message, nullptr, 0, 0) > 0) {
          TranslateMessage(&l_Message);
          DispatchMessage(&l_Message);
        }

        if (g_Window) {
          DestroyWindow(g_Window);
          g_Window = nullptr;
        }
        UnregisterClassA(l_ClassName, l_Instance);

        delete g_Logo;
        g_Logo = nullptr;
        delete g_TextLogo;
        g_TextLogo = nullptr;
        if (g_GdiplusToken != 0) {
          Gdiplus::GdiplusShutdown(g_GdiplusToken);
          g_GdiplusToken = 0;
        }
      }
    } // namespace
#endif

    void start()
    {
#ifdef _WIN32
      if (g_Running) {
        return;
      }

      g_Running = true;
      g_Ready = false;
      g_Thread = std::thread(splash_thread);

      while (!g_Ready) {
        Sleep(1);
      }
#endif
    }

    void set_status(const std::string &p_Status)
    {
#ifdef _WIN32
      {
        std::lock_guard<std::mutex> l_Lock(g_StatusMutex);
        g_Status = p_Status;
      }
      if (g_Window) {
        InvalidateRect(g_Window, nullptr, FALSE);
      }
#else
      (void)p_Status;
#endif
    }

    void stop()
    {
#ifdef _WIN32
      if (!g_Running) {
        return;
      }

      g_Running = false;
      if (g_Window) {
        PostMessage(g_Window, WM_CLOSE, 0, 0);
      }
      if (g_ThreadId != 0) {
        PostThreadMessage(g_ThreadId, WM_QUIT, 0, 0);
      }
      if (g_Thread.joinable()) {
        g_Thread.join();
      }
      g_ThreadId = 0;
#endif
    }
  } // namespace Splash
} // namespace Lowder
