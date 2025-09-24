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

      void render_mesh(Renderer::RenderView p_RenderView,
                       Renderer::Mesh p_Mesh,
                       const Math::Color p_Color,
                       const Math::Matrix4x4 p_Transformation,
                       const bool p_DepthTest, const bool p_Wireframe)
      {
        Renderer::DebugGeometryDraw l_Draw;
        l_Draw.depthTest = p_DepthTest;
        l_Draw.color = p_Color;
        l_Draw.wireframe = p_Wireframe;
        l_Draw.submesh = p_Mesh.get_gpu().get_submeshes()[0];
        l_Draw.transform = p_Transformation;

        p_RenderView.add_debug_geometry(l_Draw);
      }

      void render_box(Math::Box p_Box, Math::Color p_Color,
                      bool p_DepthTest, bool p_Wireframe)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Box.position) *
            glm::toMat4(p_Box.rotation) *
            glm::scale(glm::mat4(1.0f), p_Box.halfExtents * 2.0f);

        render_mesh(Renderer::get_editor_renderview(),
                    Renderer::get_primitives().unitCube, p_Color,
                    l_Transform, p_DepthTest, p_Wireframe);
      }

      void render_sphere(Math::Sphere p_Sphere, Math::Color p_Color,
                         bool p_DepthTest, bool p_Wireframe)
      {
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Sphere.position) *
            glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f),
                       Math::Vector3(p_Sphere.radius));

        render_mesh(Renderer::get_editor_renderview(),
                    Renderer::get_primitives().unitIcoSphere, p_Color,
                    l_Transform, p_DepthTest, p_Wireframe);
      }

      void render_cylinder(Math::Cylinder p_Cylinder,
                           Math::Color p_Color, bool p_DepthTest,
                           bool p_Wireframe)
      {
        // TODO: FIX
        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Cylinder.position) *
            glm::toMat4(p_Cylinder.rotation) *
            glm::scale(glm::mat4(1.0f),
                       Math::Vector3(p_Cylinder.radius,
                                     p_Cylinder.height / 2.0f,
                                     p_Cylinder.radius));

        render_mesh(Renderer::get_editor_renderview(),
                    Renderer::get_primitives().unitCube, p_Color,
                    l_Transform, p_DepthTest, p_Wireframe);
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

      void
      render_spherical_billboard(Math::Vector3 p_Position,
                                 float p_Size,
                                 Renderer::EditorImage p_EditorImage)
      {
        Util::HandleLock l_ImageLock(p_EditorImage, false);

        if (!l_ImageLock.owns_lock()) {
          return;
        }

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

          l_RenderView.add_debug_geometry(l_Draw);
        }
      }

      void render_line(Math::Vector3 p_Start, Math::Vector3 p_End,
                       Math::Color p_Color, bool p_DepthTest,
                       bool p_Wireframe, float p_Thickness)
      {
        {
          float x = p_Start.x;
          p_Start.x = p_Start.z * -1.0f;
          p_Start.z = x;
        }
        {
          float x = p_End.x;
          p_End.x = p_End.z * -1.0f;
          p_End.z = x;
        }

        Math::Cylinder l_Cylinder;

        Math::Vector3 l_Diff = p_End - p_Start;
        l_Diff = Math::VectorUtil::normalize(l_Diff);

        float l_Distance = Math::VectorUtil::distance(p_Start, p_End);

        Low::Math::Vector3 l_Up(0.0f, 1.0f, 0.0f);
        l_Up = Math::VectorUtil::normalize(l_Up);

        Math::Vector3 l_RotatedDiff = l_Diff;
        {
          float x = l_RotatedDiff.x;
          l_RotatedDiff.x = l_RotatedDiff.z * -1.0f;
          l_RotatedDiff.z = x * -2.0f;
        }

        l_Cylinder.position =
            p_Start + (l_RotatedDiff * (l_Distance * 0.5f));
        l_Cylinder.height = l_Distance * 0.5f;
        l_Cylinder.radius = p_Thickness;
        l_Diff.y *= -1.0f;
        l_Cylinder.rotation = Math::VectorUtil::between(l_Up, l_Diff);
        // render_cylinder(l_Cylinder, p_Color, p_DepthTest,
        // p_Wireframe);

        Math::Cylinder p_Cylinder = l_Cylinder;

        Math::Matrix4x4 l_Transform =
            glm::translate(glm::mat4(1.0f), p_Cylinder.position) *
            glm::toMat4(p_Cylinder.rotation) *
            glm::scale(glm::mat4(1.0f),
                       Math::Vector3(p_Cylinder.radius,
                                     p_Cylinder.height,
                                     p_Cylinder.radius));

        render_mesh(Renderer::get_editor_renderview(),
                    Renderer::get_primitives().unitCube, p_Color,
                    l_Transform, p_DepthTest, p_Wireframe);
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
        Math::Matrix4x4 l_Transformation =
            generateModelMatrix(p_Vertex0, p_Vertex2, p_Vertex1);

        /*
              render_mesh(g_Meshes.triangle, p_Color,
           l_Transformation, p_DepthTest, p_Wireframe);
        */

        return;
        // TODO: fix

        /*
        Renderer::render_debug_triangle(p_Color, p_Vertex0, p_Vertex1,
                                        p_Vertex2);
                                        */
      }

      Renderer::Mesh get_plane()
      {
        return Renderer::get_primitives().unitQuad;
      }
    } // namespace DebugGeometry
  } // namespace Core
} // namespace Low
