#include "LowUtilFileSystem.h"

#include "LowUtilString.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilAssert.h"

#include "LowMath.h"

#include <filesystem>

namespace Low {
  namespace Util {
    namespace FileSystem {
      WatchHandle g_CurrentId = 0;

      Map<Util::String, WatchHandle> g_Handles;

      Map<WatchHandle, DirectoryWatcher> g_Directories;
      Map<WatchHandle, FileWatcher> g_Files;

      Util::String get_cwd()
      {
        std::filesystem::path l_Cwd = std::filesystem::current_path();

        StringBuilder l_Builder;
        l_Builder.append(l_Cwd.string().c_str());

        return l_Builder.get();
      }

      void update_directory(WatchHandle p_WatchHandle);

      static String get_name_from_path(String p_Path)
      {
        String l_Path = StringHelper::replace(p_Path, '\\', '/');
        List<String> l_Parts;
        StringHelper::split(l_Path, '/', l_Parts);

        return l_Parts[l_Parts.size() - 1];
      }

      WatchHandle watch_directory(
          String p_Path,
          Util::Function<Handle(FileWatcher &)> p_HandleCallback,
          float p_UpdateTimer)
      {
        auto l_HandlePos = g_Handles.find(p_Path);

        WatchHandle l_WatchHandle = 0;

        if (l_HandlePos == g_Handles.end()) {
          if (!Util::FileIO::file_exists_sync(p_Path.c_str())) {
            LOW_LOG_WARN
                << "Tried to watch directory that does not exist "
                << p_Path << LOW_LOG_END;
            return 0;
          }
          DirectoryWatcher l_DirectoryWatcher;
          l_DirectoryWatcher.watchHandle = ++g_CurrentId;
          l_DirectoryWatcher.path = p_Path;
          l_DirectoryWatcher.name = get_name_from_path(p_Path);
          l_DirectoryWatcher.updateTimer = p_UpdateTimer;
          l_DirectoryWatcher.currentUpdateTimer = 0.0f;
          l_DirectoryWatcher.update = true;
          l_DirectoryWatcher.handleCallback = p_HandleCallback;

          g_Directories[l_DirectoryWatcher.watchHandle] =
              l_DirectoryWatcher;
          g_Handles[p_Path] = l_DirectoryWatcher.watchHandle;

          l_WatchHandle = l_DirectoryWatcher.watchHandle;
          update_directory(l_WatchHandle);
        } else {
          l_WatchHandle = l_HandlePos->second;

          g_Directories[l_WatchHandle].updateTimer =
              LOW_MATH_MIN(g_Directories[l_WatchHandle].updateTimer,
                           p_UpdateTimer);
          g_Directories[l_WatchHandle].update = true;
        }

        return l_WatchHandle;
      }

      WatchHandle watch_file(
          String p_Path,
          Util::Function<Handle(FileWatcher &)> p_HandleCallback,
          float p_UpdateTimer)
      {
        auto l_HandlePos = g_Handles.find(p_Path);

        WatchHandle l_WatchHandle = 0;

        if (l_HandlePos == g_Handles.end()) {
          FileWatcher l_FileWatcher;
          l_FileWatcher.watchHandle = ++g_CurrentId;
          l_FileWatcher.path = p_Path;
          l_FileWatcher.name = get_name_from_path(p_Path);
          l_FileWatcher.currentUpdateTimer = p_UpdateTimer;
          l_FileWatcher.update = true;
          l_FileWatcher.handleCallback = p_HandleCallback;
          l_FileWatcher.handle = 0;
          l_FileWatcher.updateTimer = p_UpdateTimer;

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
        DirectoryWatcher &l_DirectoryWatcher =
            g_Directories[p_WatchHandle];

        // Resetting update timers
        l_DirectoryWatcher.update = false;
        l_DirectoryWatcher.lateUpdate = true;
        l_DirectoryWatcher.currentUpdateTimer =
            l_DirectoryWatcher.updateTimer;

        l_DirectoryWatcher.subdirectories.clear();
        l_DirectoryWatcher.files.clear();

        List<Util::String> l_Contents;
        FileIO::list_directory(l_DirectoryWatcher.path.c_str(),
                               l_Contents);

        std::sort(
            l_Contents.begin(), l_Contents.end(),
            [](const Util::String &a, const Util::String &b) {
              Util::String aName = a;
              Util::String bName = b;

              std::transform(
                  aName.begin(), aName.end(), aName.begin(),
                  [](unsigned char c) { return std::tolower(c); });

              std::transform(
                  bName.begin(), bName.end(), bName.begin(),
                  [](unsigned char c) { return std::tolower(c); });

              return aName < bName;
            });

        for (auto it = l_Contents.begin(); it != l_Contents.end();
             ++it) {
          String i_Path =
              StringHelper::replace(it->c_str(), '\\', '/');

          if (FileIO::is_directory(i_Path.c_str())) {
            WatchHandle i_WatchHandle = watch_directory(
                i_Path, l_DirectoryWatcher.handleCallback,
                l_DirectoryWatcher.updateTimer);
            DirectoryWatcher &i_NewDirectoryWatcher =
                get_directory_watcher(i_WatchHandle);
            i_NewDirectoryWatcher.parentWatchHandle =
                l_DirectoryWatcher.watchHandle;
            l_DirectoryWatcher.subdirectories.push_back(
                i_WatchHandle);
          } else {
            l_DirectoryWatcher.files.push_back(
                watch_file(i_Path, l_DirectoryWatcher.handleCallback,
                           l_DirectoryWatcher.updateTimer));
          }
        }
      }

      static void tick_directories(float p_Delta)
      {
        for (auto it = g_Directories.begin();
             it != g_Directories.end(); ++it) {
          if (it->second.update ||
              it->second.currentUpdateTimer <= 0.0f) {
            update_directory(it->first);
          } else {
            it->second.currentUpdateTimer -= p_Delta;
          }
        }
      }

      static bool update_file(WatchHandle p_WatchHandle)
      {
        FileWatcher &l_FileWatcher = g_Files[p_WatchHandle];

        if (!Util::FileIO::file_exists_sync(
                l_FileWatcher.path.c_str())) {
          return false;
        }

        // Resetting update timers
        l_FileWatcher.update = false;
        l_FileWatcher.currentUpdateTimer = l_FileWatcher.updateTimer;
        l_FileWatcher.modifiedTimestamp =
            Util::FileIO::modified_sync(l_FileWatcher.path.c_str());

        l_FileWatcher.handle =
            l_FileWatcher.handleCallback(l_FileWatcher);

        return true;
      }

      static void tick_files(float p_Delta)
      {
        for (auto it = g_Files.begin(); it != g_Files.end(); ++it) {
          if (it->second.update ||
              it->second.currentUpdateTimer <= 0.0f) {
            if (!update_file(it->first)) {
              // Removes files that are deleted
              it = g_Files.erase(it);
            }
          } else {
            it->second.currentUpdateTimer -= p_Delta;
          }
        }
      }

      static void late_update_directory(WatchHandle p_WatchHandle)
      {
        DirectoryWatcher &l_DirectoryWatcher =
            g_Directories[p_WatchHandle];

        // Resetting update timers
        l_DirectoryWatcher.lateUpdate = false;

        if (l_DirectoryWatcher.files.empty()) {
          return;
        }

        FileWatcher &l_FileWatcher =
            get_file_watcher(l_DirectoryWatcher.files[0]);

        if (l_FileWatcher.handle.get_id() == 0) {
          return;
        }

        RTTI::TypeInfo &l_TypeInfo =
            Handle::get_type_info(l_FileWatcher.handle.get_type());

        if (!l_TypeInfo.is_alive(l_FileWatcher.handle)) {
          return;
        }

        auto l_Pos = l_TypeInfo.properties.find(N(name));

        if (l_Pos == l_TypeInfo.properties.end()) {
          return;
        }

        std::sort(
            l_DirectoryWatcher.files.begin(),
            l_DirectoryWatcher.files.end(),
            [l_Pos](const WatchHandle a, const WatchHandle &b) {
              FileWatcher &l_af = get_file_watcher(a);
              FileWatcher &l_bf = get_file_watcher(b);

              String aName =
                  ((Name *)l_Pos->second.get(l_af.handle))->c_str();
              String bName =
                  ((Name *)l_Pos->second.get(l_bf.handle))->c_str();

              std::transform(
                  aName.begin(), aName.end(), aName.begin(),
                  [](unsigned char c) { return std::tolower(c); });

              std::transform(
                  bName.begin(), bName.end(), bName.begin(),
                  [](unsigned char c) { return std::tolower(c); });

              return aName < bName;
            });
      }

      static void late_tick_directories(float p_Delta)
      {
        for (auto it = g_Directories.begin();
             it != g_Directories.end(); ++it) {
          if (it->second.lateUpdate) {
            late_update_directory(it->first);
          }
        }
      }

      DirectoryWatcher &
      get_directory_watcher(WatchHandle p_WatchHandle)
      {
        auto l_Pos = g_Directories.find(p_WatchHandle);
        LOW_ASSERT(l_Pos != g_Directories.end(),
                   "Cannot find directorywatcher by watchhandle");
        return l_Pos->second;
      }

      bool file_watcher_exists(WatchHandle p_WatchHandle)
      {
        auto l_Pos = g_Files.find(p_WatchHandle);
        return l_Pos != g_Files.end();
      }

      FileWatcher &get_file_watcher(WatchHandle p_WatchHandle)
      {
        auto l_Pos = g_Files.find(p_WatchHandle);
        LOW_ASSERT(l_Pos != g_Files.end(),
                   "Cannot find filewatcher by watchhandle");
        return l_Pos->second;
      }

      void tick(float p_Delta)
      {
        tick_directories(p_Delta);
        tick_files(p_Delta);
        late_tick_directories(p_Delta);
      }
    } // namespace FileSystem
  }   // namespace Util
} // namespace Low
