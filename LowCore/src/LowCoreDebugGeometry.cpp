#include "LowCoreDebugGeometry.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilContainers.h"

#include "LowRendererExposedObjects.h"
#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace DebugGeometry {
      struct
      {
        MeshResource cube;
        MeshResource cone;
        MeshResource cylinder;
        MeshResource plane;
        MeshResource sphere;
      } g_Meshes;

      struct
      {
        Renderer::Material onesided_fill_depthtested;
        Renderer::Material onesided_fill_nodepthtest;
        Renderer::Material onesided_line_depthtested;
        Renderer::Material onesided_line_nodepthtest;
      } g_Materials;

      struct
      {
        Renderer::MaterialType basic;
        Renderer::MaterialType wireframe;
        Renderer::MaterialType nodepth_wireframe;
        Renderer::MaterialType nodepth;
        Renderer::MaterialType billboard;
      } g_MaterialTypes;

      static void load_meshes()
      {
        Util::String l_BasePath = "../../_internal/assets/meshes/";

        g_Meshes.cube = MeshResource::make(l_BasePath + "cube.glb");
        g_Meshes.cube.load();
        g_Meshes.cone = MeshResource::make(l_BasePath + "cone.glb");
        g_Meshes.cone.load();
        g_Meshes.cylinder = MeshResource::make(l_BasePath + "cylinder.glb");
        g_Meshes.cylinder.load();
        g_Meshes.plane = MeshResource::make(l_BasePath + "plane.glb");
        g_Meshes.plane.load();
        g_Meshes.sphere = MeshResource::make(l_BasePath + "sphere.glb");
        g_Meshes.sphere.load();
      }

      static void initialize_materials()
      {
        for (auto it = Renderer::MaterialType::ms_LivingInstances.begin();
             it != Renderer::MaterialType::ms_LivingInstances.end(); ++it) {
          if (it->get_name() == N(debuggeometry)) {
            g_MaterialTypes.basic = *it;
          }
          if (it->get_name() == N(debuggeometry_nodepth)) {
            g_MaterialTypes.nodepth = *it;
          }
          if (it->get_name() == N(debuggeometry_wireframe)) {
            g_MaterialTypes.wireframe = *it;
          }
          if (it->get_name() == N(debuggeometry_wireframe_nodepth)) {
            g_MaterialTypes.nodepth_wireframe = *it;
          }
          if (it->get_name() == N(debuggeometry_billboard)) {
            g_MaterialTypes.billboard = *it;
          }
        }

        LOW_ASSERT(g_MaterialTypes.basic.is_alive(),
                   "Could not find debug geometry base material type");
        LOW_ASSERT(g_MaterialTypes.nodepth.is_alive(),
                   "Could not find debug geometry nodepth material type");
        LOW_ASSERT(g_MaterialTypes.wireframe.is_alive(),
                   "Could not find debug geometry wireframe material type");
        LOW_ASSERT(
            g_MaterialTypes.nodepth_wireframe.is_alive(),
            "Could not find debug geometry wireframe nodepth material type");
        LOW_ASSERT(g_MaterialTypes.billboard.is_alive(),
                   "Could not find debug geometry billboard material type");

        g_Materials.onesided_fill_depthtested = Renderer::create_material(
            N(DebugGeometryMaterial), g_MaterialTypes.basic);
        g_Materials.onesided_fill_depthtested.set_property(
            N(fallback_color),
            Util::Variant(Math::Vector4(0.2f, 0.8f, 1.0f, 1.0f)));

        g_Materials.onesided_fill_nodepthtest = Renderer::create_material(
            N(DebugGeometryMaterialNoDepthTest), g_MaterialTypes.nodepth);
        g_Materials.onesided_fill_nodepthtest.set_property(
            N(fallback_color),
            Util::Variant(Math::Vector4(0.2f, 0.8f, 1.0f, 1.0f)));

        g_Materials.onesided_line_depthtested = Renderer::create_material(
            N(DebugGeometryMaterialWireframe), g_MaterialTypes.wireframe);
        g_Materials.onesided_line_depthtested.set_property(
            N(fallback_color),
            Util::Variant(Math::Vector4(0.2f, 0.8f, 1.0f, 1.0f)));

        g_Materials.onesided_line_nodepthtest = Renderer::create_material(
            N(DebugGeometryMaterialWireframeNoDepthTest),
            g_MaterialTypes.nodepth_wireframe);
        g_Materials.onesided_line_nodepthtest.set_property(
            N(fallback_color),
            Util::Variant(Math::Vector4(0.2f, 0.8f, 1.0f, 1.0f)));
      }

      void initialize()
      {
        load_meshes();
        initialize_materials();
      }

      float screen_space_multiplier(Renderer::RenderFlow p_RenderFlow,
                                    Math::Vector3 p_Position)
      {
        return 0.0015f * p_RenderFlow.get_camera_fov() *
               glm::length(p_RenderFlow.get_camera_position() - p_Position);
      }

      void render_mesh(Renderer::Mesh p_Mesh, Renderer::Material p_Material,
                       Math::Color p_Color, Math::Matrix4x4 p_Transformation)
      {
        Renderer::RenderObject l_RenderObject;
        l_RenderObject.color = p_Color;
        l_RenderObject.material = p_Material;
        l_RenderObject.mesh = p_Mesh;
        l_RenderObject.entity_id = 0;
        l_RenderObject.transform = p_Transformation;

        Renderer::get_main_renderflow().register_renderobject(l_RenderObject);
      }

      void render_mesh(MeshResource p_MeshResource, Math::Color p_Color,
                       Math::Matrix4x4 &p_Transformation, bool p_DepthTest,
                       bool p_Wireframe)
      {
        Renderer::Material l_Material = g_Materials.onesided_fill_depthtested;

        if (p_Wireframe) {
          l_Material = g_Materials.onesided_line_depthtested;
        }

        if (!p_DepthTest) {
          l_Material = g_Materials.onesided_fill_nodepthtest;

          if (p_Wireframe) {
            l_Material = g_Materials.onesided_line_nodepthtest;
          }
        }

        for (uint32_t i = 0u; i < p_MeshResource.get_submeshes().size(); ++i) {
          render_mesh(p_MeshResource.get_submeshes()[i].mesh, l_Material,
                      p_Color,
                      p_Transformation *
                          p_MeshResource.get_submeshes()[i].transformation);
        }
      }

      void render_box(Math::Box p_Box, Math::Color p_Color, bool p_DepthTest,
                      bool p_Wireframe)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Box.position) *
            glm::toMat4(p_Box.rotation) *
            glm::scale(glm::mat4(1.0f), p_Box.halfExtents * 2.0f);

        render_mesh(g_Meshes.cube, p_Color, l_Transform, p_DepthTest,
                    p_Wireframe);
      }

      void render_sphere(Math::Sphere p_Sphere, Math::Color p_Color,
                         bool p_DepthTest, bool p_Wireframe)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Sphere.position) *
            glm::toMat4(Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f)) *
            glm::scale(glm::mat4(1.0f), Math::Vector3(p_Sphere.radius));

        render_mesh(g_Meshes.sphere, p_Color, l_Transform, p_DepthTest,
                    p_Wireframe);
      }

      void render_cylinder(Math::Cylinder p_Cylinder, Math::Color p_Color,
                           bool p_DepthTest, bool p_Wireframe)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Cylinder.position) *
            glm::toMat4(p_Cylinder.rotation) *
            glm::scale(glm::mat4(1.0f),
                       Math::Vector3(p_Cylinder.radius, p_Cylinder.height,
                                     p_Cylinder.radius));

        render_mesh(g_Meshes.cylinder, p_Color, l_Transform, p_DepthTest,
                    p_Wireframe);
      }

      void render_cone(Math::Cone p_Cone, Math::Color p_Color, bool p_DepthTest,
                       bool p_Wireframe)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Cone.position) *
            glm::toMat4(p_Cone.rotation) *
            glm::scale(
                glm::mat4(1.0f),
                Math::Vector3(p_Cone.radius, p_Cone.height, p_Cone.radius));

        render_mesh(g_Meshes.cone, p_Color, l_Transform, p_DepthTest,
                    p_Wireframe);
      }

      void render_arrow(Math::Vector3 p_Position, Math::Quaternion p_Rotation,
                        float p_Length, float p_Thickness, float p_HeadRadius,
                        float p_HeadLength, Math::Color p_Color,
                        bool p_DepthTest, bool p_Wireframe)
      {
        Math::Cylinder l_Body;
        l_Body.position = p_Position;
        l_Body.rotation = p_Rotation;
        l_Body.height = p_Length;
        l_Body.radius = p_Thickness;

        render_cylinder(l_Body, p_Color, p_DepthTest, p_Wireframe);

        Math::Vector3 l_Direction =
            Math::VectorUtil::normalize(LOW_VECTOR3_UP * p_Rotation);

        Math::Cone l_Head;
        l_Head.position =
            p_Position + (l_Direction * (p_Length + p_HeadLength));
        l_Head.rotation = p_Rotation;
        l_Head.height = p_HeadLength;
        l_Head.radius = p_HeadRadius;

        render_cone(l_Head, p_Color, p_DepthTest, p_Wireframe);
      }

      void render_spherical_billboard(Math::Vector3 p_Position, float p_Size,
                                      Renderer::Material p_Material)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Position) *
            glm::toMat4(Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f)) *
            glm::scale(glm::mat4(1.0f), Math::Vector3(p_Size));

        Math::Color l_Color(1.0f);

        for (uint32_t i = 0u; i < g_Meshes.plane.get_submeshes().size(); ++i) {
          render_mesh(g_Meshes.plane.get_submeshes()[i].mesh, p_Material,
                      l_Color, l_Transform);
        }
      }

      static Renderer::Material
      create_spherical_billboard_material(Renderer::Texture2D p_Texture)
      {
        Renderer::Material l_Material = Renderer::create_material(
            N(DebugGeometryMaterialSphericalBillboardBase),
            g_MaterialTypes.billboard);
        l_Material.set_property(N(billboard_image),
                                Util::Variant::from_handle(p_Texture));

        return l_Material;
      }

      Renderer::Material
      create_spherical_billboard_material(Util::String p_Path)
      {
        Util::Resource::Image2D l_Image;
        Util::Resource::load_image2d(p_Path, l_Image);

        Renderer::Texture2D l_Texture =
            Renderer::upload_texture(N(BillboadTexture), l_Image);

        return create_spherical_billboard_material(l_Texture);
      }
    } // namespace DebugGeometry
  }   // namespace Core
} // namespace Low
