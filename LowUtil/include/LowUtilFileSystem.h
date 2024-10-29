#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Util {
    namespace FileSystem {
      typedef uint64_t WatchHandle;

      Util::String LOW_EXPORT get_cwd();

      struct FileWatcher
      {
        WatchHandle watchHandle;
        String name;
        String path;
        bool update;
        float currentUpdateTimer;
        float updateTimer;
        uint64_t modifiedTimestamp;
        Util::Function<Handle(FileWatcher &)> handleCallback;
        Util::Handle handle;
      };

      struct DirectoryWatcher
      {
        WatchHandle watchHandle;
        String name;
        String path;
        List<WatchHandle> files;
        List<WatchHandle> subdirectories;
        bool update;
        bool lateUpdate;
        float currentUpdateTimer;
        float updateTimer;
        WatchHandle parentWatchHandle;
        Util::Function<Handle(FileWatcher &)> handleCallback;
      };

      void tick(float p_Delta);

      WatchHandle LOW_EXPORT watch_directory(
          String p_Path,
          Util::Function<Handle(FileWatcher &)> p_HandleCallback,
          float p_UpdateTimer = 3.0f);

      WatchHandle LOW_EXPORT watch_file(
          String p_Path,
          Util::Function<Handle(FileWatcher &)> p_HandleCallback,
          float p_UpdateTimer = 3.0f);

      DirectoryWatcher LOW_EXPORT &
      get_directory_watcher(WatchHandle p_WatchHandle);
      FileWatcher LOW_EXPORT &
      get_file_watcher(WatchHandle p_WatchHandle);
      bool LOW_EXPORT file_watcher_exists(WatchHandle p_WatchHandle);
    } // namespace FileSystem
  }   // namespace Util
} // namespace Low
