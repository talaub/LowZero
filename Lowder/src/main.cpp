#include "LowUtilFileSystem.h"
#include <iostream>

#ifndef LOW_HIDE_WINDOWS_CONSOLE
#define LOW_HIDE_WINDOWS_CONSOLE 0
#endif

#if defined(_WIN32) && defined(_MSC_VER) && LOW_HIDE_WINDOWS_CONSOLE
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:mainCRTStartup")
#endif

#include "LowderSplash.h"
#include "LowderCrashReporter.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowRendererResourceImporter.h"
#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"
#include "LowUtilYaml.h"
#include "LowUtilName.h"
#include "LowUtilResource.h"
#include "LowUtilContainers.h"
#include "LowUtilGlobals.h"
#include "LowUtilString.h"
#include "LowUtilModuleInfo.h"

#include "LowRenderer.h"
#include "LowRendererRenderView.h"

#include "LowCore.h"
#include "LowCoreGameLoop.h"
#include "LowCoreScene.h"
#include "LowCoreRegion.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreMeshRenderer.h"
#include "LowCoreDirectionalLight.h"

#include <stdint.h>

#include "LowEditor.h"
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
using LibHandle = HMODULE;
inline LibHandle open_library(const char *path)
{
  return ::LoadLibraryA(path);
}
inline void close_library(LibHandle h)
{
  if (h)
    ::FreeLibrary(h);
}
inline void *get_symbol(LibHandle h, const char *name)
{
  return reinterpret_cast<void *>(::GetProcAddress(h, name));
}
inline std::string last_dl_error()
{
  DWORD e = ::GetLastError();
  if (!e)
    return {};
  LPVOID buf = nullptr;
  ::FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&buf, 0, nullptr);
  std::string msg = buf ? (char *)buf : "unknown error";
  if (buf)
    ::LocalFree(buf);
  return msg;
}
#define DYNLIB_EXT ".dll"
#define STDCALL __stdcall
#else
#include <dlfcn.h>
using LibHandle = void *;
inline LibHandle open_library(const char *path)
{
  // RTLD_NOW: resolve eagerly; switch to RTLD_LAZY if you prefer
  return ::dlopen(path, RTLD_NOW | RTLD_LOCAL);
}
inline void close_library(LibHandle h)
{
  if (h)
    ::dlclose(h);
}
inline void *get_symbol(LibHandle h, const char *name)
{
  ::dlerror(); // clear
  void *p = ::dlsym(h, name);
  (void)::dlerror(); // read & drop; callers can call last_dl_error if
                     // needed
  return p;
}
inline std::string last_dl_error()
{
  const char *e = ::dlerror();
  return e ? std::string(e) : std::string{};
}
#define DYNLIB_EXT ".so"
#define STDCALL /* nothing */
#endif

using f_funci = int(STDCALL *)();

enum class ModuleType
{
  Runtime,
  Editor
};

Low::Util::List<f_funci> g_EditorModuleInitialize;
Low::Util::List<f_funci> g_EditorModuleCleanup;

Low::Util::List<f_funci> g_RuntimeModuleInitialize;
Low::Util::List<f_funci> g_RuntimeModuleCleanup;

static void tick_splash_shutdown(float p_Delta,
                                 Low::Util::EngineState p_State)
{
  (void)p_Delta;
  (void)p_State;

  static uint32_t l_RenderedFrames = 0u;
  static bool l_SplashClosed = false;
  static bool l_MainWindowShown = false;
  if (l_SplashClosed) {
    return;
  }

  ++l_RenderedFrames;

  if (!l_MainWindowShown && l_RenderedFrames >= 3u) {
    Low::Util::Window::get_main_window().show();
    l_MainWindowShown = true;
  }

  if (l_RenderedFrames >= 5u) {
    Lowder::Splash::stop();
    Low::Util::Window::get_main_window().show();
    l_SplashClosed = true;
  }
}

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

static void setup_scene()
{
  Low::Core::Scene l_Scene =
      Low::Core::Scene::find_by_name(N(TestScene));
  if (l_Scene.is_alive()) {
    l_Scene.load();
  }
};

int load_module_lib(ModuleType type, const Low::Util::String &path)
{
  LOW_LOG_DEBUG << path << LOW_LOG_END;

  LibHandle lib = open_library(path.c_str());
  if (!lib) {
    LOW_LOG_WARN << "could not load the dynamic library: "
                 << last_dl_error() << LOW_LOG_END;
    return 1;
  }

  auto get_func = [&](const char *name) -> f_funci {
    auto p = reinterpret_cast<f_funci>(get_symbol(lib, name));
    if (!p) {
      LOW_LOG_WARN << "could not locate function '" << name
                   << "': " << last_dl_error() << LOW_LOG_END;
    }
    return p;
  };

  f_funci plugin_initialize = get_func("plugin_initialize");
  f_funci plugin_cleanup = get_func("plugin_cleanup");
  if (!plugin_initialize || !plugin_cleanup) {
    close_library(lib);
    return 1;
  }

  switch (type) {
  case ModuleType::Runtime:
    g_RuntimeModuleInitialize.push_back(plugin_initialize);
    g_RuntimeModuleCleanup.push_back(plugin_cleanup);
    break;
  case ModuleType::Editor:
    g_EditorModuleInitialize.push_back(plugin_initialize);
    g_EditorModuleCleanup.push_back(plugin_cleanup);
    break;
  default:
    LOW_LOG_FATAL << "Unknown module type" << LOW_LOG_END;
    close_library(lib);
    return 1;
  }

  auto l_GetInfo = reinterpret_cast<Low::Util::GetModuleInfoFunction>(
      get_symbol(lib, LOW_MODULE_INFO_FUNCTION_NAME));
  if (l_GetInfo) {
    Low::Util::register_module_info(l_GetInfo());
  }

  return 0;
}

ModuleType parse_module_type(Low::Util::String p_ModuleTypeString)
{
  if (p_ModuleTypeString == "runtime") {
    return ModuleType::Runtime;
  } else if (p_ModuleTypeString == "editor") {
    return ModuleType::Editor;
  }

  LOW_ASSERT(false, "Unknown module type string");

  return ModuleType::Runtime;
}

static Low::Util::String get_name_from_path(Low::Util::String p_Path)
{
  using namespace Low::Util;
  String l_Path = StringHelper::replace(p_Path, '\\', '/');
  List<String> l_Parts;
  StringHelper::split(l_Path, '/', l_Parts);

  return l_Parts[l_Parts.size() - 1];
}

static Low::Util::String
build_module_lib_path(Low::Util::String p_ProjectPath,
                      Low::Util::String p_ModulePath)
{
  Low::Util::String l_Path =
      Low::Util::get_executable_directory() + "/";
  if (Low::Util::get_project().engineSource ==
      Low::Util::EngineSource::Packaged) {
    l_Path = p_ProjectPath + "/bin/";
  }

#ifdef _WIN32
  l_Path += get_name_from_path(p_ModulePath);
  l_Path += ".dll";
#else
  l_Path += "lib/lib";
  l_Path += get_name_from_path(p_ModulePath);
  l_Path += ".so";
#endif

  return l_Path;
}

void load_module(Low::Util::String p_ProjectPath,
                 Low::Util::String p_Path)
{
  using namespace Low;

  LOW_LOG_INFO << "Loading module: " << p_Path << LOW_LOG_END;

  Util::String l_ModuleConfigPath = p_Path + "/module.yaml";
  Util::Serial::Node l_ConfigNode =
      Util::Serial::load_yaml_file(l_ModuleConfigPath.c_str());

  ModuleType l_ModuleType =
      parse_module_type(l_ConfigNode["type"].as<Low::Util::String>());

  Util::String l_DllPath =
      build_module_lib_path(p_ProjectPath, p_Path);
  load_module_lib(l_ModuleType, l_DllPath);
}

void load_project(Low::Util::String p_ProjectPath)
{
  using namespace Low;
  Util::String l_Path = p_ProjectPath + "/modules";

  Util::List<Util::String> l_FilePaths;

  Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);

  /*
  for (int i = 0; i < l_FilePaths.size(); ++i) {
    if (!Util::FileIO::is_directory(l_FilePaths[i].c_str())) {
      continue;
    }
    Util::String i_ModuleConfigPath =
        l_FilePaths[i] + "/module.yaml";
    if (!Util::FileIO::file_exists_sync(i_ModuleConfigPath.c_str())) {
      continue;
    }

    load_module(p_ProjectPath, l_FilePaths[i]);
  }
  */

  // HACK: We need some kind of dependency graph so we know which
  // modules to load first
  load_module(p_ProjectPath, p_ProjectPath + "/modules/Gameplay");
  load_module(p_ProjectPath, p_ProjectPath + "/modules/Editor");
}

int run_low(bool p_IsHost, Low::Util::String p_ProjectPath)
{
  Lowder::Splash::set_status("Initializing utility systems...");
  Low::Util::set_main_window_initially_hidden(true);
  Low::Util::Log::set_console_output_enabled(
      !LOW_HIDE_WINDOWS_CONSOLE);
  Low::Util::initialize(p_ProjectPath);

  Low::Util::Globals::set(N(IS_HOST), p_IsHost);

  Lowder::Splash::set_status("Loading project...");
  load_project(p_ProjectPath);
  // return 0;

  // All modules of a project share its build id
  {
    const Low::Util::Project &l_Project = Low::Util::get_project();
    const auto &l_Modules = Low::Util::get_module_infos();
    Lowder::CrashReporter::set_project_info(
        l_Project.name.c_str(), l_Project.version.c_str(),
        l_Modules.empty() ? "unknown"
                          : l_Modules[0].projectBuildId.c_str());
  }

  Lowder::Splash::set_status("Initializing renderer...");
  Low::Renderer::initialize();

  Lowder::Splash::set_status("Initializing core systems...");
  Low::Core::initialize();

  Low::Core::GameLoop::register_tick_callback(&Low::Editor::tick);
  Low::Core::GameLoop::register_late_tick_callback(
      &tick_splash_shutdown);

  // Mtd::initialize();

  for (int i = 0; i < g_RuntimeModuleInitialize.size(); ++i) {
    Lowder::Splash::set_status("Initializing runtime modules...");
    g_RuntimeModuleInitialize[i]();
  }

  Lowder::Splash::set_status("Setting up scene...");
  setup_scene();

  Lowder::Splash::set_status("Initializing editor...");
  Low::Editor::initialize();

  for (int i = 0; i < g_EditorModuleInitialize.size(); ++i) {
    Lowder::Splash::set_status("Initializing editor modules...");
    g_EditorModuleInitialize[i]();
  }

  Lowder::Splash::set_status("Loading user settings...");
  Low::Editor::load_user_settings();

  Lowder::Splash::set_status("Finalizing initialization...");

  Low::Core::GameLoop::start();

  // Mtd::cleanup();

  for (int i = 0; i < g_EditorModuleCleanup.size(); ++i) {
    g_EditorModuleCleanup[i]();
  }
  for (int i = 0; i < g_RuntimeModuleCleanup.size(); ++i) {
    g_RuntimeModuleCleanup[i]();
  }

  Low::Editor::cleanup();

  Low::Core::cleanup();

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}

// Read straight from project.yaml instead of through Util, so crashes
// during startup can already be uploaded
static void configure_crash_upload(const Low::Util::String &p_ProjectPath)
{
  try {
    const Low::Util::Yaml::Node l_Config = Low::Util::Yaml::load_file(
        (p_ProjectPath + "/project.yaml").c_str());
    const Low::Util::Yaml::Node l_CrashReporting =
        l_Config["crash_reporting"];
    if (!l_CrashReporting || !l_CrashReporting.IsMap()) {
      return;
    }

    std::string l_Url;
    std::string l_Token;
    if (l_CrashReporting["url"]) {
      l_Url = l_CrashReporting["url"].as<std::string>();
    }
    if (l_CrashReporting["token"]) {
      l_Token = l_CrashReporting["token"].as<std::string>();
    }
    Lowder::CrashReporter::set_upload_target(l_Url, l_Token);
  } catch (const std::exception &) {
    // A broken project.yaml is reported by Util::initialize, without
    // upload target the crash reporter only saves reports locally
  }
}

int main(int argc, char *argv[])
{
  bool l_IsHost = false;
  Low::Util::String l_ProjectPath = "";
  const bool l_HasArgument = argc > 1;
  if (l_HasArgument) {
    l_ProjectPath = argv[1];
  }
  if (argc > 2) {
    Low::Util::String l_Arg = argv[2];
    l_IsHost = l_Arg == "1";
  }

  if (!l_ProjectPath.empty()) {
    l_ProjectPath = Low::Util::resolve_project_path(l_ProjectPath);
  }
  if (l_ProjectPath.empty()) {
    assert(!l_HasArgument &&
           "The passed project path does not seem to be a valid "
           "LowEngine project.");
    l_ProjectPath = Low::Util::find_project_root(
        Low::Util::get_executable_directory());
  }
  if (l_ProjectPath.empty()) {
    l_ProjectPath = Low::Util::find_project_root(
        Low::Util::FileSystem::get_cwd());
  }

  // TODO: Replace with message box
  assert(!l_ProjectPath.empty() &&
         "Lowder started without a project.");

  // Util isn't initialized yet, so ask the lookup directly
  Low::Util::EngineSource l_EngineSource;
  const Low::Util::String l_EngineRoot =
      Low::Util::find_engine_root(&l_EngineSource);
  const Low::Util::String l_ImageDirectory =
      l_EngineRoot.empty() ? ""
                           : l_EngineRoot + "/LowData/editor/images";

  Lowder::CrashReporter::initialize(l_ProjectPath.c_str(),
                                    l_ImageDirectory.c_str());
  configure_crash_upload(l_ProjectPath);

  Lowder::Splash::set_source_build(l_EngineSource ==
                                   Low::Util::EngineSource::Source);
  Lowder::Splash::set_image_directory(l_ImageDirectory.c_str());

  Lowder::Splash::start();
  int l_Result = run_low(
      l_IsHost, Low::Util::PathHelper::normalize(l_ProjectPath));
  Lowder::Splash::stop();
  Lowder::CrashReporter::shutdown();
  return l_Result;
}
