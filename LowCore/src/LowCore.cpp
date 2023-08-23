#include "LowCore.h"

#include "LowCoreScene.h"
#include "LowCoreRegion.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreDirectionalLight.h"
#include "LowCorePointLight.h"
#include "LowCoreRigidbody.h"
#include "LowCoreMeshRenderer.h"
#include "LowCoreMeshAsset.h"
#include "LowCoreMeshResource.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreTexture2D.h"
#include "LowCoreMaterial.h"
#include "LowCorePrefab.h"
#include "LowCoreGameLoop.h"
#include "LowCorePhysicsSystem.h"
#include "LowCoreScriptingEngine.h"
#include "LowCoreMonoUtils.h"
#include "LowCorePrefabInstance.h"

#include "LowRenderer.h"

#include "LowUtilFileIO.h"
#include "LowUtilString.h"
#include "LowUtilLogger.h"
#include "LowUtilGlobals.h"

#include <fstream>
#include <stdlib.h>

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char *pName, int flags, unsigned debugFlags,
                     const char *file, int line)
{
  return malloc(size);
}

namespace Low {
  namespace Core {
    Util::EngineState g_CurrentEngineState;

    FileSystemWatchers g_FilesystemWatchers;
    Util::Map<uint16_t, Util::FileSystem::WatchHandle> g_WatchHandles;

    static void initialize_filesystem_watchers()
    {
      float l_UpdateTime = 8.0f;
      Util::String l_DataPath = LOW_DATA_PATH;

      g_FilesystemWatchers.meshAssetDirectory =
          Util::FileSystem::watch_directory(
              l_DataPath + "/assets/meshes",
              [](Util::FileSystem::FileWatcher &p_FileWatcher) {
                if (!Util::StringHelper::ends_with(p_FileWatcher.name,
                                                   ".mesh.yaml")) {
                  return (Util::Handle)0;
                }
                Util::List<Util::String> l_Parts;
                Util::StringHelper::split(p_FileWatcher.name, '.', l_Parts);
                uint64_t l_UniqueId = std::stoull(l_Parts[0].c_str());

                return Util::find_handle_by_unique_id(l_UniqueId);
              },
              l_UpdateTime);
      g_WatchHandles[MeshAsset::TYPE_ID] =
          g_FilesystemWatchers.meshAssetDirectory;

      g_FilesystemWatchers.materialAssetDirectory =
          Util::FileSystem::watch_directory(
              l_DataPath + "/assets/materials",
              [](Util::FileSystem::FileWatcher &p_FileWatcher) {
                if (!Util::StringHelper::ends_with(p_FileWatcher.name,
                                                   ".material.yaml")) {
                  return (Util::Handle)0;
                }
                Util::List<Util::String> l_Parts;
                Util::StringHelper::split(p_FileWatcher.name, '.', l_Parts);
                uint64_t l_UniqueId = std::stoull(l_Parts[0].c_str());

                return Util::find_handle_by_unique_id(l_UniqueId);
              },
              l_UpdateTime);
      g_WatchHandles[Material::TYPE_ID] =
          g_FilesystemWatchers.materialAssetDirectory;

      g_FilesystemWatchers.prefabAssetDirectory =
          Util::FileSystem::watch_directory(
              l_DataPath + "/assets/prefabs",
              [](Util::FileSystem::FileWatcher &p_FileWatcher) {
                if (!Util::StringHelper::ends_with(p_FileWatcher.name,
                                                   ".prefab.yaml")) {
                  return (Util::Handle)0;
                }
                Util::List<Util::String> l_Parts;
                Util::StringHelper::split(p_FileWatcher.name, '.', l_Parts);
                uint64_t l_UniqueId = std::stoull(l_Parts[0].c_str());

                return Util::find_handle_by_unique_id(l_UniqueId);
              },
              l_UpdateTime);
      g_WatchHandles[Prefab::TYPE_ID] =
          g_FilesystemWatchers.prefabAssetDirectory;

      g_FilesystemWatchers.meshResourceDirectory =
          Util::FileSystem::watch_directory(
              l_DataPath + "/resources/meshes",
              [](Util::FileSystem::FileWatcher &p_FileWatcher) {
                if (!Util::StringHelper::ends_with(p_FileWatcher.name,
                                                   ".glb")) {
                  return (Util::Handle)0;
                }
                return (Util::Handle)MeshResource::make(p_FileWatcher.name)
                    .get_id();
              },
              l_UpdateTime);
      g_WatchHandles[MeshResource::TYPE_ID] =
          g_FilesystemWatchers.meshResourceDirectory;
    }

    static void initialize_asset_types()
    {
      MeshAsset::initialize();
      Material::initialize();
      Prefab::initialize();
    }

    static void initialize_resource_types()
    {
      MeshResource::initialize();
      Texture2D::initialize();
    }

    static void initialize_component_types()
    {
      Component::Transform::initialize();
      Component::MeshRenderer::initialize();
      Component::DirectionalLight::initialize();
      Component::PointLight::initialize();
      Component::Rigidbody::initialize();
      Component::PrefabInstance::initialize();
    }

    static void initialize_base_types()
    {
      Scene::initialize();
      Region::initialize();
      Entity::initialize();
    }

    static void initialize_types()
    {
      initialize_resource_types();
      initialize_asset_types();
      initialize_base_types();
      initialize_component_types();
    }

    static void load_mesh_resources()
    {
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\resources\\meshes";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".glb";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          MeshResource::make(i_Path.substr(i_Path.find_last_of('\\') + 1));
        }
      }
    }

    static void load_texture2d_resources()
    {
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\resources\\img2d";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".ktx";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Texture2D::make(i_Path.substr(i_Path.find_last_of('\\') + 1));
        }
      }
    }

    static void load_mesh_assets_from_directory(Util::String p_Path)
    {
      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(p_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".mesh.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          MeshAsset::deserialize(i_Node, 0);
        } else if (Util::FileIO::is_directory(i_Path.c_str())) {
          load_mesh_assets_from_directory(i_Path);
        }
      }
    }

    static void load_prefabs_from_directory(Util::String p_Path)
    {
      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(p_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".prefab.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          Prefab::deserialize(i_Node, 0);
        } else if (Util::FileIO::is_directory(i_Path.c_str())) {
          load_prefabs_from_directory(i_Path);
        }
      }
    }

    static void load_mesh_assets()
    {
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\assets\\meshes";

      load_mesh_assets_from_directory(l_Path);
    }

    static void load_materials()
    {
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\assets\\materials";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".material.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          Material::deserialize(i_Node, 0);
        }
      }
    }

    static void load_prefabs()
    {
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\assets\\prefabs";

      load_prefabs_from_directory(l_Path);
    }

    static void load_regions()
    {
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\assets\\regions";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".region.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          Region::deserialize(i_Node, 0);
        }
      }
    }

    static void load_scenes()
    {
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\assets\\scenes";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".scene.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          Scene::deserialize(i_Node, 0);
        }
      }
    }

    static void load_resources()
    {
      load_mesh_resources();
      load_texture2d_resources();
    }

    static void load_assets()
    {
      load_mesh_assets();
      load_materials();
      load_prefabs();
    }
    MonoClass *GetClassInAssembly(MonoAssembly *assembly,
                                  const char *namespaceName,
                                  const char *className)
    {
      MonoImage *image = mono_assembly_get_image(assembly);
      MonoClass *klass = mono_class_from_name(image, namespaceName, className);

      if (klass == nullptr) {
        // Log error here
        return nullptr;
      }

      return klass;
    }

    void test_mono()
    {
      MonoClass *testingClass = GetClassInAssembly(
          Mono::get_context().game_assembly, "MtdScripts", "MonoTest");

      LOW_ASSERT(testingClass, "Class not found");

      MonoMethod *method =
          mono_class_get_method_from_name(testingClass, "Tick", 0);

      LOW_ASSERT(method, "Method not found");

      MonoObject *exception = nullptr;
      mono_runtime_invoke(method, nullptr, nullptr, &exception);

      if (exception) {
        mono_print_unhandled_exception(exception);
      }

      //_LOW_ASSERT(false);
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

      ScriptingEngine::initialize();

      DebugGeometry::initialize();
      GameLoop::initialize();

      load_resources();
      load_assets();

      load_regions();
      load_scenes();
    }

    static void cleanup_asset_types()
    {
      Prefab::cleanup();
      MeshAsset::cleanup();
      Material::cleanup();
    }

    static void cleanup_resource_types()
    {
      MeshResource::cleanup();
      Texture2D::cleanup();
    }

    static void cleanup_component_types()
    {
      Component::PrefabInstance::cleanup();
      Component::Rigidbody::cleanup();
      Component::PointLight::cleanup();
      Component::DirectionalLight::cleanup();
      Component::Transform::cleanup();
      Component::MeshRenderer::cleanup();
    }

    static void cleanup_base_types()
    {
      Entity::cleanup();
      Region::cleanup();
      Scene::cleanup();
    }

    static void cleanup_types()
    {
      cleanup_component_types();
      cleanup_base_types();
      cleanup_asset_types();
      cleanup_resource_types();
    }

    void cleanup()
    {
      GameLoop::cleanup();
      ScriptingEngine::cleanup();
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

      g_StoredData.regions.clear();
      g_StoredData.regions.resize(g_StoredData.scene.get_regions().size());

      for (auto it = g_StoredData.scene.get_regions().begin();
           it != g_StoredData.scene.get_regions().end(); ++it) {
      }

      g_StoredData.cameraPosition =
          Renderer::get_main_renderflow().get_camera_position();
      g_StoredData.cameraDirection =
          Renderer::get_main_renderflow().get_camera_direction();

      g_CurrentEngineState = Util::EngineState::PLAYING;
    }

    void exit_playmode()
    {
      g_CurrentEngineState = Util::EngineState::EDITING;

      Renderer::get_main_renderflow().set_camera_position(
          g_StoredData.cameraPosition);
      Renderer::get_main_renderflow().set_camera_direction(
          g_StoredData.cameraDirection);

      g_StoredData.scene.load();
    }

    FileSystemWatchers &get_filesystem_watchers()
    {
      return g_FilesystemWatchers;
    }

    Util::FileSystem::WatchHandle get_filesystem_watcher(uint16_t p_Type)
    {
      auto l_Pos = g_WatchHandles.find(p_Type);

      if (l_Pos != g_WatchHandles.end()) {
        return l_Pos->second;
      }

      return 0;
    }
  } // namespace Core
} // namespace Low
