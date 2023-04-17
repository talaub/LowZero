#include <iostream>

#include "imgui.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"
#include "LowUtilYaml.h"
#include "LowUtilName.h"
#include "LowUtilResource.h"
#include "LowUtilContainers.h"

#include "LowRenderer.h"
#include "LowRendererRenderFlow.h"

#include "LowCore.h"
#include "LowCoreGameLoop.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreMeshRenderer.h"

#include <stdint.h>

#include <microprofile.h>
#include <vector>

#include "LowEditorMainWindow.h"

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

static void setup_scene()
{
  Low::Core::MeshAsset l_SphereMeshAsset =
      Low::Core::MeshAsset::make(N(Asset1));
  Low::Core::MeshResource l_SphereMeshResource = Low::Core::MeshResource::make(
      Low::Util::String(LOW_DATA_PATH) + "/assets/model/sphere.glb");
  l_SphereMeshAsset.set_lod0(l_SphereMeshResource);

  {
    Low::Util::Yaml::Node l_Node;
    l_SphereMeshResource.serialize(l_Node);
    Low::Util::String l_Path = LOW_DATA_PATH;
    l_Path += "/test.yaml";
    Low::Util::Yaml::write_file(l_Path.c_str(), l_Node);
  }

  Low::Core::MeshAsset l_CubeMeshAsset = Low::Core::MeshAsset::make(N(Asset2));
  Low::Core::MeshResource l_CubeMeshResource = Low::Core::MeshResource::make(
      Low::Util::String(LOW_DATA_PATH) + "/assets/model/cube.glb");
  l_CubeMeshAsset.set_lod0(l_CubeMeshResource);

  Low::Core::MeshAsset l_SuzanneMeshAsset =
      Low::Core::MeshAsset::make(N(Asset3));
  Low::Core::MeshResource l_SuzanneMeshResource = Low::Core::MeshResource::make(
      Low::Util::String(LOW_DATA_PATH) + "/assets/model/suzanne.glb");
  l_SuzanneMeshAsset.set_lod0(l_SuzanneMeshResource);

  {
    Low::Core::Entity l_Entity = Low::Core::Entity::make(N(Ground));
    Low::Core::Component::Transform l_Transform =
        Low::Core::Component::Transform::make(l_Entity);
    Low::Core::Component::MeshRenderer l_MeshRenderer =
        Low::Core::Component::MeshRenderer::make(l_Entity);
    l_MeshRenderer.set_mesh(l_CubeMeshAsset);

    l_Transform.position(Low::Math::Vector3(0.0f, 0.0f, -3.0f));
    l_Transform.rotation(Low::Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
    l_Transform.scale(Low::Math::Vector3(20.0f, 0.4f, 20.0f));
  }
  {
    Low::Core::Entity l_Entity = Low::Core::Entity::make(N(Sphere));
    Low::Core::Component::Transform l_Transform =
        Low::Core::Component::Transform::make(l_Entity);
    Low::Core::Component::MeshRenderer l_MeshRenderer =
        Low::Core::Component::MeshRenderer::make(l_Entity);
    l_MeshRenderer.set_mesh(l_SphereMeshAsset);

    l_Transform.position(Low::Math::Vector3(0.0f, 3.0f, -5.0f));
    l_Transform.rotation(Low::Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
    l_Transform.scale(Low::Math::Vector3(1.0f));
  }
  {
    Low::Core::Entity l_Entity = Low::Core::Entity::make(N(Cube));
    Low::Core::Component::Transform l_Transform =
        Low::Core::Component::Transform::make(l_Entity);
    Low::Core::Component::MeshRenderer l_MeshRenderer =
        Low::Core::Component::MeshRenderer::make(l_Entity);
    l_MeshRenderer.set_mesh(l_CubeMeshAsset);

    l_Transform.position(Low::Math::Vector3(-3.0f, 3.0f, -8.0f));
    l_Transform.rotation(Low::Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
    l_Transform.scale(Low::Math::Vector3(1.0f));
  }
  {
    Low::Core::Entity l_Entity = Low::Core::Entity::make(N(Monkey));
    Low::Core::Component::Transform l_Transform =
        Low::Core::Component::Transform::make(l_Entity);
    Low::Core::Component::MeshRenderer l_MeshRenderer =
        Low::Core::Component::MeshRenderer::make(l_Entity);
    l_MeshRenderer.set_mesh(l_SuzanneMeshAsset);

    l_Transform.position(Low::Math::Vector3(2.0f, 3.0f, -8.0f));
    l_Transform.rotation(
        Low::Math::Quaternion(Low::Math::VectorUtil::from_euler(
            Low::Math::Vector3(0.0f, 180.0f, 180.0f))));
    l_Transform.scale(Low::Math::Vector3(1.0f));
  }
};

int main()
{
  Low::Util::initialize();

  Low::Renderer::initialize();

  Low::Core::initialize();

  setup_scene();

  Low::Editor::initialize();

  Low::Core::GameLoop::register_tick_callback(&Low::Editor::tick);

  Low::Core::GameLoop::start();

  Low::Core::cleanup();

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}
