#include "LowUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilConfig.h"
#include "LowUtilHandle.h"
#include "LowUtilProfiler.h"
#include "LowUtilMemory.h"
#include "LowUtilJobManager.h"
#include "LowUtilFileSystem.h"
#include "LowUtilAssetManager.h"
#include "SDL_video.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#endif

namespace Low {
  namespace Util {
    Project g_Project;

    Low::Util::Window g_MainWindow;
    bool g_MainWindowInitiallyHidden = false;

#ifdef _WIN32
    static LRESULT CALLBACK custom_window_proc(HWND p_Hwnd,
                                               UINT p_Message,
                                               WPARAM p_WParam,
                                               LPARAM p_LParam)
    {
      if (p_Message == WM_NCCALCSIZE &&
          g_MainWindow.customDecorations &&
          p_Hwnd == static_cast<HWND>(g_MainWindow.nativeHandle)) {
        return 0;
      }

      if (p_Message == WM_NCHITTEST &&
          g_MainWindow.customDecorations &&
          p_Hwnd == static_cast<HWND>(g_MainWindow.nativeHandle)) {
        POINT l_Point{GET_X_LPARAM(p_LParam),
                      GET_Y_LPARAM(p_LParam)};
        ScreenToClient(p_Hwnd, &l_Point);

        RECT l_ClientRect{};
        GetClientRect(p_Hwnd, &l_ClientRect);

        const int l_Width = l_ClientRect.right - l_ClientRect.left;
        const int l_Height = l_ClientRect.bottom - l_ClientRect.top;
        const int l_Border = g_MainWindow.customResizeBorder;

        if (l_Point.y < g_MainWindow.customTitleBarHeight &&
            l_Point.x >= g_MainWindow.customTitleBarControlsLeft) {
          return HTCLIENT;
        }

        const bool l_Left = l_Point.x < l_Border;
        const bool l_Right = l_Point.x >= l_Width - l_Border;
        const bool l_Top = l_Point.y < l_Border;
        const bool l_Bottom = l_Point.y >= l_Height - l_Border;

        if (l_Left && l_Top) {
          return HTTOPLEFT;
        }
        if (l_Right && l_Top) {
          return HTTOPRIGHT;
        }
        if (l_Left && l_Bottom) {
          return HTBOTTOMLEFT;
        }
        if (l_Right && l_Bottom) {
          return HTBOTTOMRIGHT;
        }
        if (l_Top) {
          return HTTOP;
        }
        if (l_Bottom) {
          return HTBOTTOM;
        }
        if (l_Left) {
          return HTLEFT;
        }
        if (l_Right) {
          return HTRIGHT;
        }

        if (l_Point.y < g_MainWindow.customTitleBarHeight &&
            l_Point.x >= g_MainWindow.customTitleBarInteractiveLeft &&
            l_Point.x < g_MainWindow.customTitleBarControlsLeft) {
          return HTCAPTION;
        }
      }

      WNDPROC l_Previous =
          reinterpret_cast<WNDPROC>(g_MainWindow.nativeWndProc);
      if (l_Previous) {
        return CallWindowProc(l_Previous, p_Hwnd, p_Message,
                              p_WParam, p_LParam);
      }
      return DefWindowProc(p_Hwnd, p_Message, p_WParam, p_LParam);
    }

    static void install_custom_window_proc(Window &p_Window)
    {
      SDL_SysWMinfo l_Info;
      SDL_VERSION(&l_Info.version);
      if (!SDL_GetWindowWMInfo(p_Window.sdlwindow, &l_Info)) {
        return;
      }

      HWND l_Hwnd = l_Info.info.win.window;
      p_Window.nativeHandle = l_Hwnd;

      LONG_PTR l_Style = GetWindowLongPtr(l_Hwnd, GWL_STYLE);
      if (!p_Window.nativeStyle) {
        p_Window.nativeStyle = static_cast<std::intptr_t>(l_Style);
      }
      l_Style &= ~WS_POPUP;
      l_Style |= WS_OVERLAPPEDWINDOW;
      SetWindowLongPtr(l_Hwnd, GWL_STYLE, l_Style);

      if (!p_Window.nativeWndProc) {
        p_Window.nativeWndProc =
            reinterpret_cast<void *>(SetWindowLongPtr(
                l_Hwnd, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(custom_window_proc)));
      }

      SetWindowPos(l_Hwnd, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                       SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }

    static void restore_custom_window_proc(Window &p_Window)
    {
      if (!p_Window.nativeHandle || !p_Window.nativeWndProc) {
        return;
      }

      SetWindowLongPtr(
          static_cast<HWND>(p_Window.nativeHandle), GWLP_WNDPROC,
          reinterpret_cast<LONG_PTR>(p_Window.nativeWndProc));
      p_Window.nativeWndProc = nullptr;
      if (p_Window.nativeStyle) {
        SetWindowLongPtr(static_cast<HWND>(p_Window.nativeHandle),
                         GWL_STYLE,
                         static_cast<LONG_PTR>(p_Window.nativeStyle));
      }
    }
#endif

    static SDL_HitTestResult custom_window_hit_test(SDL_Window *p_Window,
                                                    const SDL_Point *p_Area,
                                                    void *p_Data)
    {
      Window *l_Window = static_cast<Window *>(p_Data);
      if (!l_Window || !l_Window->customDecorations ||
          p_Window != l_Window->sdlwindow) {
        return SDL_HITTEST_NORMAL;
      }

      int l_Width = 0;
      int l_Height = 0;
      SDL_GetWindowSize(p_Window, &l_Width, &l_Height);

      const int l_Border = l_Window->customResizeBorder;

      if (p_Area->y < l_Window->customTitleBarHeight &&
          p_Area->x >= l_Window->customTitleBarControlsLeft) {
        return SDL_HITTEST_NORMAL;
      }

      const bool l_Left = p_Area->x < l_Border;
      const bool l_Right = p_Area->x >= l_Width - l_Border;
      const bool l_Top = p_Area->y < l_Border;
      const bool l_Bottom = p_Area->y >= l_Height - l_Border;

      if (l_Left && l_Top) {
        return SDL_HITTEST_RESIZE_TOPLEFT;
      }
      if (l_Right && l_Top) {
        return SDL_HITTEST_RESIZE_TOPRIGHT;
      }
      if (l_Left && l_Bottom) {
        return SDL_HITTEST_RESIZE_BOTTOMLEFT;
      }
      if (l_Right && l_Bottom) {
        return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
      }
      if (l_Top) {
        return SDL_HITTEST_RESIZE_TOP;
      }
      if (l_Bottom) {
        return SDL_HITTEST_RESIZE_BOTTOM;
      }
      if (l_Left) {
        return SDL_HITTEST_RESIZE_LEFT;
      }
      if (l_Right) {
        return SDL_HITTEST_RESIZE_RIGHT;
      }

      if (p_Area->y < l_Window->customTitleBarHeight &&
          p_Area->x >= l_Window->customTitleBarInteractiveLeft &&
          p_Area->x < l_Window->customTitleBarControlsLeft) {
        return SDL_HITTEST_DRAGGABLE;
      }

      return SDL_HITTEST_NORMAL;
    }

    void initialize()
    {

      g_Project.dataPath = "./data";
      g_Project.rootPath = "./";
      g_Project.assetCachePath = "./data/.asset_cache";
      g_Project.editorImagesPath = "./data/.editor_images";
      g_Project.managedPath = "./data/.managed";
      g_Project.visualScriptOut = "./data/.vs_out";

      g_Project.engineRootPath = "./";
      g_Project.engineDataPath = "./LowData";

      Log::initialize();
      Memory::initialize();
      Name::initialize();
      Config::initialize();
      JobManager::initialize();
      AssetManager::initialize();

      {
        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags =
            (SDL_WindowFlags)(SDL_WINDOW_VULKAN |
                              SDL_WINDOW_RESIZABLE);
        if (g_MainWindowInitiallyHidden) {
          window_flags =
              (SDL_WindowFlags)(window_flags | SDL_WINDOW_HIDDEN);
        }

        g_MainWindow.shouldClose = false;
        g_MainWindow.minimized = false;
        g_MainWindow.customResizeBorder = 10;
        g_MainWindow.sdlwindow = SDL_CreateWindow(
            "LowEngine", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, 1280, 720, window_flags);
        g_MainWindow.set_custom_decorations(true);
      }

      LOW_LOG_INFO << "Util initialized" << LOW_LOG_END;
    }

    void set_main_window_initially_hidden(bool p_Hidden)
    {
      g_MainWindowInitiallyHidden = p_Hidden;
    }

    int execute_command(const String &p_Command, bool p_HideWindow,
                        String *p_Output)
    {
#ifdef _WIN32
      STARTUPINFOA l_StartupInfo = {};
      l_StartupInfo.cb = sizeof(l_StartupInfo);

      HANDLE l_ReadPipe = nullptr;
      HANDLE l_WritePipe = nullptr;

      if (p_Output) {
        SECURITY_ATTRIBUTES l_Sa = {};
        l_Sa.nLength = sizeof(l_Sa);
        l_Sa.bInheritHandle = TRUE;
        CreatePipe(&l_ReadPipe, &l_WritePipe, &l_Sa, 0);
        SetHandleInformation(l_ReadPipe, HANDLE_FLAG_INHERIT, 0);

        l_StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
        l_StartupInfo.hStdOutput = l_WritePipe;
        l_StartupInfo.hStdError = l_WritePipe;
        l_StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
      }

      if (p_HideWindow) {
        l_StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        l_StartupInfo.wShowWindow = SW_HIDE;
      }

      PROCESS_INFORMATION l_ProcessInfo = {};
      String l_Command = p_Command;
      if (!CreateProcessA(nullptr, l_Command.data(), nullptr, nullptr,
                          p_Output ? TRUE : FALSE,
                          p_HideWindow ? CREATE_NO_WINDOW : 0, nullptr,
                          nullptr, &l_StartupInfo, &l_ProcessInfo)) {
        if (l_ReadPipe)
          CloseHandle(l_ReadPipe);
        if (l_WritePipe)
          CloseHandle(l_WritePipe);
        return -1;
      }

      if (p_Output) {
        CloseHandle(l_WritePipe);
        char l_Buf[4096];
        DWORD l_BytesRead;
        while (ReadFile(l_ReadPipe, l_Buf, sizeof(l_Buf) - 1,
                        &l_BytesRead, nullptr) &&
               l_BytesRead > 0) {
          l_Buf[l_BytesRead] = '\0';
          *p_Output += l_Buf;
        }
        CloseHandle(l_ReadPipe);
      }

      WaitForSingleObject(l_ProcessInfo.hProcess, INFINITE);

      DWORD l_ExitCode = 0;
      GetExitCodeProcess(l_ProcessInfo.hProcess, &l_ExitCode);
      CloseHandle(l_ProcessInfo.hThread);
      CloseHandle(l_ProcessInfo.hProcess);
      return static_cast<int>(l_ExitCode);
#else
      (void)p_HideWindow;
      if (p_Output) {
        FILE *l_Pipe = popen(p_Command.c_str(), "r");
        if (!l_Pipe)
          return -1;
        char l_Buf[4096];
        while (fgets(l_Buf, sizeof(l_Buf), l_Pipe)) {
          *p_Output += l_Buf;
        }
        return pclose(l_Pipe);
      }
      return system(p_Command.c_str());
#endif
    }

    void tick_handle_reference_resolvers(const float p_Delta);

    void tick(float p_Delta)
    {
      FileSystem::tick(p_Delta);
      AssetManager::tick(p_Delta);
      tick_handle_reference_resolvers(p_Delta);

      SDL_Event e;

      while (SDL_PollEvent(&e) != 0) {
        // close the window when user alt-f4s or clicks the X button
        if (e.type == SDL_QUIT)
          g_MainWindow.shouldClose = true;

        if (e.type == SDL_WINDOWEVENT) {
          if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
            g_MainWindow.minimized = true;
          }
          if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
            g_MainWindow.minimized = false;
          }
          if (e.window.event == SDL_WINDOWEVENT_CLOSE &&
              e.window.windowID ==
                  SDL_GetWindowID(g_MainWindow.sdlwindow)) {
            g_MainWindow.shouldClose = true;
          }
        }

        for (u32 i = 0;
             i < Window::get_main_window().eventCallbacks.size();
             ++i) {
          Window::get_main_window().eventCallbacks[i](&e);
        }
      }

      // do not draw if we are minimized
      if (g_MainWindow.minimized) {
        // throttle the speed to avoid the endless spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    void cleanup()
    {
      SDL_DestroyWindow(g_MainWindow.sdlwindow);

      AssetManager::cleanup();
      JobManager::cleanup();
      Name::cleanup();
      Memory::cleanup();

      Profiler::evaluate_memory_allocation();

      LOW_LOG_INFO << "Util shutdown" << LOW_LOG_END;
      Log::cleanup();
    }

    const Project &get_project()
    {
      return g_Project;
    }

    namespace {
      static String trim_path_separators(const String &p_Path)
      {
        if (p_Path.empty()) {
          return p_Path;
        }

        u32 l_Start = 0;
        u32 l_End = (u32)p_Path.size();

        while (l_Start < l_End &&
               (p_Path[l_Start] == '/' || p_Path[l_Start] == '\\')) {
          ++l_Start;
        }

        while (l_End > l_Start && (p_Path[l_End - 1] == '/' ||
                                   p_Path[l_End - 1] == '\\')) {
          --l_End;
        }

        return p_Path.substr(l_Start, l_End - l_Start);
      }
    } // namespace

    ProjectPathBuilder::ProjectPathBuilder(String p_RootPath)
    {
      String l_Root = p_RootPath;
      while (!l_Root.empty() &&
             (l_Root.back() == '/' || l_Root.back() == '\\')) {
        l_Root.pop_back();
      }

      m_Builder.append(l_Root);
      m_HasPath = !l_Root.empty();
    }

    ProjectPathBuilder &
    ProjectPathBuilder::join(const String &p_PathPart)
    {
      String l_Part = trim_path_separators(p_PathPart);
      if (l_Part.empty()) {
        return *this;
      }

      if (m_HasPath) {
        m_Builder.append("/");
      }

      m_Builder.append(l_Part);
      m_HasPath = true;
      return *this;
    }

    ProjectPathBuilder &
    ProjectPathBuilder::join(const char *p_PathPart)
    {
      return join(String(p_PathPart));
    }

    String ProjectPathBuilder::get() const
    {
      return m_Builder.get();
    }

    ProjectPathBuilder::operator String() const
    {
      return get();
    }

    ProjectPathBuilder project_data_path()
    {
      return ProjectPathBuilder(get_project().dataPath);
    }

    ProjectPathBuilder project_managed_path()
    {
      return ProjectPathBuilder(get_project().managedPath);
    }

    ProjectPathBuilder project_visual_script_out_path()
    {
      return ProjectPathBuilder(get_project().visualScriptOut);
    }

    ProjectPathBuilder project_root_path()
    {
      return ProjectPathBuilder(get_project().rootPath);
    }

    ProjectPathBuilder project_asset_cache_path()
    {
      return ProjectPathBuilder(get_project().assetCachePath);
    }

    ProjectPathBuilder project_editor_images_path()
    {
      return ProjectPathBuilder(get_project().editorImagesPath);
    }

    ProjectPathBuilder engine_root_path()
    {
      return ProjectPathBuilder(get_project().engineRootPath);
    }

    ProjectPathBuilder engine_data_path()
    {
      return ProjectPathBuilder(get_project().engineDataPath);
    }

    String project_data_path(const String &p_RelativePath)
    {
      return project_data_path().join(p_RelativePath).get();
    }

    String project_root_path(const String &p_RelativePath)
    {
      return project_root_path().join(p_RelativePath).get();
    }

    String project_managed_path(const String &p_RelativePath)
    {
      return project_managed_path().join(p_RelativePath).get();
    }

    String project_asset_cache_path(const String &p_RelativePath)
    {
      return project_asset_cache_path().join(p_RelativePath).get();
    }

    String project_editor_images_path(const String &p_RelativePath)
    {
      return project_editor_images_path().join(p_RelativePath).get();
    }

    String
    project_visual_script_out_path(const String &p_RelativePath)
    {
      return project_visual_script_out_path()
          .join(p_RelativePath)
          .get();
    }

    String engine_root_path(const String &p_RelativePath)
    {
      return engine_root_path().join(p_RelativePath).get();
    }

    String engine_data_path(const String &p_RelativePath)
    {
      return engine_data_path().join(p_RelativePath).get();
    }

    Window &Window::get_main_window()
    {
      return g_MainWindow;
    }

    void Window::get_size(int *p_Width, int *p_Height)
    {
      SDL_GetWindowSize(sdlwindow, p_Width, p_Height);
    }

    void Window::set_custom_decorations(bool p_Enabled)
    {
      customDecorations = p_Enabled;
#ifndef _WIN32
      SDL_SetWindowBordered(sdlwindow, p_Enabled ? SDL_FALSE
                                                 : SDL_TRUE);
#endif
#ifdef _WIN32
      if (p_Enabled) {
        install_custom_window_proc(*this);
      } else {
        restore_custom_window_proc(*this);
      }
#endif
      SDL_SetWindowHitTest(sdlwindow,
                           p_Enabled ? custom_window_hit_test
                                     : nullptr,
                           p_Enabled ? this : nullptr);
    }

    void Window::set_custom_title_bar_hit_zones(
        int p_Height, int p_InteractiveLeft, int p_ControlsLeft)
    {
      customTitleBarHeight = p_Height;
      customTitleBarInteractiveLeft = p_InteractiveLeft;
      customTitleBarControlsLeft = p_ControlsLeft;
    }

    void Window::minimize()
    {
#ifdef _WIN32
      if (nativeHandle) {
        SendMessage(static_cast<HWND>(nativeHandle), WM_SYSCOMMAND,
                    SC_MINIMIZE, 0);
        return;
      }
#endif
      SDL_MinimizeWindow(sdlwindow);
    }

    void Window::maximize_or_restore()
    {
#ifdef _WIN32
      if (nativeHandle) {
        HWND l_Hwnd = static_cast<HWND>(nativeHandle);
        SendMessage(l_Hwnd, WM_SYSCOMMAND,
                    IsZoomed(l_Hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
        return;
      }
#endif
      if ((SDL_GetWindowFlags(sdlwindow) & SDL_WINDOW_MAXIMIZED) !=
          0) {
        SDL_RestoreWindow(sdlwindow);
      } else {
        SDL_MaximizeWindow(sdlwindow);
      }
    }

    void Window::request_close()
    {
#ifdef _WIN32
      if (nativeHandle) {
        PostMessage(static_cast<HWND>(nativeHandle), WM_CLOSE, 0, 0);
      }
#endif
      shouldClose = true;
    }

    void Window::show()
    {
      SDL_ShowWindow(sdlwindow);
    }

    void Window::hide()
    {
      SDL_HideWindow(sdlwindow);
    }
  } // namespace Util
} // namespace Low
