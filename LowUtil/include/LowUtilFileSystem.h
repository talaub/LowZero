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
        String trimmedPath;
        bool update;
        float currentUpdateTimer;
        float updateTimer;
        uint64_t modifiedTimestamp;
        Util::Function<Handle(FileWatcher &)> handleCallback;
        Util::Handle handle;
        bool hidden;
        String nameClean;
        String subtype;
        String extension;
        u64 subtypeHash;
        String nameCleanPrettified;
        u32 typeEnum;
        u64 assetId;
      };

      struct DirectoryWatcher
      {
        WatchHandle watchHandle;
        String name;
        String path;
        String trimmedPath;
        String cleanedPath;
        List<WatchHandle> files;
        List<WatchHandle> subdirectories;
        bool update;
        bool lateUpdate;
        float currentUpdateTimer;
        float updateTimer;
        WatchHandle parentWatchHandle;
        bool hidden;
        Util::Function<Handle(FileWatcher &)> handleCallback;
        u32 trimAmount;
      };

      void tick(float p_Delta);

      WatchHandle LOW_EXPORT watch_directory(
          String p_Path,
          Util::Function<Handle(FileWatcher &)> p_HandleCallback,
          float p_UpdateTimer = 3.0f, const u32 p_TrimCount = 0);

      WatchHandle LOW_EXPORT watch_file(
          String p_Path,
          Util::Function<Handle(FileWatcher &)> p_HandleCallback,
          float p_UpdateTimer = 3.0f, const u32 p_TrimCount = 0);

      DirectoryWatcher LOW_EXPORT &
      get_directory_watcher(WatchHandle p_WatchHandle);
      FileWatcher LOW_EXPORT &
      get_file_watcher(WatchHandle p_WatchHandle);
      bool LOW_EXPORT file_watcher_exists(WatchHandle p_WatchHandle);

      List<String> LOW_EXPORT get_files_with_suffix(
          const char *p_DirectoryPath, const char *p_Suffix);
      void LOW_EXPORT collect_files_with_suffix(
          const char *p_DirectoryPath, const char *p_Suffix,
          List<String> &p_OutFiles);
    } // namespace FileSystem
  } // namespace Util
} // namespace Low
