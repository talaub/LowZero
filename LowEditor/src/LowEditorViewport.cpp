#include "LowEditorViewport.h"
#include "LowMath.h"
#include "LowRenderer.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderScene.h"
#include "LowRendererRenderStep.h"
#include "LowRendererResourceManager.h"
#include "LowRendererSkeletonState.h"
#include "LowRendererSkinningSystem.h"
#include "LowRendererTextureState.h"
#include "LowEditorGui.h"
#include <imgui.h>

namespace Low {
  namespace Editor {
    static float calculate_sphere_fit_distance(
        const Math::Sphere &p_Sphere,
        const Renderer::RenderView p_RenderView,
        const float p_Margin = 1.25f)
    {
      const Math::UVector2 l_Dimensions =
          p_RenderView.get_dimensions();
      const float l_Aspect =
          l_Dimensions.y > 0u
              ? (float)l_Dimensions.x / (float)l_Dimensions.y
              : 1.0f;
      const float l_VerticalFov =
          glm::radians(p_RenderView.get_camera_fov());
      const float l_VerticalHalfTan = glm::tan(l_VerticalFov * 0.5f);
      const float l_HorizontalHalfTan = l_VerticalHalfTan * l_Aspect;
      const float l_Radius = glm::max(p_Sphere.radius, 0.05f);

      const float l_VerticalDistance =
          l_Radius / glm::max(l_VerticalHalfTan, 0.001f);
      const float l_HorizontalDistance =
          l_Radius / glm::max(l_HorizontalHalfTan, 0.001f);

      return glm::max(l_VerticalDistance, l_HorizontalDistance) *
             p_Margin;
    }

    static bool setup_bind_pose(
        Renderer::SkeletalRenderObject p_RenderObject,
        Renderer::SkinningPose p_Pose,
        Util::List<Math::Matrix4x4> &p_OutGlobalPose)
    {
      if (!p_RenderObject.is_alive() || !p_Pose.is_alive() ||
          !p_Pose.get_skeleton().is_alive()) {
        return false;
      }

      Renderer::Skeleton l_Skeleton = p_Pose.get_skeleton();
      if (l_Skeleton.get_state() != Renderer::SkeletonState::LOADED) {
        return false;
      }

      Util::List<Renderer::SkeletonBone> &l_Bones =
          l_Skeleton.get_bones();
      if (l_Bones.empty()) {
        return false;
      }

      p_OutGlobalPose.resize(l_Bones.size());
      Util::List<Math::Matrix4x4> &l_PoseMatrices =
          p_Pose.get_matrices();
      l_PoseMatrices.resize(l_Bones.size());

      for (u32 i = 0u; i < (u32)l_Bones.size(); ++i) {
        p_OutGlobalPose[i] = l_Bones[i].global_bind_transform;
        l_PoseMatrices[i] =
            l_Bones[i].global_bind_transform *
            l_Bones[i].inverse_bind_matrix;
      }

      p_Pose.mark_dirty();
      return true;
    }

    Viewport::Viewport(const Math::UVector2 p_Dimensions)
        : m_RenderScene(Renderer::RenderScene::make(N(Viewport))),
          m_RenderView(Renderer::RenderView::make(N(Viewport))),
          m_ViewportHovered(false)
    {
      m_RenderView.set_render_scene(m_RenderScene);
      m_RenderView.set_dimensions(p_Dimensions);

      m_RenderView.add_step_by_name(RENDERSTEP_SHADOW_PASS_NAME);
      m_RenderView.add_step_by_name(RENDERSTEP_SOLID_MATERIAL_NAME);
      m_RenderView.add_step_by_name(RENDERSTEP_SSAO_NAME);
      m_RenderView.add_step_by_name(RENDERSTEP_LIGHTCULLING_NAME);
      m_RenderView.add_step_by_name(RENDERSTEP_CAVITIES_NAME);
      m_RenderView.add_step_by_name(RENDERSTEP_LIGHTING_NAME);
      m_RenderView.add_step_by_name(RENDERSTEP_SSGI_NAME);
    }

    Viewport::~Viewport()
    {
      for (Renderer::RenderObject i_RenderObject : m_RenderObjects) {
        if (i_RenderObject.is_alive()) {
          i_RenderObject.destroy();
        }
      }
      m_RenderObjects.clear();

      if (m_RenderView.is_alive()) {
        m_RenderView.destroy();
      }
      if (m_RenderScene.is_alive()) {
        m_RenderScene.destroy();
      }
    }

    void
    Viewport::set_camera_direction(const Math::Vector3 p_Direction)
    {
      m_RenderView.set_camera_direction(p_Direction);
    }

    Math::Vector3 Viewport::get_camera_direction() const
    {
      return m_RenderView.get_camera_direction();
    }

    void Viewport::set_camera_position(const Math::Vector3 p_Position)
    {
      m_RenderView.set_camera_position(p_Position);
    }

    Math::Vector3 Viewport::get_camera_position() const
    {
      return m_RenderView.get_camera_position();
    }

    void Viewport::set_dimensions(const Math::UVector2 p_Dimensions)
    {
      m_RenderView.set_dimensions(p_Dimensions);
    }

    Renderer::RenderObject
    Viewport::spawn_mesh(Renderer::Mesh p_Mesh,
                         Renderer::Material p_Material)
    {
      Renderer::RenderObject l_RenderObject =
          Renderer::RenderObject::make(m_RenderScene, p_Mesh);
      l_RenderObject.set_material(p_Material);

      m_RenderObjects.push_back(l_RenderObject);

      return l_RenderObject;
    }

    Renderer::RenderObject Viewport::spawn_mesh(Renderer::Mesh p_Mesh)
    {
      return spawn_mesh(p_Mesh, Renderer::get_default_material());
    }

    bool Viewport::tick(const float p_Delta)
    {
      return render_viewport(p_Delta);
    }

    bool Viewport::render_viewport(const float p_Delta)
    {
      if (!get_out_texture().is_alive()) {
        return false;
      }

      const Math::UVector2 l_Dimensions =
          m_RenderView.get_dimensions();

      const ImVec2 l_DrawPos = ImGui::GetCursorScreenPos();

      Renderer::Texture l_OutTexture = get_out_texture();
      if (l_OutTexture.is_imgui_texture_ready()) {
        ImGui::Image(l_OutTexture.get_gpu().get_imgui_texture_id(),
                     ImVec2(l_Dimensions.x, l_Dimensions.y));
      } else {
        ImGui::Dummy(ImVec2(l_Dimensions.x, l_Dimensions.y));
      }

      m_ViewportHovered = ImGui::IsItemHovered();

      if (get_out_texture().get_state() !=
          Renderer::TextureState::LOADED) {
        return true;
        ImGui::SetCursorScreenPos(
            l_DrawPos +
            ImVec2(l_Dimensions.x / 2, l_Dimensions.y / 2));
        Gui::spinner("meshspinner", 20.0f, 5,
                     Math::Color(1.0, 1.0f, 1.0f, 1.0));
      }

      return true;
    }

    MeshViewer::~MeshViewer()
    {
    }

    SkeletalMeshViewer::~SkeletalMeshViewer()
    {
      if (m_RenderObject.is_alive()) {
        m_RenderObject.destroy();
      }
      if (m_SkinningInstance.is_alive()) {
        m_SkinningInstance.destroy();
      }
      if (m_Pose.is_alive()) {
        m_Pose.destroy();
      }
    }

    bool MeshViewer::tick(const float p_Delta)
    {
      if (!Viewport::tick(p_Delta)) {
        return false;
      }

      if (m_RenderObject.get_mesh().get_state() !=
          Renderer::MeshState::LOADED) {
        return true;
      }

      if (!m_LowSpotCalculated) {
        m_LowSpotCalculated = true;

        const float l_LowSpot = m_RenderObject.get_mesh()
                                    .get_gpu()
                                    .get_aabb()
                                    .bounds.min.y -
                                0.1f;

        Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

        l_LocalMatrix = glm::translate(
            l_LocalMatrix, Math::Vector3(0.0f, l_LowSpot, 0.0f));
        l_LocalMatrix *=
            glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        l_LocalMatrix = glm::scale(l_LocalMatrix,
                                   Math::Vector3(50.0f, 0.1f, 50.0f));

        m_GroundRenderObject.set_world_transform(l_LocalMatrix);
      }

      const Math::Sphere l_BoundingSphere =
          m_RenderObject.get_mesh().get_gpu().get_bounding_sphere();
      const Math::Vector3 l_Target = l_BoundingSphere.position;

      if (!m_InitialCameraSetup) {
        m_InitialCameraSetup = true;

        const Math::Vector3 l_Direction =
            glm::normalize(Math::Vector3(0.2f, -0.1f, -1.0f));

        m_CameraOrbitDistance = calculate_sphere_fit_distance(
            l_BoundingSphere, m_RenderView, 1.35f);
        set_camera_direction(l_Direction);
        set_camera_position(l_Target -
                            (l_Direction * m_CameraOrbitDistance));
      }

      Math::Vector3 l_CameraPosition = get_camera_position();
      Math::Vector3 l_Offset = l_CameraPosition - l_Target;

      if (glm::length(l_Offset) < 0.001f) {
        l_Offset = Math::Vector3(0.0f, 0.0f, 1.0f);
      }

      m_CameraOrbitDistance = glm::length(l_Offset);

      if (m_ViewportHovered) {
        ImGuiIO &l_Io = ImGui::GetIO();

        if (l_Io.MouseWheel != 0.0f) {
          const float l_MinDistance =
              glm::max(l_BoundingSphere.radius * 0.25f, 0.05f);
          const float l_ZoomSpeed =
              glm::max(m_CameraOrbitDistance * 0.15f, 0.1f);

          m_CameraOrbitDistance -= l_Io.MouseWheel * l_ZoomSpeed;
          m_CameraOrbitDistance =
              glm::max(m_CameraOrbitDistance, l_MinDistance);

          l_Offset = glm::normalize(l_Offset) * m_CameraOrbitDistance;
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          const ImVec2 l_MouseDelta = l_Io.MouseDelta;

          if (l_MouseDelta.x != 0.0f || l_MouseDelta.y != 0.0f) {
            const float l_RotationSpeed = 0.01f;
            const float l_Yaw = -l_MouseDelta.x * l_RotationSpeed;
            const float l_Pitch = -l_MouseDelta.y * l_RotationSpeed;

            const Math::Vector3 l_Up(0.0f, 1.0f, 0.0f);
            Math::Vector3 l_Right = glm::cross(l_Up, l_Offset);

            if (glm::length(l_Right) > 0.0001f) {
              l_Right = glm::normalize(l_Right);

              const Math::Quaternion l_YawRotation =
                  glm::angleAxis(l_Yaw, l_Up);
              const Math::Quaternion l_PitchRotation =
                  glm::angleAxis(l_Pitch, l_Right);

              Math::Vector3 l_RotatedOffset =
                  l_YawRotation * l_Offset;
              Math::Vector3 l_PitchedOffset =
                  l_PitchRotation * l_RotatedOffset;

              const Math::Vector3 l_ViewDirection =
                  glm::normalize(-l_PitchedOffset);

              if (glm::abs(glm::dot(l_ViewDirection, l_Up)) < 0.99f) {
                l_Offset = l_PitchedOffset;
              } else {
                l_Offset = l_RotatedOffset;
              }
            }
          }
        }
      }

      l_CameraPosition = l_Target + l_Offset;
      set_camera_position(l_CameraPosition);
      set_camera_direction(
          glm::normalize(l_Target - l_CameraPosition));

      return true;
    }

    bool SkeletalMeshViewer::tick(const float p_Delta)
    {
      if (!Viewport::tick(p_Delta)) {
        return false;
      }

      if (m_RenderObject.get_mesh().get_state() !=
          Renderer::MeshState::LOADED) {
        return true;
      }

      if (!m_Pose.get_skeleton().is_alive() &&
          m_RenderObject.get_mesh().get_skeleton().is_alive()) {
        m_Pose.set_skeleton(m_RenderObject.get_mesh().get_skeleton());
      }

      if (!m_BindPoseInitialized) {
        m_BindPoseInitialized =
            setup_bind_pose(m_RenderObject, m_Pose,
                            m_BindGlobalPose);
      }

      if (m_BindPoseInitialized && !m_BindPoseSubmitted) {
        m_BindPoseSubmitted =
            Renderer::SkinningSystem::
                evaluate_global_pose_for_skeletal_renderobject(
                    m_RenderObject, m_Pose.get_skeleton(),
                    m_BindGlobalPose);
      }

      if (!m_LowSpotCalculated) {
        m_LowSpotCalculated = true;

        const float l_LowSpot = m_RenderObject.get_mesh()
                                    .get_gpu()
                                    .get_aabb()
                                    .bounds.min.y -
                                0.1f;

        Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

        l_LocalMatrix = glm::translate(
            l_LocalMatrix, Math::Vector3(0.0f, l_LowSpot, 0.0f));
        l_LocalMatrix *=
            glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        l_LocalMatrix = glm::scale(l_LocalMatrix,
                                   Math::Vector3(50.0f, 0.1f, 50.0f));

        m_GroundRenderObject.set_world_transform(l_LocalMatrix);
      }

      const Math::Sphere l_BoundingSphere =
          m_RenderObject.get_mesh().get_gpu().get_bounding_sphere();
      const Math::Vector3 l_Target = l_BoundingSphere.position;

      if (!m_InitialCameraSetup) {
        m_InitialCameraSetup = true;

        const Math::Vector3 l_Direction =
            glm::normalize(Math::Vector3(0.2f, -0.1f, -1.0f));

        m_CameraOrbitDistance = calculate_sphere_fit_distance(
            l_BoundingSphere, m_RenderView, 1.35f);
        set_camera_direction(l_Direction);
        set_camera_position(l_Target -
                            (l_Direction * m_CameraOrbitDistance));
      }

      Math::Vector3 l_CameraPosition = get_camera_position();
      Math::Vector3 l_Offset = l_CameraPosition - l_Target;

      if (glm::length(l_Offset) < 0.001f) {
        l_Offset = Math::Vector3(0.0f, 0.0f, 1.0f);
      }

      m_CameraOrbitDistance = glm::length(l_Offset);

      if (m_ViewportHovered) {
        ImGuiIO &l_Io = ImGui::GetIO();

        if (l_Io.MouseWheel != 0.0f) {
          const float l_MinDistance =
              glm::max(l_BoundingSphere.radius * 0.25f, 0.05f);
          const float l_ZoomSpeed =
              glm::max(m_CameraOrbitDistance * 0.15f, 0.1f);

          m_CameraOrbitDistance -= l_Io.MouseWheel * l_ZoomSpeed;
          m_CameraOrbitDistance =
              glm::max(m_CameraOrbitDistance, l_MinDistance);

          l_Offset = glm::normalize(l_Offset) * m_CameraOrbitDistance;
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          const ImVec2 l_MouseDelta = l_Io.MouseDelta;

          if (l_MouseDelta.x != 0.0f || l_MouseDelta.y != 0.0f) {
            const float l_RotationSpeed = 0.01f;
            const float l_Yaw = -l_MouseDelta.x * l_RotationSpeed;
            const float l_Pitch = -l_MouseDelta.y * l_RotationSpeed;

            const Math::Vector3 l_Up(0.0f, 1.0f, 0.0f);
            Math::Vector3 l_Right = glm::cross(l_Up, l_Offset);

            if (glm::length(l_Right) > 0.0001f) {
              l_Right = glm::normalize(l_Right);

              const Math::Quaternion l_YawRotation =
                  glm::angleAxis(l_Yaw, l_Up);
              const Math::Quaternion l_PitchRotation =
                  glm::angleAxis(l_Pitch, l_Right);

              Math::Vector3 l_RotatedOffset =
                  l_YawRotation * l_Offset;
              Math::Vector3 l_PitchedOffset =
                  l_PitchRotation * l_RotatedOffset;

              const Math::Vector3 l_ViewDirection =
                  glm::normalize(-l_PitchedOffset);

              if (glm::abs(glm::dot(l_ViewDirection, l_Up)) < 0.99f) {
                l_Offset = l_PitchedOffset;
              } else {
                l_Offset = l_RotatedOffset;
              }
            }
          }
        }
      }

      l_CameraPosition = l_Target + l_Offset;
      set_camera_position(l_CameraPosition);
      set_camera_direction(
          glm::normalize(l_Target - l_CameraPosition));

      return true;
    }

    bool MaterialViewer::tick(const float p_Delta)
    {
      if (!Viewport::tick(p_Delta)) {
        return false;
      }

      if (m_RenderObject.get_mesh().get_state() !=
          Renderer::MeshState::LOADED) {
        return true;
      }

      Math::Sphere l_BoundingSphere =
          m_RenderObject.get_mesh().get_gpu().get_bounding_sphere();

      if (l_BoundingSphere.radius < 0.5f) {
        l_BoundingSphere.position = Math::Vector3(0.0f);
        l_BoundingSphere.radius = 1.0f;
      }

      const Math::Vector3 l_Target = l_BoundingSphere.position;

      if (!m_InitialCameraSetup) {
        m_InitialCameraSetup = true;

        const Math::Vector3 l_Direction =
            glm::normalize(Math::Vector3(0.2f, -0.1f, -1.0f));

        m_CameraOrbitDistance = calculate_sphere_fit_distance(
            l_BoundingSphere, m_RenderView, 1.35f);
        set_camera_direction(l_Direction);
        set_camera_position(l_Target -
                            (l_Direction * m_CameraOrbitDistance));
      }

      Math::Vector3 l_CameraPosition = get_camera_position();
      Math::Vector3 l_Offset = l_CameraPosition - l_Target;

      if (glm::length(l_Offset) < 0.001f) {
        l_Offset = Math::Vector3(0.0f, 0.0f, 1.0f);
      }

      m_CameraOrbitDistance = glm::length(l_Offset);

      if (m_ViewportHovered) {
        ImGuiIO &l_Io = ImGui::GetIO();

        if (l_Io.MouseWheel != 0.0f) {
          const float l_MinDistance =
              glm::max(l_BoundingSphere.radius * 0.25f, 0.05f);
          const float l_ZoomSpeed =
              glm::max(m_CameraOrbitDistance * 0.15f, 0.1f);

          m_CameraOrbitDistance -= l_Io.MouseWheel * l_ZoomSpeed;
          m_CameraOrbitDistance =
              glm::max(m_CameraOrbitDistance, l_MinDistance);

          l_Offset = glm::normalize(l_Offset) * m_CameraOrbitDistance;
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          const ImVec2 l_MouseDelta = l_Io.MouseDelta;

          if (l_MouseDelta.x != 0.0f || l_MouseDelta.y != 0.0f) {
            const float l_RotationSpeed = 0.01f;
            const float l_Yaw = -l_MouseDelta.x * l_RotationSpeed;
            const float l_Pitch = -l_MouseDelta.y * l_RotationSpeed;

            const Math::Vector3 l_Up(0.0f, 1.0f, 0.0f);
            Math::Vector3 l_Right = glm::cross(l_Up, l_Offset);

            if (glm::length(l_Right) > 0.0001f) {
              l_Right = glm::normalize(l_Right);

              const Math::Quaternion l_YawRotation =
                  glm::angleAxis(l_Yaw, l_Up);
              const Math::Quaternion l_PitchRotation =
                  glm::angleAxis(l_Pitch, l_Right);

              Math::Vector3 l_RotatedOffset =
                  l_YawRotation * l_Offset;
              Math::Vector3 l_PitchedOffset =
                  l_PitchRotation * l_RotatedOffset;

              const Math::Vector3 l_ViewDirection =
                  glm::normalize(-l_PitchedOffset);

              if (glm::abs(glm::dot(l_ViewDirection, l_Up)) < 0.99f) {
                l_Offset = l_PitchedOffset;
              } else {
                l_Offset = l_RotatedOffset;
              }
            }
          }
        }
      }

      l_CameraPosition = l_Target + l_Offset;
      set_camera_position(l_CameraPosition);
      set_camera_direction(
          glm::normalize(l_Target - l_CameraPosition));

      return true;
    }

    bool UiViewport::tick(const float p_Delta)
    {
      if (!Viewport::tick(p_Delta)) {
        return false;
      }
      return true;
    }
  } // namespace Editor
} // namespace Low
