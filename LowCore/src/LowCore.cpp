#include "LowCore.h"

#include "LowCoreScene.h"
#include "LowCoreRegion.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreDirectionalLight.h"
#include "LowCoreMeshRenderer.h"
#include "LowCoreMeshAsset.h"
#include "LowCoreMeshResource.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreTexture2D.h"
#include "LowCoreMaterial.h"

#include "LowUtilFileIO.h"
#include "LowUtilString.h"
#include "LowUtilLogger.h"

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
    static void initialize_asset_types()
    {
      MeshAsset::initialize();
      Material::initialize();
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

    static void load_mesh_assets()
    {
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\assets\\meshes";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".mesh.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          MeshAsset::deserialize(i_Node, 0);
        }
      }
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

    static void load_resources()
    {
      load_mesh_resources();
      load_texture2d_resources();
    }

    static void load_assets()
    {
      load_mesh_assets();
      load_materials();
    }

    void initialize()
    {
      initialize_types();

      DebugGeometry::initialize();

      load_resources();
      load_assets();

      load_regions();
    }

    static void cleanup_asset_types()
    {
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
      cleanup_types();
    }
  } // namespace Core
} // namespace Low
