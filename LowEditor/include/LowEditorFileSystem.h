#pragma once

#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    namespace FileSystem {
      typedef uint64_t WatchHandle;

      struct FileWatcher
      {
        WatchHandle watchHandle;
        Util::String name;
        Util::String path;
      };

      struct DirectoryWatcher
      {
        WatchHandle watchHandle;
        Util::String name;
        Util::String path;
        Util::List<WatchHandle> files;
        Util::List<WatchHandle> subdirectories;
        bool update;
        float currentUpdateTimer;
        float updateTimer;
      };

      void tick(float p_Delta);

      WatchHandle watch_directory(Util::String p_Path,
                                  float p_UpdateTimer = 3.0f);

      WatchHandle watch_file(Util::String p_Path, float p_UpdateTimer = 3.0f);

      DirectoryWatcher &get_directory_watcher(WatchHandle p_WatchHandle);
      FileWatcher &get_file_watcher(WatchHandle p_WatchHandle);
    } // namespace FileSystem
  }   // namespace Editor
} // namespace Low
