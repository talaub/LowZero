#include "LowCore.h"

#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreMeshRenderer.h"
#include "LowCoreMeshAsset.h"
#include "LowCoreMeshResource.h"
#include "LowCoreDebugGeometry.h"

#include "LowUtilFileIO.h"
#include "LowUtilString.h"

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
    }

    static void initialize_resource_types()
    {
      MeshResource::initialize();
    }

    static void initialize_component_types()
    {
      Component::Transform::initialize();
      Component::MeshRenderer::initialize();
    }

    static void initialize_base_types()
    {
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
      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\assets\\meshes";

      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
      Util::String l_Ending = ".glb";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          MeshResource::make(i_Path);
        }
      }
    }

    static void load_resources()
    {
      load_mesh_resources();
    }

    void initialize()
    {
      initialize_types();

      DebugGeometry::initialize();

      load_mesh_resources();
    }

    static void cleanup_asset_types()
    {
      MeshAsset::cleanup();
    }

    static void cleanup_resource_types()
    {
      MeshResource::cleanup();
    }

    static void cleanup_component_types()
    {
      Component::Transform::cleanup();
      Component::MeshRenderer::cleanup();
    }

    static void cleanup_base_types()
    {
      Entity::cleanup();
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
