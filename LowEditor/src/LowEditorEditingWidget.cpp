#include "LowEditorEditingWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"
#include "LowRenderer.h"

#include "LowMathQuaternionUtil.h"

namespace Low {
  namespace Editor {
    EditingWidget::EditingWidget() : m_CameraSpeed(3.5f)
    {
      m_RenderFlowWidget =
          new RenderFlowWidget("Viewport", Renderer::get_main_renderflow());
      m_LastPitchYaw = Math::Vector2(0.0f, -90.0f);

      for (auto it = Renderer::MaterialType::ms_LivingInstances.begin();
           it != Renderer::MaterialType::ms_LivingInstances.end(); ++it) {

        if (it->get_name() == N(debuggeometry)) {
          m_Material = Renderer::create_material(N(DebugGeometryMaterial), *it);
          m_Material.set_property(
              N(fallback_color),
              Util::Variant(Math::Vector4(0.2f, 0.8f, 1.0f, 1.0f)));

          return;
        }
      }
      LOW_ASSERT(false, "Could not find material type");
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

    void EditingWidget::camera_movement(float p_Delta)
    {
      Math::Vector3 l_CameraPosition =
          m_RenderFlowWidget->get_renderflow().get_camera_position();
      Math::Vector3 l_CameraDirection =
          m_RenderFlowWidget->get_renderflow().get_camera_direction();

      Math::Vector3 l_CameraFront = l_CameraDirection * -1.0f;

      Math::Vector3 l_CameraRight =
          glm::normalize(glm::cross(LOW_VECTOR3_UP, l_CameraFront)) * -1.0f;
      Math::Vector3 l_CameraUp =
          glm::cross(l_CameraFront, l_CameraRight) * -1.0f;

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
      }

      m_RenderFlowWidget->get_renderflow().set_camera_position(
          l_CameraPosition);
    }

    void EditingWidget::render(float p_Delta)
    {
      m_RenderFlowWidget->render(p_Delta);

      if (m_RenderFlowWidget->is_hovered()) {
        camera_movement(p_Delta);
      }

      m_LastMousePosition = m_RenderFlowWidget->get_relative_hover_position();

      if (!Renderer::Mesh::ms_LivingInstances.empty()) {
        Renderer::RenderObject l_RenderObject;
        l_RenderObject.world_position = Math::Vector3(1.0f, 2.0f, -10.0f);
        l_RenderObject.world_rotation =
            Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        l_RenderObject.world_scale = Math::Vector3(1.0f);
        l_RenderObject.color = Math::Color(1.0f, 0.0, 0.0f, 1.0f);
        l_RenderObject.entity_id = 0;
        l_RenderObject.mesh = Renderer::Mesh::ms_LivingInstances[0];
        l_RenderObject.material = m_Material;

        Renderer::get_main_renderflow().register_renderobject(l_RenderObject);
      }
    }
  } // namespace Editor
} // namespace Low
