#include "LowUtilAssetManager.h"

#include "LowUtilFileSystem.h"
#include "LowUtil.h"

#define EVENT_DEBOUNCE_TIME 2.0f

namespace Low {
  namespace Util {
    namespace AssetManager {
      List<TypeRegistrator> g_AssetTypes;

      FileSystem::Watcher g_DataWatcher;

      struct FileEvent
      {
        FileSystem::Watcher::Event fsEvent;
        float timer;
      };

      List<FileEvent> g_QueuedEvents;

      static void initialize_assets(const TypeRegistrator p_AssetType)
      {
        List<String> l_Paths;
        for (auto it : p_AssetType.initializeDirectories) {
          for (auto sit : p_AssetType.assetSuffixes) {
            Util::FileSystem::collect_files_with_suffix(
                it.path.c_str(), sit.c_str(), l_Paths, it.recursive);
          }
        }

        for (auto it : l_Paths) {
          p_AssetType.initializer(it);
          // TODO: Register?
        }
      }

      void register_asset_type(const TypeRegistrator &p_Registrator)
      {
        g_AssetTypes.push_back(p_Registrator);

        if (p_Registrator.initializeOnStartup) {
          initialize_assets(p_Registrator);
        }
      }

      void initialize()
      {
        LOW_ASSERT(g_DataWatcher.start(get_project().dataPath),
                   "Failed to start raw asset file watcher.");
      }

      void cleanup()
      {
        g_DataWatcher.stop();
      }

      static void
      handle_file_event(FileSystem::Watcher::Event p_Event)
      {
        const String l_FullEventPath = get_project().dataPath + '/' +
                                       p_Event.path.string().c_str();

        for (auto i_Type : g_AssetTypes) {
          bool i_IsImport = false;

          for (auto i_RawSuffix : i_Type.rawSuffixes) {
            if (i_RawSuffix ==
                String(p_Event.path.extension().string().c_str())) {
              i_IsImport = true;
              break;
            }
          }

          if (i_IsImport) {
            for (auto i_ImportDir : i_Type.importDirectories) {
              if (!i_ImportDir.autoscan) {
                continue;
              }

              const bool i_IsValidImportDir =
                  FileSystem::is_file_in_directory(
                      std::filesystem::path(l_FullEventPath.c_str()),
                      std::filesystem::path(i_ImportDir.path.c_str()),
                      i_ImportDir.recursive);

              if (i_IsValidImportDir) {
                if (p_Event.type ==
                    FileSystem::Watcher::EventType::Removed) {
                  i_Type.rawDeleter(l_FullEventPath);
                } else {
                  const Util::String i_ImportedPath = i_Type.importer(
                      Util::String(p_Event.path.string().c_str()));

                  if (!i_ImportedPath.empty()) {
                    // TODO: Register?
                    i_Type.initializer(i_ImportedPath);
                  }
                }

                break;
              }
            }
          }
        }
      }

      static void poll_watcher(const float p_Delta)
      {

        auto l_Events = g_DataWatcher.poll();

        for (auto &i_Event : l_Events) {
          if (i_Event.type ==
              Util::FileSystem::Watcher::EventType::Overflow) {
            continue;
          }

          bool i_AddEvent = true;

          for (auto &i_QueuedEvent : g_QueuedEvents) {
            if (i_QueuedEvent.fsEvent.path == i_Event.path) {

              if (i_QueuedEvent.fsEvent.type == i_Event.type) {
                i_AddEvent = false;
                break;
              }

              if (i_QueuedEvent.fsEvent.type ==
                      FileSystem::Watcher::EventType::Added &&
                  i_Event.type ==
                      FileSystem::Watcher::EventType::Modified) {
                i_AddEvent = false;
                break;
              }
            }
          }

          if (i_AddEvent) {
            g_QueuedEvents.push_back({i_Event, EVENT_DEBOUNCE_TIME});
          }
        }
      }

      static void tick_scheduled_events(const float p_Delta)
      {
        for (auto it = g_QueuedEvents.begin();
             it != g_QueuedEvents.end();) {
          it->timer -= p_Delta;

          if (it->timer <= 0.0f) {
            handle_file_event(it->fsEvent);
            it = g_QueuedEvents.erase(it);

          } else {
            ++it;
          }
        }
      }

      void tick(const float p_Delta)
      {
        tick_scheduled_events(p_Delta);

        poll_watcher(p_Delta);
      }
    } // namespace AssetManager
  } // namespace Util
} // namespace Low
