#pragma once

#include "LowCoreApi.h"

#include "LowCoreScene.h"

#include "LowUtilEnums.h"

namespace Low {
  namespace Core {
    LOW_CORE_API void initialize();
    LOW_CORE_API void cleanup();

    LOW_CORE_API Util::EngineState get_engine_state();
    LOW_CORE_API void begin_playmode();
    LOW_CORE_API void exit_playmode();

    LOW_CORE_API Scene get_loaded_scene();
    LOW_CORE_API void load_scene(Scene p_Scene);

    void test_mono();
  } // namespace Core
} // namespace Low
