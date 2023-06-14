#include "MtdPlugin.h"

#include "MtdCameraController.h"
#include "MtdTestSystem.h"

#include "LowCoreGameLoop.h"

#include "LowUtilLogger.h"

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

namespace Mtd {
  static void tick(float p_Delta, Low::Util::EngineState p_State)
  {
    System::Test::tick(p_Delta, p_State);
  }

  static void initialize_component_types()
  {
    Component::CameraController::initialize();
  }

  static void initialize_types()
  {
    initialize_component_types();
  }

  void initialize()
  {
    initialize_types();

    Low::Core::GameLoop::register_tick_callback(&tick);
  }

  static void cleanup_component_types()
  {
    Component::CameraController::cleanup();
  }

  static void cleanup_types()
  {
    cleanup_component_types();
  }

  void cleanup()
  {
    cleanup_types();
  }
} // namespace Mtd
