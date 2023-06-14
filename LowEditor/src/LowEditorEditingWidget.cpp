#include "LowEditorEditingWidget.h"

#include "LowEditorMainWindow.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"
#include "IconsFontAwesome5.h"
#include "LowRenderer.h"

#include "LowCore.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreDirectionalLight.h"
#include "LowCorePointLight.h"
#include "LowCoreRigidbody.h"

#include "LowUtilEnums.h"

#include "LowMathQuaternionUtil.h"
#include <stdint.h>

#define LOW_EDITOR_BILLBOARD_SIZE 0.5f

namespace Low {
  namespace Editor {
    void render_billboards(float p_Delta, RenderFlowWidget &p_RenderFlowWidget)
    {
      Renderer::RenderFlow l_RenderFlow = p_RenderFlowWidget.get_renderflow();

      Helper::SphericalBillboardMaterials l_Materials =
          Helper::get_spherical_billboard_materials();

      for (Core::Component::DirectionalLight i_Light :
           Core::Component::DirectionalLight::ms_LivingInstances) {
        Core::Component::Transform i_Transform =
            i_Light.get_entity().get_transform();

        float l_ScreenSpaceAdjustment =
            Core::DebugGeometry::screen_space_multiplier(
                l_RenderFlow, i_Transform.get_world_position());

        Core::DebugGeometry::render_spherical_billboard(
            i_Transform.get_world_position(),
            LOW_EDITOR_BILLBOARD_SIZE * l_ScreenSpaceAdjustment,
            l_Materials.sun);
      }

      for (Core::Component::PointLight i_Light :
           Core::Component::PointLight::ms_LivingInstances) {
        Core::Component::Transform i_Transform =
            i_Light.get_entity().get_transform();

        float l_ScreenSpaceAdjustment =
            Core::DebugGeometry::screen_space_multiplier(
                l_RenderFlow, i_Transform.get_world_position());

        Core::DebugGeometry::render_spherical_billboard(
            i_Transform.get_world_position(),
            LOW_EDITOR_BILLBOARD_SIZE * l_ScreenSpaceAdjustment,
            l_Materials.bulb);
      }
    }

    void render_region_gizmos(float p_Delta,
                              RenderFlowWidget &p_RenderFlowWidget,
                              Core::Region p_Region)
    {
      Renderer::RenderFlow l_RenderFlow = p_RenderFlowWidget.get_renderflow();

      if (p_Region.is_streaming_enabled()) {
        float l_ScreenSpaceAdjustment =
            Core::DebugGeometry::screen_space_multiplier(
                l_RenderFlow, p_Region.get_streaming_position());

        Helper::SphericalBillboardMaterials l_Materials =
            Helper::get_spherical_billboard_materials();

        Math::Cylinder l_StreamingCylinder;
        l_StreamingCylinder.position = p_Region.get_streaming_position();
        l_StreamingCylinder.radius = p_Region.get_streaming_radius();
        l_StreamingCylinder.height = 75.0f;
        l_StreamingCylinder.rotation = Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

        Core::DebugGeometry::render_cylinder(
            l_StreamingCylinder, Math::Color(1.0f, 1.0f, 0.0f, 1.0f), true,
            true);

        Core::DebugGeometry::render_spherical_billboard(
            p_Region.get_streaming_position(),
            LOW_EDITOR_BILLBOARD_SIZE * l_ScreenSpaceAdjustment,
            l_Materials.region);
      }
    }

    static void
    render_rigidbody_debug_geometry(float p_Delta,
                                    RenderFlowWidget &p_RenderFlowWidget)
    {
      if (!get_selected_entity().is_alive()) {
        return;
      }

      if (!get_selected_entity().has_component(
              Core::Component::Rigidbody::TYPE_ID)) {
        return;
      }

      Core::Component::Transform l_Transform =
          get_selected_entity().get_transform();
      Core::Component::Rigidbody l_Rigidbody =
          get_selected_entity().get_component(
              Core::Component::Rigidbody::TYPE_ID);

      Math::Color l_DrawColor(0.0f, 1.0f, 0.0f, 1.0f);

      if (l_Rigidbody.get_shape().type == Math::ShapeType::BOX) {
        Math::Box l_Box = l_Rigidbody.get_shape().box;
        l_Box.position = l_Rigidbody.get_rigid_dynamic().get_position();
        l_Box.rotation = l_Rigidbody.get_rigid_dynamic().get_rotation();

        Core::DebugGeometry::render_box(l_Box, l_DrawColor, false, true);
      }
    }

    void render_gizmos(float p_Delta, RenderFlowWidget &p_RenderFlowWidget)
    {
      const float l_TopPadding = 40.0f;

      ImVec2 l_WindowPos = ImGui::GetWindowPos();

      ImGui::SetNextWindowPos(
          {p_RenderFlowWidget.get_widget_position().x +
               (p_RenderFlowWidget.get_renderflow().get_dimensions().x / 2.0f) +
               10.0f,
           p_RenderFlowWidget.get_widget_position().y + l_TopPadding});

      ImGui::BeginChild(
          "Controls", ImVec2(30, 30), false,
          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking |
              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
              ImGuiWindowFlags_NoScrollWithMouse |
              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
              ImGuiWindowFlags_AlwaysAutoResize);
      if (Core::get_engine_state() == Util::EngineState::EDITING) {
        if (ImGui::Button(ICON_FA_PLAY)) {
          set_selected_entity(0);
          Core::begin_playmode();
        }
      } else if (Core::get_engine_state() == Util::EngineState::PLAYING) {
        if (ImGui::Button(ICON_FA_STOP)) {
          Core::exit_playmode();
        }
      }
      ImGui::EndChild();

      if (Core::get_engine_state() != Util::EngineState::EDITING) {
        return;
      }

      render_rigidbody_debug_geometry(p_Delta, p_RenderFlowWidget);

      render_billboards(p_Delta, p_RenderFlowWidget);

      Renderer::RenderFlow l_RenderFlow = p_RenderFlowWidget.get_renderflow();

      static ImGuizmo::OPERATION l_Operation = ImGuizmo::TRANSLATE;

      ImGui::SetNextWindowPos(
          {p_RenderFlowWidget.get_widget_position().x + 20.0f,
           p_RenderFlowWidget.get_widget_position().y + l_TopPadding});

      ImGui::BeginChild(
          "Tools", ImVec2(30, 200), false,
          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking |
              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
              ImGuiWindowFlags_NoScrollWithMouse |
              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
              ImGuiWindowFlags_AlwaysAutoResize);
      if (ImGui::Button(ICON_FA_ARROWS_ALT)) {
        l_Operation = ImGuizmo::TRANSLATE;
      }
      // ImGui::SameLine();
      if (ImGui::Button(ICON_FA_SYNC)) {
        l_Operation = ImGuizmo::ROTATE;
      }
      // ImGui::SameLine();
      if (ImGui::Button(ICON_FA_ARROWS_ALT_H)) {
        l_Operation = ImGuizmo::SCALE;
      }
      ImGui::EndChild();

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

        Math::Matrix4x4 l_TransformMatrix =
            glm::translate(glm::mat4(1.0f), l_Transform.get_world_position()) *
            glm::toMat4(l_Transform.get_world_rotation()) *
            glm::scale(glm::mat4(1.0f), l_Transform.get_world_scale());

        if (ImGuizmo::Manipulate((float *)&l_ViewMatrix,
                                 (float *)&l_ProjectionMatrix, l_Operation,
                                 ImGuizmo::LOCAL, (float *)&l_TransformMatrix,
                                 NULL, NULL, NULL, NULL)) {

          glm::vec3 scale;
          glm::quat rotation;
          glm::vec3 translation;
          glm::vec3 skew;
          glm::vec4 perspective;
          glm::decompose(l_TransformMatrix, scale, rotation, translation, skew,
                         perspective);

          l_Transform.position(translation);
          l_Transform.rotation(rotation);
          l_Transform.scale(scale);
        }
      }

      Core::Region l_Region = get_selected_handle().get_id();
      if (l_Region.is_alive()) {
        render_region_gizmos(p_Delta, p_RenderFlowWidget, l_Region);
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

      float m_Sensitivity = 5000.0f * m_CameraSpeed;

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

    void EditingWidget::render_editing(float p_Delta)
    {
      ImGuiIO &io = ImGui::GetIO();

      bool l_AllowRendererBasedPicking = m_RenderFlowWidget->is_hovered();

      if (m_RenderFlowWidget->is_hovered()) {
        l_AllowRendererBasedPicking =
            l_AllowRendererBasedPicking && !camera_movement(p_Delta);
      }

      if (io.MouseWheel > 0.05f) {
        m_CameraSpeed += 0.25f;
      } else if (io.MouseWheel < -0.05f) {
        m_CameraSpeed -= 0.25f;
      }

      m_CameraSpeed = Math::Util::clamp(m_CameraSpeed, 0.1f, 15.0f);

      m_LastMousePosition = m_RenderFlowWidget->get_relative_hover_position();

      if (ImGuizmo::IsOver()) {
        l_AllowRendererBasedPicking = false;
      }
      if (ImGui::IsAnyItemHovered()) {
        l_AllowRendererBasedPicking = false;
      }

      Math::Vector2 l_HoverCoordinates = {2.0f, 2.0f};

      if (l_AllowRendererBasedPicking) {
        l_HoverCoordinates = m_RenderFlowWidget->get_relative_hover_position();

        uint32_t l_EntityIndex;
        m_RenderFlowWidget->get_renderflow()
            .get_resources()
            .get_buffer_resource(N(IdWritebackBuffer))
            .read(&l_EntityIndex, sizeof(uint32_t), 0);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {

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

    void EditingWidget::render(float p_Delta)
    {
      m_RenderFlowWidget->render(p_Delta);

      if (Core::get_engine_state() == Util::EngineState::EDITING) {
        render_editing(p_Delta);
      }
    }
  } // namespace Editor
} // namespace Low
