#include "LowCoreDebugGeometry.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"
#include "LowRendererEditorImage.h"
#include "LowRendererPrimitives.h"
#include "LowRendererTextureState.h"
#include "LowRendererResourceManager.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "LowUtilContainers.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace DebugGeometry {

      void initialize()
      {
      }

      float screen_space_multiplier(Renderer::RenderView p_RenderView,
                                    Math::Vector3 p_Position)
      {
        return 0.0015f * p_RenderView.get_camera_fov() *
               glm::length(p_RenderView.get_camera_position() -
                           p_Position);
      }

      static void render_mesh(Renderer::RenderView p_RenderView,
                              Renderer::Mesh p_Mesh,
                              const Math::Color p_Color,
                              const Math::Matrix4x4 p_Transformation,
                              const bool p_DepthTest,
                              const bool p_Wireframe,
                              u32 p_PickId = LOW_UINT32_MAX)
      {
        Renderer::DebugGeometryDraw l_Draw;
        l_Draw.depthTest = p_DepthTest;
        l_Draw.color = p_Color;
        l_Draw.wireframe = p_Wireframe;
        l_Draw.submesh = p_Mesh.get_gpu().get_submeshes()[0];
        l_Draw.pickId = p_PickId;
        l_Draw.transform = p_Transformation;

        p_RenderView.add_debug_geometry(l_Draw);
      }

      void render_box(Math::Box p_Box, Math::Color p_Color,
                      bool p_DepthTest, bool p_Wireframe)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Box.position) *
            glm::toMat4(p_Box.rotation) *
            glm::scale(glm::mat4(1.0f), p_Box.halfExtents);

        render_mesh(Renderer::get_editor_renderview(),
                    Renderer::get_primitives().unitCube, p_Color,
                    l_Transform, p_DepthTest, p_Wireframe);
      }

      void render_sphere(Math::Sphere p_Sphere, Math::Color p_Color,
                         bool p_DepthTest, bool p_Wireframe,
                         u32 p_PickId)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Sphere.position) *
            glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f),
                       Math::Vector3(p_Sphere.radius));

        render_mesh(Renderer::get_editor_renderview(),
                    Renderer::get_primitives().unitIcoSphere, p_Color,
                    l_Transform, p_DepthTest, p_Wireframe, p_PickId);
      }

      void render_cylinder(Math::Cylinder p_Cylinder,
                           Math::Color p_Color, bool p_DepthTest,
                           bool p_Wireframe)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Cylinder.position) *
            glm::toMat4(p_Cylinder.rotation) *
            glm::scale(glm::mat4(1.0f),
                       Math::Vector3(p_Cylinder.radius,
                                     p_Cylinder.height,
                                     p_Cylinder.radius));

        render_mesh(Renderer::get_editor_renderview(),
                    Renderer::get_primitives().unitCylinder, p_Color,
                    l_Transform, p_DepthTest, p_Wireframe);
      }

      void render_capsule(Math::Cylinder p_Capsule,
                          Math::Color p_Color, bool p_DepthTest,
                          bool p_Wireframe)
      {
        const float l_Radius = glm::max(p_Capsule.radius, 0.0f);
        const float l_CylinderHeight =
            glm::max(p_Capsule.height - (l_Radius * 2.0f), 0.0f);
        const Math::Vector3 l_Direction =
            Math::VectorUtil::rotate_by_quaternion(
                LOW_VECTOR3_UP, p_Capsule.rotation);
        const Math::Vector3 l_CapOffset =
            l_Direction * (l_CylinderHeight * 0.5f);

        if (l_CylinderHeight > LOW_MATH_EPSILON) {
          Math::Cylinder l_Body;
          l_Body.position = p_Capsule.position;
          l_Body.rotation = p_Capsule.rotation;
          l_Body.radius = l_Radius;
          l_Body.height = l_CylinderHeight;

          render_cylinder(l_Body, p_Color, p_DepthTest, p_Wireframe);
        }

        Math::Sphere l_Cap;
        l_Cap.radius = l_Radius;
        l_Cap.position = p_Capsule.position + l_CapOffset;
        render_sphere(l_Cap, p_Color, p_DepthTest, p_Wireframe);

        if (l_CylinderHeight <= LOW_MATH_EPSILON) {
          return;
        }

        l_Cap.position = p_Capsule.position - l_CapOffset;
        render_sphere(l_Cap, p_Color, p_DepthTest, p_Wireframe);
      }

      void render_cone(Math::Cone p_Cone, Math::Color p_Color,
                       bool p_DepthTest, bool p_Wireframe)
      {
        // TODO: FIX
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Cone.position) *
            glm::toMat4(p_Cone.rotation) *
            glm::scale(glm::mat4(1.0f),
                       Math::Vector3(p_Cone.radius, p_Cone.height,
                                     p_Cone.radius));

        render_mesh(Renderer::get_editor_renderview(),
                    Renderer::get_primitives().unitCube, p_Color,
                    l_Transform, p_DepthTest, p_Wireframe);
      }

      void render_arrow(Math::Vector3 p_Position,
                        Math::Quaternion p_Rotation, float p_Length,
                        float p_Thickness, float p_HeadRadius,
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

      void render_spherical_billboard(
          Math::Vector3 p_Position, float p_Size,
          Renderer::EditorImage p_EditorImage, Entity p_Entity)
      {
        if (!p_EditorImage.is_alive()) {
          return;
        }
        if (p_EditorImage.get_state() ==
            Renderer::TextureState::UNLOADED) {
          Renderer::ResourceManager::load_editor_image(p_EditorImage);
        }

        Renderer::RenderView l_RenderView =
            Renderer::get_editor_renderview();

        const Math::Quaternion l_Rotation =
            Math::VectorUtil::from_direction(
                l_RenderView.get_camera_direction(), LOW_VECTOR3_UP);

        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Position) *
            glm::toMat4(l_Rotation) *
            glm::scale(glm::mat4(1.0f), Math::Vector3(p_Size));

        const Math::Color l_Color(1.0f);

        {
          Renderer::DebugGeometryDraw l_Draw;
          l_Draw.depthTest = true;
          l_Draw.color = l_Color;
          l_Draw.wireframe = false;
          l_Draw.submesh = Renderer::get_primitives()
                               .unitQuad.get_gpu()
                               .get_submeshes()[0];
          l_Draw.transform = l_Transform;
          l_Draw.editorImage = p_EditorImage;

          l_Draw.pickId = LOW_UINT32_MAX;
          if (p_Entity.is_alive()) {
            l_Draw.pickId = p_Entity.get_index();
          }

          l_RenderView.add_debug_geometry(l_Draw);
        }
      }

      void render_line(Math::Vector3 p_Start, Math::Vector3 p_End,
                       Math::Color p_Color, bool p_DepthTest,
                       float p_Thickness)
      {
        render_line(Renderer::get_editor_renderview(), p_Start,
                    p_End, p_Color, p_DepthTest, p_Thickness);
      }

      void render_line(Renderer::RenderView p_RenderView,
                       Math::Vector3 p_Start, Math::Vector3 p_End,
                       Math::Color p_Color, bool p_DepthTest,
                       float p_Thickness)
      {
        if (Math::VectorUtil::distance(p_Start, p_End) <=
            LOW_MATH_EPSILON) {
          return;
        }

        Renderer::DebugLineDraw l_Draw;
        l_Draw.start = p_Start;
        l_Draw.end = p_End;
        l_Draw.color = p_Color;
        l_Draw.depth_test = p_DepthTest;
        l_Draw.thickness = p_Thickness;
        l_Draw.pick_id = LOW_UINT32_MAX;

        p_RenderView.get_debug_geometry_lines().push_back(l_Draw);
      }

      glm::mat4 generateModelMatrix(const glm::vec3 &v0,
                                    const glm::vec3 &v1,
                                    const glm::vec3 &v2)
      {
        glm::vec3 translation = (v0 + v1 + v2) / 3.0f;

        // Calculate the vectors along two edges of the triangle
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        // Calculate the normal vector of the triangle
        glm::vec3 normal = glm::cross(edge1, edge2);
        normal = glm::normalize(normal);

        // Calculate scaling factors based on the lengths of the edges
        float scaleX = glm::length(edge1);
        float scaleY =
            glm::length(v2 - v1) * -1.0f;    // or any other edge
        float scaleZ = glm::length(v2 - v0); // or any other edge

        // Create a rotation matrix to align the triangle with the x-z
        // plane
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix =
            glm::rotate(rotationMatrix, glm::atan(normal.z, normal.x),
                        glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix =
            glm::rotate(rotationMatrix, -glm::asin(normal.y),
                        glm::vec3(1.0f, 0.0f, 0.0f));

        // Create the scaling matrix
        glm::mat4 scalingMatrix = glm::scale(
            glm::mat4(1.0f), glm::vec3(scaleX, scaleY, scaleZ));

        // Create the final model matrix by combining translation,
        // rotation, and scaling
        glm::mat4 modelMatrix =
            glm::translate(glm::mat4(1.0f), translation) *
            rotationMatrix * scalingMatrix;

        return modelMatrix;
      }

      void render_triangle(Math::Vector3 p_Vertex0,
                           Math::Vector3 p_Vertex1,
                           Math::Vector3 p_Vertex2,
                           Math::Color p_Color, bool p_DepthTest,
                           bool p_Wireframe)
      {
        render_triangle(Renderer::get_editor_renderview(), p_Vertex0,
                        p_Vertex1, p_Vertex2, p_Color, p_DepthTest,
                        p_Wireframe);
      }

      void render_triangle(Renderer::RenderView p_RenderView,
                           Math::Vector3 p_Vertex0,
                           Math::Vector3 p_Vertex1,
                           Math::Vector3 p_Vertex2,
                           Math::Color p_Color, bool p_DepthTest,
                           bool p_Wireframe)
      {
        Renderer::DebugTriangleDraw l_Draw;
        l_Draw.p0 = p_Vertex0;
        l_Draw.p1 = p_Vertex1;
        l_Draw.p2 = p_Vertex2;
        l_Draw.color = p_Color;
        l_Draw.depth_test = p_DepthTest;
        l_Draw.fill = !p_Wireframe;
        l_Draw.thickness = 1.0f;
        l_Draw.pick_id = LOW_UINT32_MAX;

        p_RenderView.get_debug_geometry_triangles().push_back(l_Draw);
      }

      Renderer::Mesh get_plane()
      {
        return Renderer::get_primitives().unitQuad;
      }
    } // namespace DebugGeometry
  } // namespace Core
} // namespace Low
