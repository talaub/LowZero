#include "LowEditorFileSystem.h"

#include "LowUtilString.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilAssert.h"

#include "LowMath.h"

namespace Low {
  namespace Editor {
    namespace FileSystem {
      WatchHandle g_CurrentId = 0;

      Util::Map<Util::String, WatchHandle> g_Handles;

      Util::Map<WatchHandle, DirectoryWatcher> g_Directories;
      Util::Map<WatchHandle, FileWatcher> g_Files;

      static Util::String get_name_from_path(Util::String p_Path)
      {
        Util::String l_Path = Util::StringHelper::replace(p_Path, '\\', '/');
        Util::List<Util::String> l_Parts;
        Util::StringHelper::split(l_Path, '/', l_Parts);

        return l_Parts[l_Parts.size() - 1];
      }

      WatchHandle watch_directory(Util::String p_Path, float p_UpdateTimer)
      {
        auto l_HandlePos = g_Handles.find(p_Path);

        WatchHandle l_WatchHandle = 0;

        if (l_HandlePos == g_Handles.end()) {
          DirectoryWatcher l_DirectoryWatcher;
          l_DirectoryWatcher.watchHandle = g_CurrentId++;
          l_DirectoryWatcher.path = p_Path;
          l_DirectoryWatcher.name = get_name_from_path(p_Path);
          l_DirectoryWatcher.updateTimer = p_UpdateTimer;
          l_DirectoryWatcher.currentUpdateTimer = 0.0f;
          l_DirectoryWatcher.update = true;

          g_Directories[l_DirectoryWatcher.watchHandle] = l_DirectoryWatcher;
          g_Handles[p_Path] = l_DirectoryWatcher.watchHandle;

          l_WatchHandle = l_DirectoryWatcher.watchHandle;
        } else {
          l_WatchHandle = l_HandlePos->second;

          g_Directories[l_WatchHandle].updateTimer = LOW_MATH_MIN(
              g_Directories[l_WatchHandle].updateTimer, p_UpdateTimer);
          g_Directories[l_WatchHandle].update = true;
        }

        return l_WatchHandle;
      }

      WatchHandle watch_file(Util::String p_Path, float p_UpdateTimer)
      {
        auto l_HandlePos = g_Handles.find(p_Path);

        WatchHandle l_WatchHandle = 0;

        if (l_HandlePos == g_Handles.end()) {
          FileWatcher l_FileWatcher;
          l_FileWatcher.watchHandle = g_CurrentId++;
          l_FileWatcher.path = p_Path;
          l_FileWatcher.name = get_name_from_path(p_Path);

          g_Files[l_FileWatcher.watchHandle] = l_FileWatcher;
          g_Handles[p_Path] = l_FileWatcher.watchHandle;

          l_WatchHandle = l_FileWatcher.watchHandle;
        } else {
          l_WatchHandle = l_HandlePos->second;
        }

        return l_WatchHandle;
      }

      static void update_directory(WatchHandle p_WatchHandle)
      {
        DirectoryWatcher &l_DirectoryWatcher = g_Directories[p_WatchHandle];

        // Resetting update timers
        l_DirectoryWatcher.update = false;
        l_DirectoryWatcher.currentUpdateTimer = l_DirectoryWatcher.updateTimer;

        l_DirectoryWatcher.subdirectories.clear();
        l_DirectoryWatcher.files.clear();

        Util::List<Util::String> l_Contents;
        Util::FileIO::list_directory(l_DirectoryWatcher.path.c_str(),
                                     l_Contents);

        for (auto it = l_Contents.begin(); it != l_Contents.end(); ++it) {
          if (Util::FileIO::is_directory(it->c_str())) {
            l_DirectoryWatcher.subdirectories.push_back(
                watch_directory(*it, l_DirectoryWatcher.updateTimer));
          } else {
            l_DirectoryWatcher.files.push_back(
                watch_file(*it, l_DirectoryWatcher.updateTimer));
          }
        }
      }

      static void tick_directories(float p_Delta)
      {
        for (auto it = g_Directories.begin(); it != g_Directories.end(); ++it) {
          if (it->second.update || it->second.currentUpdateTimer <= 0.0f) {
            update_directory(it->first);
          } else {
            it->second.currentUpdateTimer -= p_Delta;
          }
        }
      }

      DirectoryWatcher &get_directory_watcher(WatchHandle p_WatchHandle)
      {
        // TODO: Insert check
        return g_Directories[p_WatchHandle];
      }

      FileWatcher &get_file_watcher(WatchHandle p_WatchHandle)
      {
        // TODO: Insert check
        return g_Files[p_WatchHandle];
      }

      void tick(float p_Delta)
      {
        tick_directories(p_Delta);
      }
    } // namespace FileSystem
  }   // namespace Editor
} // namespace Low
