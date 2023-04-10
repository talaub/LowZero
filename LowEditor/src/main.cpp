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
  Low::Core::MeshAsset l_MeshAsset = Low::Core::MeshAsset::make(N(Asset1));
  Low::Core::MeshResource l_MeshResource = Low::Core::MeshResource::make(
      Low::Util::String(LOW_DATA_PATH) + "/assets/model/sphere.glb");
  l_MeshAsset.set_lod0(l_MeshResource);

  Low::Core::Entity l_Entity = Low::Core::Entity::make(N(Entity1));
  Low::Core::Component::Transform l_Transform =
      Low::Core::Component::Transform::make(l_Entity);
  Low::Core::Component::MeshRenderer l_MeshRenderer =
      Low::Core::Component::MeshRenderer::make(l_Entity);
  l_MeshRenderer.set_mesh(l_MeshAsset);

  l_Transform.position(Low::Math::Vector3(1.0f, 1.0f, -4.0f));
  l_Transform.rotation(Low::Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
  l_Transform.scale(Low::Math::Vector3(0.6f));
}

int main()
{
  float delta = 0.004f;

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
