#include "LowEditorEditingWidget.h"

#include "LowEditorMainWindow.h"

#include "imgui.h"
#include "ImGuizmo.h"
#include "IconsFontAwesome5.h"
#include "LowRenderer.h"

#include "LowCoreDebugGeometry.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"

#include "LowMathQuaternionUtil.h"

namespace Low {
  namespace Editor {
    void render_gizmos(float p_Delta, RenderFlowWidget &p_RenderFlowWidget)
    {
      Renderer::RenderFlow l_RenderFlow = p_RenderFlowWidget.get_renderflow();

      static bool l_ToolsWindowOpen = true;

      static ImGuizmo::OPERATION l_Operation = ImGuizmo::TRANSLATE;

      ImGui::Begin("Tools", &l_ToolsWindowOpen,
                   ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking |
                       ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_NoDecoration |
                       ImGuiWindowFlags_AlwaysAutoResize);
      if (ImGui::Button(ICON_FA_ARROWS_ALT)) {
        l_Operation = ImGuizmo::TRANSLATE;
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_SYNC)) {
        l_Operation = ImGuizmo::ROTATE;
      }
      ImGui::End();

      static const float identityMatrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f,
                                               0.f, 0.f, 0.f, 0.f, 1.f, 0.f,
                                               0.f, 0.f, 0.f, 1.f};

      ImGuizmo::SetDrawlist();
      ImGuizmo::SetOrthographic(false);
      float windowWidth = (float)ImGui::GetWindowWidth();
      float windowHeight = (float)ImGui::GetWindowHeight();
      ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
                        windowWidth, windowHeight);

      Math::Matrix4x4 l_ProjectionMatrix =
          glm::perspective(glm::radians(l_RenderFlow.get_camera_fov()),
                           ((float)l_RenderFlow.get_dimensions().x) /
                               ((float)l_RenderFlow.get_dimensions().y),
                           l_RenderFlow.get_camera_near_plane(),
                           l_RenderFlow.get_camera_far_plane());

      Math::Matrix4x4 l_ViewMatrix =
          glm::lookAt(l_RenderFlow.get_camera_position(),
                      l_RenderFlow.get_camera_position() +
                          l_RenderFlow.get_camera_direction(),
                      LOW_VECTOR3_UP);

      Core::Entity l_Entity = get_selected_entity();

      if (l_Entity.is_alive()) {
        Core::Component::Transform l_Transform = l_Entity.get_transform();

        Math::Matrix4x4 l_ModelMatrix =
            glm::translate(glm::mat4(1.0f), l_Transform.get_world_position()) *
            glm::toMat4(l_Transform.get_world_rotation()) *
            glm::scale(glm::mat4(1.0f), l_Transform.get_world_scale());

        /*
              ImGuizmo::DrawGrid((float *)&l_ViewMatrix, (float
           *)&l_ProjectionMatrix, identityMatrix, 100.f);
        */

        /*
              ImGuizmo::DrawCubes((float *)&l_ViewMatrix,
                                  (float *)&l_ProjectionMatrix,
                                  (float *)&l_ModelMatrix, 1);
        */

        if (ImGuizmo::Manipulate((float *)&l_ViewMatrix,
                                 (float *)&l_ProjectionMatrix, l_Operation,
                                 ImGuizmo::LOCAL, (float *)&l_ModelMatrix, NULL,
                                 NULL, NULL, NULL)) {
          Math::Vector3 l_Position;
          Math::Vector3 l_Rotation;
          Math::Vector3 l_Scale;

          ImGuizmo::DecomposeMatrixToComponents(
              (float *)&l_ModelMatrix, (float *)&l_Position,
              (float *)&l_Rotation, (float *)&l_Scale);

          Math::Quaternion l_RotQuat = glm::quat_cast(l_ModelMatrix);

          l_Transform.position(l_Position);
          l_Transform.rotation(l_RotQuat);
          /*
                l_Transform.scale(l_Scale);
          */
        }
      }
    }

    EditingWidget::EditingWidget() : m_CameraSpeed(3.5f)
    {
      m_RenderFlowWidget =
          new RenderFlowWidget(ICON_FA_EYE " Viewport",
                               Renderer::get_main_renderflow(), &render_gizmos);
      m_LastPitchYaw = Math::Vector2(0.0f, -90.0f);
    }

    void EditingWidget::set_camera_rotation(const float p_Pitch,
                                            const float p_Yaw)
    {
      Math::Quaternion l_CameraDirection =
          m_RenderFlowWidget->get_renderflow().get_camera_direction();

      float l_Pitch = m_LastPitchYaw.x;
      float l_Yaw = m_LastPitchYaw.y;
      l_Pitch -= p_Pitch;
      l_Yaw -= p_Yaw;

      const float l_PitchMin = -85.f;
      const float l_PitchMax = 85.f;
      l_Pitch = Math::Util::clamp(l_Pitch, l_PitchMin, l_PitchMax);

      const float l_YawRadian = glm::radians(l_Yaw);
      const float l_PitchRadian = glm::radians(l_Pitch);

      Math::Vector3 l_Forward = Math::Vector3(
          cos(l_PitchRadian) * cos(l_YawRadian), -sin(l_PitchRadian),
          cos(l_PitchRadian) * sin(l_YawRadian));

      m_RenderFlowWidget->get_renderflow().set_camera_direction(l_Forward);

      m_LastPitchYaw.x = l_Pitch;
      m_LastPitchYaw.y = l_Yaw;
    }

    bool EditingWidget::camera_movement(float p_Delta)
    {
      bool l_ReturnValue = false;

      Math::Vector3 l_CameraPosition =
          m_RenderFlowWidget->get_renderflow().get_camera_position();
      Math::Vector3 l_CameraDirection =
          m_RenderFlowWidget->get_renderflow().get_camera_direction();

      Math::Vector3 l_CameraFront = l_CameraDirection * -1.0f;

      Math::Vector3 l_CameraRight =
          glm::normalize(glm::cross(LOW_VECTOR3_UP, l_CameraFront)) * -1.0f;
      Math::Vector3 l_CameraUp =
          glm::cross(l_CameraFront, l_CameraRight) * -1.0f;

      if (!ImGui::IsAnyItemActive()) {
        if (Renderer::get_window().keyboard_button_down(
                Renderer::Input::KeyboardButton::W)) {
          l_CameraPosition -= (l_CameraFront * p_Delta * m_CameraSpeed);
        }
        if (Renderer::get_window().keyboard_button_down(
                Renderer::Input::KeyboardButton::S)) {
          l_CameraPosition += (l_CameraFront * p_Delta * m_CameraSpeed);
        }

        if (Renderer::get_window().keyboard_button_down(
                Renderer::Input::KeyboardButton::A)) {
          l_CameraPosition += (l_CameraRight * p_Delta * m_CameraSpeed);
        }
        if (Renderer::get_window().keyboard_button_down(
                Renderer::Input::KeyboardButton::D)) {
          l_CameraPosition -= (l_CameraRight * p_Delta * m_CameraSpeed);
        }

        if (Renderer::get_window().keyboard_button_down(
                Renderer::Input::KeyboardButton::Q)) {
          l_CameraPosition += (l_CameraUp * p_Delta * m_CameraSpeed);
        }
        if (Renderer::get_window().keyboard_button_down(
                Renderer::Input::KeyboardButton::E)) {
          l_CameraPosition -= (l_CameraUp * p_Delta * m_CameraSpeed);
        }
      }

      float m_Sensitivity = 3000.0f;

      Math::Vector2 l_MousePositionDifference =
          m_LastMousePosition -
          m_RenderFlowWidget->get_relative_hover_position();

      l_MousePositionDifference *= m_Sensitivity * p_Delta;

      if (m_LastMousePosition.x < 1.5f && m_LastMousePosition.y < 1.5f &&
          Renderer::get_window().mouse_button_down(
              Renderer::Input::MouseButton::RIGHT)) {

        set_camera_rotation(l_MousePositionDifference.y,
                            l_MousePositionDifference.x);

        l_ReturnValue = true;
      }

      m_RenderFlowWidget->get_renderflow().set_camera_position(
          l_CameraPosition);

      return l_ReturnValue;
    }

    void EditingWidget::render(float p_Delta)
    {
      m_RenderFlowWidget->render(p_Delta);

      bool l_AllowRendererBasedPicking = m_RenderFlowWidget->is_hovered();

      if (m_RenderFlowWidget->is_hovered()) {
        l_AllowRendererBasedPicking =
            l_AllowRendererBasedPicking && !camera_movement(p_Delta);
      }

      m_LastMousePosition = m_RenderFlowWidget->get_relative_hover_position();

      /*
      Math::Box l_Box;
      l_Box.position = {1.0f, 2.0f, -10.0f};
      l_Box.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
      l_Box.halfExtents = {0.5f, 0.5f, 0.5f};

      Core::DebugGeometry::render_box(l_Box, {0.0f, 1.0f, 0.0f, 1.0f}, false,
                                      true);

      Math::Cylinder l_Cylinder;
      l_Cylinder.position = {1.0f, 2.0f, 3.0f};
      l_Cylinder.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
      l_Cylinder.radius = 0.3f;
      l_Cylinder.height = 8.0f;

      Core::DebugGeometry::render_cylinder(l_Cylinder, {0.0f, 0.0f, 1.0f, 1.0f},
                                           true, false);

      Math::Cone l_Cone;
      l_Cone.position = {4.0f, 2.0f, 3.0f};
      l_Cone.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
      l_Cone.radius = 0.3f;
      l_Cone.height = 0.8f;

      Core::DebugGeometry::render_cone(l_Cone, {1.0f, 0.0f, 1.0f, 1.0f}, true,
                                       true);

      Math::Vector3 l_ArrowPosition = {-3.0f, 2.0f, 5.0f};
      float l_ScreenSpaceMultiplier =
          Core::DebugGeometry::screen_space_multiplier(
              m_RenderFlowWidget->get_renderflow(), l_ArrowPosition);

      Core::DebugGeometry::render_arrow(
          l_ArrowPosition, {0.0f, 0.0f, 0.0f, 1.0f},
          0.6f * l_ScreenSpaceMultiplier, 0.05f * l_ScreenSpaceMultiplier,
          0.13f * l_ScreenSpaceMultiplier, 0.15f * l_ScreenSpaceMultiplier,
          {1.0f, 0.0f, 0.0f, 1.0f}, true, false);
      */

      if (ImGuizmo::IsOver()) {
        l_AllowRendererBasedPicking = false;
      }
      if (ImGui::IsAnyItemHovered()) {
        l_AllowRendererBasedPicking = false;
      }

      Math::Vector2 l_HoverCoordinates = {2.0f, 2.0f};

      if (l_AllowRendererBasedPicking) {
        l_HoverCoordinates = m_RenderFlowWidget->get_relative_hover_position();

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
          uint32_t l_EntityIndex;
          m_RenderFlowWidget->get_renderflow()
              .get_resources()
              .get_buffer_resource(N(IdWritebackBuffer))
              .read(&l_EntityIndex, sizeof(uint32_t), 0);

          Core::Entity l_Entity;
          if (l_EntityIndex != ~0u) {
            l_Entity = Core::Entity::find_by_index(l_EntityIndex);
          }

          set_selected_entity(l_Entity);
        }
      }

      m_RenderFlowWidget->get_renderflow()
          .get_resources()
          .get_buffer_resource(N(HoverCoordinatesBuffer))
          .set(&l_HoverCoordinates);
    }
  } // namespace Editor
} // namespace Low
