#pragma once

#include "LowCoreScene.h"
#include "LowCoreRegion.h"

namespace Low {
  namespace Editor {
    namespace SaveHelper {
      void save_region(Core::Region p_Region, bool p_ShowMessage = true);

      void save_scene(Core::Scene p_Scene, bool p_ShowMessage = true);
    } // namespace SaveHelper
  }   // namespace Editor
} // namespace Low
