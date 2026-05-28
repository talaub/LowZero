#include "LowUtilAssetManager.h"

#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilFileSystem.h"
#include "LowUtil.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "LowUtilSerialization.h"
#include "LowUtilString.h"
#include <filesystem>

#define EVENT_DEBOUNCE_TIME 2.0f

#define AM_LOG_ERROR LOW_LOG_ERROR << "[AssetManager] "
#define AM_LOG_WARN LOW_LOG_WARN << "[AssetManager] "
#define AM_LOG_DEBUG LOW_LOG_DEBUG << "[AssetManager] "
#define AM_LOG_INFO LOW_LOG_INFO << "[AssetManager] "

namespace Low {
  namespace Util {
    List<AssetManager::TypeRegistrator> g_AssetTypes;

    struct AssetRecord
    {
      String path;
      Handle handle;
      u64 unique_id;
      Util::Name name;

      AssetRecord()
      {
      }
      AssetRecord(Handle h, const String p) : path(p), handle(h)
      {
        if (Handle::is_registered_type(h.get_type())) {
          RTTI::TypeInfo &l_TypeInfo =
              Handle::get_type_info(h.get_type());
          {
            auto l_Pos = l_TypeInfo.properties.find(N(unique_id));
            if (l_Pos != l_TypeInfo.properties.end()) {
              l_Pos->second.get(h, &unique_id);
            }
          }
          {
            auto l_Pos = l_TypeInfo.properties.find(N(name));
            if (l_Pos != l_TypeInfo.properties.end()) {
              l_Pos->second.get(h, &name);
            }
          }
        }
      }
    };

    List<AssetRecord> g_AssetRecords;

    FileSystem::Watcher g_DataWatcher;

    static String normalize_asset_path(const String &p_Path)
    {
      return PathHelper::normalize(p_Path);
    }

    static AssetRecord *
    find_asset_record_by_path(const String &p_Path)
    {
      const String l_Path = normalize_asset_path(p_Path);
      for (AssetRecord &i_Record : g_AssetRecords) {
        if (i_Record.path == l_Path) {
          return &i_Record;
        }
      }

      return nullptr;
    }

    static AssetRecord *find_primary_asset_record(Handle p_Handle)
    {
      for (AssetRecord &i_Record : g_AssetRecords) {
        if (i_Record.handle == p_Handle) {
          return &i_Record;
        }
      }

      return nullptr;
    }

    static void upsert_asset_record(const AssetRecord &p_Record)
    {
      if (AssetRecord *i_Record =
              find_asset_record_by_path(p_Record.path)) {
        *i_Record = p_Record;
        return;
      }

      g_AssetRecords.push_back(p_Record);
    }

    struct LoadEntry
    {
      String path;
      AssetManager::LoadPriority priority;
      Handle handle;
    };

    static auto g_LoadEntryComparator = [](const LoadEntry &p_A,
                                           const LoadEntry &p_B) {
      return p_A.priority > p_B.priority;
    };

    static List<LoadEntry> g_LoadEntriesContainer;

    static PriorityQueue<LoadEntry, decltype(g_LoadEntryComparator)>
        g_LoadEntries(g_LoadEntryComparator, g_LoadEntriesContainer);

    struct FileEvent
    {
      FileSystem::Watcher::Event fsEvent;
      float timer;
    };

    List<FileEvent> g_QueuedEvents;

    static bool
    find_asset_type(const u16 p_TypeId,
                    AssetManager::TypeRegistrator &p_OutAssetType)
    {
      for (AssetManager::TypeRegistrator &i_AssetType :
           g_AssetTypes) {
        if (i_AssetType.typeId == p_TypeId) {
          p_OutAssetType = i_AssetType;
          return true;
        }
      }

      return false;
    }

    static void
    initialize_asset(const AssetManager::TypeRegistrator &p_AssetType,
                     const Util::String p_Path)
    {
      if (p_AssetType.initializer) {
        Util::Handle l_Handle = p_AssetType.initializer(p_Path);

        AssetManager::_register(l_Handle, p_Path);
      } else {
        LOW_LOG_ERROR << "Failed to initialize " << p_AssetType.name
                      << " using path " << p_Path << LOW_LOG_END;
        LOW_ASSERT(
            false,
            "Trying to initialize asset put no initializer found.");
      }
    }

    static void
    import(const AssetManager::TypeRegistrator &p_AssetType,
           const String &p_Path)
    {
      const bool l_IsAssetPath =
          Util::FileSystem::is_file_in_directory(
              p_Path, Util::project_asset_cache_path());
      const bool l_IsManaged = Util::FileSystem::is_file_in_directory(
          p_Path, Util::project_managed_path());
      const bool l_IsVsOut = Util::FileSystem::is_file_in_directory(
          p_Path, Util::project_visual_script_out_path());
      const bool l_IsEditorImage =
          Util::FileSystem::is_file_in_directory(
              p_Path, Util::project_editor_images_path());

      if (l_IsAssetPath || l_IsManaged || l_IsVsOut ||
          l_IsEditorImage) {
        return;
      }

      AM_LOG_DEBUG << "Importing " << p_AssetType.name << " from '"
                   << p_Path << "'" << LOW_LOG_END;
      const Util::String l_ImportedPath =
          p_AssetType.importer(Util::String(p_Path.c_str()));

      if (!l_ImportedPath.empty()) {
        AM_LOG_DEBUG << "Import successful" << LOW_LOG_END;
        initialize_asset(p_AssetType, l_ImportedPath);
      } else {
        AM_LOG_DEBUG << "Import aborted" << LOW_LOG_END;
      }
    }

    static void
    initialize_assets(const AssetManager::TypeRegistrator p_AssetType)
    {
      List<String> l_Paths;
      for (auto it : p_AssetType.initializeDirectories) {
        for (auto sit : p_AssetType.assetSuffixes) {
          Util::FileSystem::collect_files_with_suffix(
              it.path.c_str(), sit.c_str(), l_Paths, it.recursive);
        }
      }

      RTTI::TypeInfo l_Info =
          Handle::get_type_info(p_AssetType.typeId);

      for (auto it : l_Paths) {

        initialize_asset(p_AssetType, it);
      }
    }

    void AssetManager::register_asset_type(
        const AssetManager::TypeRegistrator &p_Registrator)
    {
      g_AssetTypes.push_back(p_Registrator);

      if (p_Registrator.initializeOnStartup) {
        initialize_assets(p_Registrator);
      }

      if (p_Registrator.importOnStartup) {
        List<String> l_Paths;
        for (auto it : p_Registrator.importDirectories) {
          for (auto sit : p_Registrator.rawSuffixes) {
            Util::FileSystem::collect_files_with_suffix(
                it.path.c_str(), sit.c_str(), l_Paths, it.recursive);
          }
        }

        for (auto i_Path : l_Paths) {
          import(p_Registrator, PathHelper::normalize(i_Path));
        }
      }
    }

    void AssetManager::initialize()
    {
      LOW_ASSERT(g_DataWatcher.start(get_project().dataPath),
                 "Failed to start raw asset file watcher.");
    }

    void AssetManager::cleanup()
    {
      g_DataWatcher.stop();
    }

    static void handle_file_event(FileSystem::Watcher::Event p_Event)
    {
      const String l_FullEventPath =
          PathHelper::normalize(get_project().dataPath + '/' +
                                p_Event.path.string().c_str());

      for (auto i_Type : g_AssetTypes) {
        bool i_IsImport = false;

        for (auto i_RawSuffix : i_Type.rawSuffixes) {
          if (StringHelper::ends_with(l_FullEventPath,
                                      i_RawSuffix)) {
            i_IsImport = true;
            break;
          }
        }

        if (!i_IsImport && i_Type.autoInitialize &&
            p_Event.type != FileSystem::Watcher::EventType::Removed) {
          for (auto i_Suffix : i_Type.assetSuffixes) {
            if (StringHelper::ends_with(l_FullEventPath, i_Suffix)) {
              for (auto i_Dir : i_Type.initializeDirectories) {
                const bool i_IsValidInitDir =
                    FileSystem::is_file_in_directory(
                        std::filesystem::path(
                            l_FullEventPath.c_str()),
                        std::filesystem::path(i_Dir.path.c_str()),
                        i_Dir.recursive);

                if (i_IsValidInitDir) {
                  if (!find_asset_record_by_path(l_FullEventPath)) {
                    initialize_asset(i_Type, l_FullEventPath);
                  }
                }
              }
              break;
            }
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
                String i_Path = PathHelper::normalize(
                    i_ImportDir.path + "/" +
                    p_Event.path.string().c_str());
                import(i_Type, i_Path);
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

    static void tick_load_queue(const float p_Delta)
    {
      if (g_LoadEntries.empty()) {
        return;
      }

      // Only do one entry per frame right now

      const LoadEntry l_Entry = g_LoadEntries.top();
      g_LoadEntries.pop();

      // PERF: Move out into separate thread
      // TODO: Load yaml on different thread

      Serial::Node l_Node =
          Serial::load_yaml_file(l_Entry.path.c_str());
      // PERF: Maybe cache typeinfo in load entry
      const RTTI::TypeInfo &l_TypeInfo =
          Handle::get_type_info(l_Entry.handle.get_type());

      // TODO: Check if handle is still alive
      l_TypeInfo.post_load(l_Entry.handle, l_Node);
    }

    void AssetManager::tick(const float p_Delta)
    {
      tick_load_queue(p_Delta);

      tick_scheduled_events(p_Delta);

      poll_watcher(p_Delta);
    }

    Handle AssetManager::_find_by_path(const String p_Path)
    {
      if (AssetRecord *l_Record = find_asset_record_by_path(p_Path)) {
        return l_Record->handle;
      }

      return Handle::DEAD;
    }

    Handle AssetManager::_create(const u16 p_TypeId,
                                 const Name p_Name,
                                 const String p_Path)
    {
      String l_Path = PathHelper::normalize(p_Path);
      if (!StringHelper::ends_with(l_Path, "/")) {
        l_Path += "/";
      }

      TypeRegistrator l_AssetType;
      if (!find_asset_type(p_TypeId, l_AssetType)) {
        AM_LOG_ERROR
            << "Tried to create asset of unregistered asset type."
            << LOW_LOG_END;
        return Handle::DEAD;
      }

      if (!l_AssetType.creatable) {
        AM_LOG_ERROR << "Tried to create asset of asset type that "
                        "is not marked as creatable."
                     << LOW_LOG_END;
        return Handle::DEAD;
      }

      if (l_AssetType.assetSuffixes.empty()) {
        AM_LOG_ERROR << "Tried to create asset of asset type without "
                        "assigned suffix."
                     << LOW_LOG_END;
        return Handle::DEAD;
      }

      l_Path += p_Name.c_str();
      l_Path += l_AssetType.assetSuffixes[0];

      AM_LOG_DEBUG << "Creating asset at '" << l_Path << "'"
                   << LOW_LOG_END;

      Handle l_Asset = l_AssetType.creator(p_Name, l_Path);

      _save(l_Asset);

      return l_Asset;
    }

    void AssetManager::_save(Util::Handle p_Handle)
    {
      AssetManager::TypeRegistrator l_AssetType;
      if (!find_asset_type(p_Handle.get_type(), l_AssetType)) {
        AM_LOG_ERROR << "Tried to save handle but could not find "
                        "registered asset type for it."
                     << LOW_LOG_END;
        return;
      }

      if (!l_AssetType.supportsSaving) {
        AM_LOG_ERROR << "Tried to save handle but asset type does "
                        "not support saving."
                     << LOW_LOG_END;
        return;
      }

      if (l_AssetType.saver) {
        l_AssetType.saver(p_Handle);
        return;
      }

      if (!l_AssetType.storeSettings.pathPropertyName.is_valid()) {
        AM_LOG_ERROR << "Tried to save handle but asset type does "
                        "not have a valid path property set."
                     << LOW_LOG_END;
        return;
      }

      RTTI::TypeInfo &l_TypeInfo =
          Handle::get_type_info(p_Handle.get_type());
      RTTI::PropertyInfo &l_PathProperty =
          l_TypeInfo
              .properties[l_AssetType.storeSettings.pathPropertyName];
      const String l_Path =
          *(String *)l_PathProperty.get_return(p_Handle);

      Serial::Node l_SaveNode;
      l_TypeInfo.serialize(p_Handle, l_SaveNode);

      Serial::write_yaml_file(l_Path.c_str(), l_SaveNode);
    }

    void AssetManager::_load(Util::Handle p_Handle,
                             const LoadPriority p_Priority)
    {
      AssetManager::TypeRegistrator l_AssetType;
      if (!find_asset_type(p_Handle.get_type(), l_AssetType)) {
        AM_LOG_ERROR << "Tried to load handle but could not find "
                        "registered asset type for it."
                     << LOW_LOG_END;
        return;
      }

      if (!l_AssetType.supportsLoading) {
        AM_LOG_ERROR << "Tried to load handle but asset type does "
                        "not support loading."
                     << LOW_LOG_END;
        return;
      }

      if (!l_AssetType.isLoadable(p_Handle)) {
        AM_LOG_WARN
            << "Tried to load handle but handle is not loadable."
            << LOW_LOG_END;
        return;
      }

      if (l_AssetType.loader) {
        l_AssetType.loader(p_Handle);
        return;
      }

      if (!l_AssetType.storeSettings.pathPropertyName.is_valid()) {
        AM_LOG_ERROR << "Tried to load handle but asset type does "
                        "not have a valid path property set."
                     << LOW_LOG_END;
        return;
      }

      RTTI::TypeInfo &l_TypeInfo =
          Handle::get_type_info(p_Handle.get_type());
      RTTI::PropertyInfo &l_PathProperty =
          l_TypeInfo
              .properties[l_AssetType.storeSettings.pathPropertyName];
      const String l_Path =
          *(String *)l_PathProperty.get_return(p_Handle);

      // PERF: Remove string copy and rework emplace
      LoadEntry l_Entry;
      l_Entry.handle = p_Handle;
      l_Entry.path = l_Path;
      l_Entry.priority = p_Priority;

      g_LoadEntries.emplace(l_Entry);
    }

    void AssetManager::_register(Util::Handle p_Handle,
                                 Util::String p_Path)
    {
      upsert_asset_record(
          AssetRecord(p_Handle, normalize_asset_path(p_Path)));

      TypeRegistrator l_AssetType;
      if (find_asset_type(p_Handle.get_type(), l_AssetType)) {
        if (l_AssetType.postRegister) {
          l_AssetType.postRegister(p_Handle);
        }
      }
    }

    void AssetManager::_register(Util::Handle p_Handle,
                                 Util::String p_Path,
                                 Util::Name p_Name,
                                 const u64 p_UniqueId)
    {
      AssetRecord l_Record;
      l_Record.handle = p_Handle;
      l_Record.unique_id = p_UniqueId;
      l_Record.path = normalize_asset_path(p_Path);
      l_Record.name = p_Name;

      upsert_asset_record(l_Record);

      TypeRegistrator l_AssetType;
      if (find_asset_type(p_Handle.get_type(), l_AssetType)) {
        if (l_AssetType.postRegister) {
          l_AssetType.postRegister(p_Handle);
        }
      }
    }

    void AssetManager::_register_alias(Util::Handle p_Handle,
                                       Util::String p_Path)
    {
      AssetRecord l_Record;
      if (AssetRecord *i_Primary =
              find_primary_asset_record(p_Handle)) {
        l_Record = *i_Primary;
        l_Record.path = normalize_asset_path(p_Path);
      } else {
        l_Record =
            AssetRecord(p_Handle, normalize_asset_path(p_Path));
      }

      upsert_asset_record(l_Record);
    }

  } // namespace Util
} // namespace Low
