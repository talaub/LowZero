#include "imgui.h"

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

#include <microprofile.h>
#include <vector>

#include "LowEditorMainWindow.h"

#include "MtdPlugin.h"

#if defined(LOW_USE_CRASHREPORTER)
#if defined(_WINDOWS)
#include <windows.h>
#include <errhandlingapi.h>
#include <excpt.h>
#include <iostream>
#include <cstdlib>
#include <exception>
#include <dbghelp.h>
#include <winternl.h>
#pragma comment(lib, "dbghelp.lib")
#endif
#endif

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char *pName, int flags, unsigned debugFlags,
                     const char *file, int line)
{
  return malloc(size);
}

static void setup_scene()
{
  Low::Core::Scene l_Scene = Low::Core::Scene::find_by_name(N(TestScene));
  l_Scene.load();
};

int run_low()
{
  Low::Util::initialize();

  Low::Renderer::initialize();

  Low::Core::initialize();

  setup_scene();

  Low::Core::GameLoop::register_tick_callback(&Low::Editor::tick);

  Mtd::initialize();

  Low::Editor::initialize();

  Low::Core::GameLoop::start();

  Mtd::cleanup();

  Low::Core::cleanup();

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}

#if defined(LOW_USE_CRASHREPORTER)
void generate_minidump(EXCEPTION_POINTERS *pExceptionPointers)
{
  HANDLE hDumpFile = CreateFile("minidump.dmp", GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hDumpFile != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = pExceptionPointers;
    exceptionInfo.ClientPointers = FALSE;

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile,
                      MiniDumpNormal, &exceptionInfo, NULL, NULL);

    CloseHandle(hDumpFile);
  }
}

LONG WINAPI on_crash(EXCEPTION_POINTERS *p_ExceptInfo)
{
  generate_minidump(p_ExceptInfo);

  return EXCEPTION_CONTINUE_SEARCH;
}

void setup_crash_reporter()
{
  SetUnhandledExceptionFilter(on_crash);
}
#endif

int main()
{

#if defined(LOW_USE_CRASHREPORTER)
  setup_crash_reporter();
#endif

  return run_low();
}
