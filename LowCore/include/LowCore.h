#pragma once

#include "LowCoreApi.h"

#include "LowCoreScene.h"

#include "LowUtilEnums.h"
#include "LowUtilFileSystem.h"

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

    struct FileSystemWatchers
    {
      Util::FileSystem::WatchHandle scriptDirectory;

      Util::FileSystem::WatchHandle meshAssetDirectory;
      Util::FileSystem::WatchHandle materialAssetDirectory;
      Util::FileSystem::WatchHandle prefabAssetDirectory;

      Util::FileSystem::WatchHandle meshResourceDirectory;
    };

    LOW_CORE_API FileSystemWatchers &get_filesystem_watchers();
    LOW_CORE_API Util::FileSystem::WatchHandle
    get_filesystem_watcher(uint16_t p_Type);
  } // namespace Core
} // namespace Low
