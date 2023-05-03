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
#include "LowRendererExposedObjects.h"
#include "LowRendererRenderFlow.h"

#include "LowCore.h"
#include "LowCoreGameLoop.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreMeshRenderer.h"
#include "LowCoreDirectionalLight.h"

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
  Low::Core::Material l_Material =
      Low::Core::Material::find_by_name(N(RustMetal));
  {
    Low::Core::Texture2D l_AlbedoTexture =
        Low::Core::Texture2D::find_by_name(N(out_rust.ktx));
    Low::Core::Texture2D l_NormalTexture =
        Low::Core::Texture2D::find_by_name(N(rust_normal.ktx));
    Low::Core::Texture2D l_Roughness =
        Low::Core::Texture2D::find_by_name(N(rust_roughness.ktx));
    Low::Core::Texture2D l_Metalness =
        Low::Core::Texture2D::find_by_name(N(rust_metalness.ktx));

    l_Material.set_property(N(albedo_map), Low::Util::Variant::from_handle(
                                               l_AlbedoTexture.get_id()));
    l_Material.set_property(N(normal_map), Low::Util::Variant::from_handle(
                                               l_NormalTexture.get_id()));
    l_Material.set_property(N(roughness_map), Low::Util::Variant::from_handle(
                                                  l_Roughness.get_id()));
    l_Material.set_property(N(metalness_map), Low::Util::Variant::from_handle(
                                                  l_Metalness.get_id()));
  }

  Low::Core::MeshAsset l_SphereMeshAsset =
      Low::Core::MeshAsset::find_by_name(N(Sphere));
  Low::Core::MeshAsset l_CubeMeshAsset =
      Low::Core::MeshAsset::find_by_name(N(Cube));
  Low::Core::MeshAsset l_SuzanneMeshAsset =
      Low::Core::MeshAsset::find_by_name(N(Suzanne));

  {
    Low::Core::Entity l_Entity = Low::Core::Entity::make(N(Sun));
    Low::Core::Component::Transform l_Transform =
        Low::Core::Component::Transform::make(l_Entity);
    Low::Core::Component::DirectionalLight l_DirectionalLight =
        Low::Core::Component::DirectionalLight::make(l_Entity);
    l_DirectionalLight.set_color(Low::Math::ColorRGB(1.0f, 1.0f, 1.0f));

    l_Transform.position(Low::Math::Vector3(0.0f, 0.0f, -3.0f));
    l_Transform.rotation(Low::Math::VectorUtil::from_direction(
        Low::Math::Vector3(2.0f, -2.5f, -0.5f), LOW_VECTOR3_FRONT * -1.0f));
    l_Transform.scale(Low::Math::Vector3(20.0f, 0.4f, 20.0f));
  }
  {
    Low::Core::Entity l_Entity = Low::Core::Entity::make(N(Ground));
    Low::Core::Component::Transform l_Transform =
        Low::Core::Component::Transform::make(l_Entity);
    Low::Core::Component::MeshRenderer l_MeshRenderer =
        Low::Core::Component::MeshRenderer::make(l_Entity);
    l_MeshRenderer.set_mesh(l_CubeMeshAsset);
    l_MeshRenderer.set_material(l_Material);

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
    l_MeshRenderer.set_material(l_Material);

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
    l_MeshRenderer.set_material(l_Material);

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
    l_MeshRenderer.set_material(l_Material);

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
