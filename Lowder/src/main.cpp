#include <iostream>

#include "LowderSplash.h"

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

  // keep 'lib' around if you need it later; if you close it now,
  // function ptrs go invalid. Store LibHandle in a vector/map keyed
  // by module if you need later cleanup.
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

void load_module(Low::Util::String p_ProjectPath,
                 Low::Util::String p_Path)
{
  using namespace Low;

  LOW_LOG_INFO << "Load module: " << p_Path << LOW_LOG_END;

  Util::String l_ModuleConfigPath = p_Path + "/module.yaml";
  Util::Serial::Node l_ConfigNode =
      Util::Serial::load_yaml_file(l_ModuleConfigPath.c_str());

  ModuleType l_ModuleType =
      parse_module_type(l_ConfigNode["type"].as<Low::Util::String>());

  Util::String l_DllPath = p_ProjectPath + "/bin";

#ifdef NDEBUG
  l_DllPath += "/RelWithDebInfo/";
#else
  l_DllPath += "/Debug/";
#endif

  l_DllPath += get_name_from_path(p_Path);
#ifdef _WIN32
  l_DllPath += ".dll";
#else
  l_DllPath += ".so";
#endif
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
      l_FilePaths[i] + "\\module.yaml";
  if (!Util::FileIO::file_exists_sync(i_ModuleConfigPath.c_str())) {
    continue;
  }

  load_module(p_ProjectPath, l_FilePaths[i]);
}
*/

// HACK: We need some kind of dependency graph so we know which
// modules to load first
#if RELEASE_BUILD
  load_module(p_ProjectPath, "./modules/Gameplay");
  load_module(p_ProjectPath, "./modules/Editor");
#else
  load_module(
      p_ProjectPath,
      "C:/Users/tlaub/Documents/LowEngine/misteda/modules/Gameplay");
  load_module(
      p_ProjectPath,
      "C:/Users/tlaub/Documents/LowEngine/misteda/modules/Editor");
#endif
}

int run_low(bool p_IsHost, Low::Util::String p_ProjectPath)
{
  Lowder::Splash::set_status("Initializing utility systems...");
  Low::Util::initialize();

  Low::Util::Globals::set(N(IS_HOST), p_IsHost);

  Lowder::Splash::set_status("Loading project...");
  load_project(p_ProjectPath);
  // return 0;

  Lowder::Splash::set_status("Initializing renderer...");
  Low::Renderer::initialize();

  Lowder::Splash::set_status("Initializing core systems...");
  Low::Core::initialize();

  Low::Core::GameLoop::register_tick_callback(&Low::Editor::tick);

  if (0) {
    Low::Renderer::ResourceImporter::import_font("C:\\roboto.ttf",
                                                 "roboto");
    Low::Util::cleanup();
  }

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

  Lowder::Splash::stop();

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

int main(int argc, char *argv[])
{
  Lowder::Splash::start();

  bool l_IsHost = false;
  Low::Util::String l_ProjectPath = "";
  if (argc > 1) {
    l_ProjectPath = argv[1];
  }
  if (argc > 2) {
    Low::Util::String l_Arg = argv[2];
    l_IsHost = l_Arg == "1";
  }
#if RELEASE_BUILD
  l_IsHost = true;
  l_ProjectPath = "./";
#endif
  int l_Result = run_low(l_IsHost, l_ProjectPath);
  Lowder::Splash::stop();
  return l_Result;
}
