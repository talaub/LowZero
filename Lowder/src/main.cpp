#include <iostream>

#include "LowMath.h"
#include "LowMathVectorUtil.h"

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
#include "LowRendererExposedObjects.h"
#include "LowRendererRenderFlow.h"

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

#include <windows.h>

typedef int(__stdcall *f_funci)();

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
  l_Scene.load();
};

void print_last_error()
{
  DWORD errorMessageID = ::GetLastError();
  if (errorMessageID == 0) {
    return; // No error message has been recorded
  }

  LPSTR messageBuffer = nullptr;
  size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&messageBuffer, 0, NULL);

  std::string message(messageBuffer, size);

  // Free the buffer.
  LocalFree(messageBuffer);

  LOW_LOG_FATAL << message << LOW_LOG_END;
}

int load_module_dll(ModuleType p_ModuleType, Low::Util::String p_Path)
{

  LOW_LOG_DEBUG << p_Path << LOW_LOG_END;
  /*
  HINSTANCE hGetProcIDDLL =
      LoadLibrary("../../../misteda/build/Debug/app/Gameplayd.dll");
      */
  HINSTANCE hGetProcIDDLL = LoadLibrary(p_Path.c_str());

  if (!hGetProcIDDLL) {
    LOW_LOG_WARN << "could not load the dynamic library"
                 << LOW_LOG_END;
    print_last_error();
    return 1;
  }

  // resolve function address here
  f_funci plugin_initialize =
      (f_funci)GetProcAddress(hGetProcIDDLL, "plugin_initialize");
  if (!plugin_initialize) {
    LOW_LOG_WARN << "could not locate the initialize function"
                 << LOW_LOG_END;
    return 1;
  }

  f_funci plugin_cleanup =
      (f_funci)GetProcAddress(hGetProcIDDLL, "plugin_cleanup");
  if (!plugin_cleanup) {
    LOW_LOG_WARN << "could not locate the cleanup function"
                 << LOW_LOG_END;
    return 1;
  }

  switch (p_ModuleType) {
  case ModuleType::Runtime: {
    g_RuntimeModuleInitialize.push_back(plugin_initialize);
    g_RuntimeModuleCleanup.push_back(plugin_cleanup);
    break;
  }
  case ModuleType::Editor: {
    g_EditorModuleInitialize.push_back(plugin_initialize);
    g_EditorModuleCleanup.push_back(plugin_cleanup);
    break;
  }
  default:
    LOW_LOG_FATAL << "Unknown module type" << LOW_LOG_END;
    break;
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

void load_module(Low::Util::String p_ProjectPath,
                 Low::Util::String p_Path)
{
  using namespace Low;

  LOW_LOG_INFO << "Load module: " << p_Path << LOW_LOG_END;

  Util::String l_ModuleConfigPath = p_Path + "\\module.yaml";
  Util::Yaml::Node l_ConfigNode =
      Util::Yaml::load_file(l_ModuleConfigPath.c_str());

  ModuleType l_ModuleType =
      parse_module_type(LOW_YAML_AS_STRING(l_ConfigNode["type"]));

  Util::String l_DllPath = p_ProjectPath + "\\bin";

#ifdef NDEBUG
  l_DllPath += "\\Release\\";
#else
  l_DllPath += "\\Debug\\";
#endif

  l_DllPath += get_name_from_path(p_Path);
  l_DllPath += ".dll";
  load_module_dll(l_ModuleType, l_DllPath);
}

void load_project(Low::Util::String p_ProjectPath)
{
  using namespace Low;
  Util::String l_Path = p_ProjectPath + "\\modules";

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

  // TODO: We need some kind of dependency graph so we know which
  // modules to load first
  load_module(p_ProjectPath, "P:/misteda/modules/Gameplay");
  load_module(p_ProjectPath, "P:/misteda/modules/Editor");
}

int run_low(bool p_IsHost, Low::Util::String p_ProjectPath)
{
  Low::Util::initialize();

  Low::Util::Globals::set(N(IS_HOST), p_IsHost);

  load_project(p_ProjectPath);
  // return 0;

  Low::Renderer::initialize();

  Low::Core::initialize();

  Low::Core::GameLoop::register_tick_callback(&Low::Editor::tick);

  // Mtd::initialize();

  for (int i = 0; i < g_RuntimeModuleInitialize.size(); ++i) {
    g_RuntimeModuleInitialize[i]();
  }

  setup_scene();

  Low::Editor::initialize();

  for (int i = 0; i < g_EditorModuleInitialize.size(); ++i) {
    g_EditorModuleInitialize[i]();
  }

  Low::Core::GameLoop::start();

  // Mtd::cleanup();

  for (int i = 0; i < g_EditorModuleCleanup.size(); ++i) {
    g_EditorModuleCleanup[i]();
  }
  for (int i = 0; i < g_RuntimeModuleCleanup.size(); ++i) {
    g_RuntimeModuleCleanup[i]();
  }

  Low::Core::cleanup();

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}

int main(int argc, char *argv[])
{
  bool l_IsHost = false;
  Low::Util::String l_ProjectPath = "";
  if (argc > 1) {
    l_ProjectPath = argv[1];
  }
  if (argc > 2) {
    Low::Util::String l_Arg = argv[2];
    l_IsHost = l_Arg == "1";
  }
  return run_low(l_IsHost, l_ProjectPath);
}
