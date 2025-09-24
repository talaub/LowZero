#include "LowCore.h"

#include "LowCoreScene.h"
#include "LowCoreRegion.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreDirectionalLight.h"
#include "LowCorePointLight.h"
#include "LowCoreRigidbody.h"
#include "LowCoreMeshRenderer.h"
#include "LowCoreDebugGeometry.h"
#include "LowCorePrefab.h"
#include "LowCoreGameLoop.h"
#include "LowCorePhysicsSystem.h"
#include "LowCorePrefabInstance.h"
#include "LowCoreCflatScripting.h"
#include "LowCoreNavmeshAgent.h"
#include "LowCoreGameMode.h"
#include "LowCoreCamera.h"

#include "LowCoreUiView.h"
#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreUiImage.h"
#include "LowCoreUiText.h"

#include "LowRenderer.h"

#include "LowUtil.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"
#include "LowUtilLogger.h"
#include "LowUtilGlobals.h"

#include <fstream>
#include <stdlib.h>

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

namespace Low {
  namespace Core {
    Util::EngineState g_CurrentEngineState;
    Util::Stack<GameMode> g_GameModes;

    FileSystemWatchers g_FilesystemWatchers;
    Util::Map<uint16_t, Util::FileSystem::WatchHandle> g_WatchHandles;

    static void initialize_filesystem_watchers()
    {

      float l_UpdateTime = 8.0f;
      Util::String l_DataPath = Util::get_project().dataPath;
      LOW_LOG_DEBUG << "Initializing filesystem watchers "
                    << l_DataPath << LOW_LOG_END;
      l_UpdateTime = 1.0f;

      LOW_LOG_DEBUG << "Watching scripts" << LOW_LOG_END;
      g_FilesystemWatchers.scriptDirectory =
          Util::FileSystem::watch_directory(
              l_DataPath + "/scripts",
              [](Util::FileSystem::FileWatcher &p_FileWatcher) {
                return (Util::Handle)0;
              },
              0.5f);

      LOW_LOG_DEBUG << "Watching prefab assets" << LOW_LOG_END;
      g_FilesystemWatchers.prefabAssetDirectory =
          Util::FileSystem::watch_directory(
              l_DataPath + "/assets/prefabs",
              [](Util::FileSystem::FileWatcher &p_FileWatcher) {
                if (!Util::StringHelper::ends_with(p_FileWatcher.name,
                                                   ".prefab.yaml")) {
                  return (Util::Handle)0;
                }
                Util::List<Util::String> l_Parts;
                Util::StringHelper::split(p_FileWatcher.name, '.',
                                          l_Parts);
                uint64_t l_UniqueId = std::stoull(l_Parts[0].c_str());

                return Util::find_handle_by_unique_id(l_UniqueId);
              },
              l_UpdateTime);
      g_WatchHandles[Prefab::TYPE_ID] =
          g_FilesystemWatchers.prefabAssetDirectory;
    }

    static void initialize_asset_types()
    {
      Prefab::initialize();
    }

    static void initialize_component_types()
    {
      Component::Transform::initialize();
      Component::MeshRenderer::initialize();
      Component::DirectionalLight::initialize();
      Component::PointLight::initialize();
      Component::Rigidbody::initialize();
      Component::PrefabInstance::initialize();
      Component::NavmeshAgent::initialize();
      Component::Camera::initialize();
    }

    static void initialize_base_types()
    {
      Scene::initialize();
      Region::initialize();
      Entity::initialize();
      GameMode::initialize();
    }

    static void initialize_ui_component_types()
    {
      UI::Component::Display::initialize();
      UI::Component::Image::initialize();
      UI::Component::Text::initialize();
    }

    static void initialize_ui_types()
    {
      UI::Element::initialize();
      UI::View::initialize();

      initialize_ui_component_types();
    }

    static void initialize_types()
    {
      initialize_asset_types();
      initialize_base_types();
      initialize_component_types();
      initialize_ui_types();
    }

    static void load_prefabs_from_directory(Util::String p_Path)
    {
      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(p_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".prefab.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node =
              Util::Yaml::load_file(i_Path.c_str());
          Prefab::deserialize(i_Node, 0);
        } else if (Util::FileIO::is_directory(i_Path.c_str())) {
          load_prefabs_from_directory(i_Path);
        }
      }
    }

    static void load_prefabs()
    {
      Util::String l_Path =
          Util::get_project().dataPath + "\\assets\\prefabs";

      load_prefabs_from_directory(l_Path);
    }

    static void load_regions()
    {
      Util::String l_Path =
          Util::get_project().dataPath + "\\assets\\regions";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".region.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node =
              Util::Yaml::load_file(i_Path.c_str());
          Region::deserialize(i_Node, 0);
        }
      }
    }

    static void load_scenes()
    {
      Util::String l_Path =
          Util::get_project().dataPath + "\\assets\\scenes";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".scene.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node =
              Util::Yaml::load_file(i_Path.c_str());
          Scene::deserialize(i_Node, 0);
        }
      }
    }

    static void load_assets()
    {
      load_prefabs();
    }

    static void load_gamemodes()
    {
      Util::String l_Path =
          Util::get_project().dataPath + "\\assets\\gamemodes";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".gamemode.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node =
              Util::Yaml::load_file(i_Path.c_str());
          GameMode::deserialize(i_Node, 0);
        }
      }
    }

    static void initialize_globals()
    {
      Util::Globals::set(N(LOW_SCREEN_OFFSET),
                         Util::Variant(Math::UVector2(0, 0)));
    }

    void initialize()
    {
      g_CurrentEngineState = Util::EngineState::EDITING;

      initialize_globals();

      initialize_filesystem_watchers();

      initialize_types();

      DebugGeometry::initialize();
      GameLoop::initialize();
      Scripting::initialize();

      load_assets();

      load_regions();
      load_scenes();

      load_gamemodes();
    }

    static void cleanup_asset_types()
    {
      Prefab::cleanup();
    }

    static void cleanup_component_types()
    {
      Component::Camera::cleanup();
      Component::NavmeshAgent::cleanup();
      Component::PrefabInstance::cleanup();
      Component::Rigidbody::cleanup();
      Component::PointLight::cleanup();
      Component::DirectionalLight::cleanup();
      Component::Transform::cleanup();
      Component::MeshRenderer::cleanup();
    }

    static void cleanup_base_types()
    {
      GameMode::cleanup();
      Entity::cleanup();
      Region::cleanup();
      Scene::cleanup();
    }

    static void cleanup_ui_component_types()
    {
      UI::Component::Text::cleanup();
      UI::Component::Image::cleanup();
      UI::Component::Display::cleanup();
    }

    static void cleanup_ui_types()
    {
      cleanup_ui_component_types();

      UI::View::cleanup();
      UI::Element::cleanup();
    }

    static void cleanup_types()
    {
      cleanup_component_types();
      cleanup_base_types();
      cleanup_asset_types();
      cleanup_ui_types();
    }

    void cleanup()
    {
      Scripting::cleanup();
      GameLoop::cleanup();
      cleanup_types();
    }

    Util::EngineState get_engine_state()
    {
      return g_CurrentEngineState;
    }

    struct PlaymodeStoredData
    {
      Scene scene;
      Util::List<Util::Yaml::Node> regions;
      Math::Vector3 cameraPosition;
      Math::Vector3 cameraDirection;
    };

    PlaymodeStoredData g_StoredData;

    void begin_playmode()
    {
      g_StoredData.scene = Scene::get_loaded_scene();

      g_GameModes.push(GameMode::find_by_name(N(DefaultGamemode)));

      g_StoredData.regions.clear();
      g_StoredData.regions.resize(
          g_StoredData.scene.get_regions().size());

      for (auto it = g_StoredData.scene.get_regions().begin();
           it != g_StoredData.scene.get_regions().end(); ++it) {
      }

      g_StoredData.cameraPosition =
          Renderer::get_editor_renderview().get_camera_position();
      g_StoredData.cameraDirection =
          Renderer::get_editor_renderview().get_camera_direction();

      g_CurrentEngineState = Util::EngineState::PLAYING;
    }

    void exit_playmode()
    {
      g_CurrentEngineState = Util::EngineState::EDITING;

      while (!g_GameModes.empty()) {
        g_GameModes.pop();
      }

      Renderer::get_editor_renderview().set_camera_position(
          g_StoredData.cameraPosition);
      Renderer::get_editor_renderview().set_camera_direction(
          g_StoredData.cameraDirection);

      g_StoredData.scene.load();
    }

    FileSystemWatchers &get_filesystem_watchers()
    {
      return g_FilesystemWatchers;
    }

    Util::FileSystem::WatchHandle
    get_filesystem_watcher(uint16_t p_Type)
    {
      auto l_Pos = g_WatchHandles.find(p_Type);

      if (l_Pos != g_WatchHandles.end()) {
        return l_Pos->second;
      }

      return 0;
    }

    GameMode get_current_gamemode()
    {
      LOW_ASSERT(!g_GameModes.empty(),
                 "Cannot fetch current gamemode. No gamemodes have "
                 "been pushed");
      if (g_GameModes.empty()) {
        return 0;
      }

      return g_GameModes.top();
    }
  } // namespace Core
} // namespace Low
