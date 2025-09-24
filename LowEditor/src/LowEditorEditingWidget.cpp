#include "LowEditorEditingWidget.h"

#include "LowEditor.h"
#include "LowEditorMainWindow.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorGui.h"
#include "LowEditorBase.h"

#include "LowRendererEditorImage.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"
#include "IconsFontAwesome5.h"
#include "IconsLucide.h"
#include "LowRenderer.h"

#include "LowCore.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreDirectionalLight.h"
#include "LowCorePointLight.h"
#include "LowCoreRigidbody.h"
#include "LowCoreNavmeshAgent.h"
#include "LowCoreCamera.h"

#include "LowUtilEnums.h"
#include "LowUtilGlobals.h"

#include "LowMathQuaternionUtil.h"
#include <stdint.h>

#define LOW_EDITOR_BILLBOARD_SIZE 0.5f

namespace Low {
  namespace Editor {
    void render_billboards(float p_Delta,
                           RenderViewWidget &p_RenderViewWidget)
    {
      Renderer::RenderView l_RenderView =
          p_RenderViewWidget.get_renderview();

      for (Core::Component::DirectionalLight i_Light :
           Core::Component::DirectionalLight::ms_LivingInstances) {
        Core::Component::Transform i_Transform =
            i_Light.get_entity().get_transform();

        float l_ScreenSpaceAdjustment =
            Core::DebugGeometry::screen_space_multiplier(
                l_RenderView, i_Transform.get_world_position());

        Core::DebugGeometry::render_spherical_billboard(
            i_Transform.get_world_position(),
            LOW_EDITOR_BILLBOARD_SIZE * l_ScreenSpaceAdjustment,
            Renderer::EditorImage::find_by_name(
                N(directional_light)));
        // TODO: Fix lookup by name
      }

      for (Core::Component::PointLight i_Light :
           Core::Component::PointLight::ms_LivingInstances) {
        Core::Component::Transform i_Transform =
            i_Light.get_entity().get_transform();

        float l_ScreenSpaceAdjustment =
            Core::DebugGeometry::screen_space_multiplier(
                l_RenderView, i_Transform.get_world_position());

        Core::DebugGeometry::render_spherical_billboard(
            i_Transform.get_world_position(),
            LOW_EDITOR_BILLBOARD_SIZE * l_ScreenSpaceAdjustment,
            Renderer::EditorImage::find_by_name(N(point_light)));
        // TODO: Fix lookup by name
      }

      for (Core::Component::Camera i_Camera :
           Core::Component::Camera::ms_LivingInstances) {
        Core::Component::Transform i_Transform =
            i_Camera.get_entity().get_transform();

        float l_ScreenSpaceAdjustment =
            Core::DebugGeometry::screen_space_multiplier(
                l_RenderView, i_Transform.get_world_position());

        // TODO: Fix
        Core::DebugGeometry::render_spherical_billboard(
            i_Transform.get_world_position(),
            LOW_EDITOR_BILLBOARD_SIZE * l_ScreenSpaceAdjustment,
            Util::Handle::DEAD);
      }
    }

    void render_region_gizmos(float p_Delta,
                              RenderViewWidget &p_RenderViewWidget,
                              Core::Region p_Region)
    {
      Renderer::RenderView l_RenderView =
          p_RenderViewWidget.get_renderview();

      if (p_Region.is_streaming_enabled()) {
        float l_ScreenSpaceAdjustment =
            Core::DebugGeometry::screen_space_multiplier(
                l_RenderView, p_Region.get_streaming_position());

        Helper::SphericalBillboardMaterials l_Materials =
            Helper::get_spherical_billboard_materials();

        Math::Cylinder l_StreamingCylinder;
        l_StreamingCylinder.position =
            p_Region.get_streaming_position();
        l_StreamingCylinder.radius = p_Region.get_streaming_radius();
        l_StreamingCylinder.height = 75.0f;
        l_StreamingCylinder.rotation =
            Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

        Core::DebugGeometry::render_cylinder(
            l_StreamingCylinder, Math::Color(1.0f, 1.0f, 0.0f, 1.0f),
            true, true);
        Core::DebugGeometry::render_cylinder(
            l_StreamingCylinder, Math::Color(1.0f, 1.0f, 0.0f, 0.1f),
            true, false);

        // TODO: Fix
        Core::DebugGeometry::render_spherical_billboard(
            p_Region.get_streaming_position(),
            LOW_EDITOR_BILLBOARD_SIZE * l_ScreenSpaceAdjustment,
            Util::Handle::DEAD);
      }
    }

    static void render_rigidbody_debug_geometry(
        float p_Delta, RenderViewWidget &p_RenderViewWidget)
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
        l_Box.position =
            l_Rigidbody.get_rigid_dynamic().get_position();
        l_Box.rotation =
            l_Rigidbody.get_rigid_dynamic().get_rotation();

        Core::DebugGeometry::render_box(l_Box, l_DrawColor, false,
                                        true);
      }
    }

    static void render_pointlight_debug_geometry(
        float p_Delta, RenderViewWidget &p_RenderViewWidget)
    {
      if (!get_selected_entity().is_alive()) {
        return;
      }

      if (!get_selected_entity().has_component(
              Core::Component::PointLight::TYPE_ID)) {
        return;
      }

      Core::Component::Transform l_Transform =
          get_selected_entity().get_transform();
      Core::Component::PointLight l_PointLight =
          get_selected_entity().get_component(
              Core::Component::PointLight::TYPE_ID);

      Math::Color l_DrawColor(0.0f, 1.0f, 0.0f, 1.0f);

      Math::Sphere l_Sphere;
      l_Sphere.position = l_Transform.get_world_position();
      l_Sphere.radius = l_PointLight.get_range();

      Core::DebugGeometry::render_sphere(
          l_Sphere, Math::Color(1.0f, 1.0f, 0.0f, 1.0f), false, true);
    }

    static void render_navmeshagent_debug_geometry(
        float p_Delta, RenderViewWidget &p_RenderViewWidget)
    {
      if (!get_selected_entity().is_alive()) {
        return;
      }

      if (!get_selected_entity().has_component(
              Core::Component::NavmeshAgent::TYPE_ID)) {
        return;
      }

      Core::Component::Transform l_Transform =
          get_selected_entity().get_transform();
      Core::Component::NavmeshAgent l_NavmeshAgent =
          get_selected_entity().get_component(
              Core::Component::NavmeshAgent::TYPE_ID);

      Math::Color l_DrawColor(0.0f, 1.0f, 0.0f, 1.0f);

      Math::Cylinder l_Cylinder;
      l_Cylinder.height = l_NavmeshAgent.get_height();
      l_Cylinder.radius = l_NavmeshAgent.get_radius();
      l_Cylinder.position = l_Transform.get_world_position();
      l_Cylinder.position += l_NavmeshAgent.get_offset();
      l_Cylinder.position.y += l_Cylinder.height / 2.0f;
      l_Cylinder.rotation = Math::QuaternionUtil::get_identity();

      Core::DebugGeometry::render_cylinder(
          l_Cylinder, Math::Color(1.0f, 0.0f, 1.0f, 1.0f), false,
          true);
    }

    void render_gizmos(float p_Delta,
                       RenderViewWidget &p_RenderViewWidget)
    {
      const float l_TopPadding = 40.0f;

      ImVec2 l_WindowPos = ImGui::GetWindowPos();

      ImGui::SetNextWindowPos(
          {p_RenderViewWidget.get_widget_position().x +
               (p_RenderViewWidget.get_renderview()
                    .get_dimensions()
                    .x /
                2.0f) +
               10.0f,
           p_RenderViewWidget.get_widget_position().y +
               l_TopPadding});

      ImGui::BeginChild("Controls", ImVec2(30, 30), false,
                        ImGuiWindowFlags_NoBackground |
                            ImGuiWindowFlags_NoDocking |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse |
                            ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoDecoration);
      if (Core::get_engine_state() == Util::EngineState::EDITING) {
        if (ImGui::Button(ICON_LC_PLAY)) {
          set_selected_entity(0);
          Core::begin_playmode();
        }
      } else if (Core::get_engine_state() ==
                 Util::EngineState::PLAYING) {
        if (ImGui::Button(ICON_LC_PAUSE)) {
          Core::exit_playmode();
        }
      }
      ImGui::EndChild();

      if (Core::get_engine_state() != Util::EngineState::EDITING) {
        return;
      }

      render_rigidbody_debug_geometry(p_Delta, p_RenderViewWidget);
      render_navmeshagent_debug_geometry(p_Delta, p_RenderViewWidget);
      render_pointlight_debug_geometry(p_Delta, p_RenderViewWidget);

      render_billboards(p_Delta, p_RenderViewWidget);

      Renderer::RenderView l_RenderView =
          p_RenderViewWidget.get_renderview();

      static ImGuizmo::OPERATION l_Operation = ImGuizmo::TRANSLATE;

      if (p_RenderViewWidget.is_hovered()) {
        if (ImGui::IsKeyPressed(ImGuiKey_1)) {
          l_Operation = ImGuizmo::TRANSLATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_2)) {
          l_Operation = ImGuizmo::ROTATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_3)) {
          l_Operation = ImGuizmo::SCALE;
        }
      }

      ImGui::SetNextWindowPos(
          {p_RenderViewWidget.get_widget_position().x + 20.0f,
           p_RenderViewWidget.get_widget_position().y +
               l_TopPadding});

      ImGui::BeginChild("Tools", ImVec2(50, 280), false,
                        ImGuiWindowFlags_NoBackground |
                            ImGuiWindowFlags_NoDocking |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse |
                            ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoDecoration);

      ImGui::PushFont(Fonts::UI(16));

      int l_ButtonSize = 45;

      if (ImGui::Button(ICON_LC_MOVE_3D,
                        ImVec2(l_ButtonSize, l_ButtonSize))) {
        l_Operation = ImGuizmo::TRANSLATE;
      }
      // ImGui::SameLine();
      if (ImGui::Button(ICON_LC_ROTATE_3D,
                        ImVec2(l_ButtonSize, l_ButtonSize))) {
        l_Operation = ImGuizmo::ROTATE;
      }
      // ImGui::SameLine();
      if (ImGui::Button(ICON_LC_SCALE_3D,
                        ImVec2(l_ButtonSize, l_ButtonSize))) {
        l_Operation = ImGuizmo::SCALE;
      }

      ImGui::Dummy(ImVec2(0.0f, 20.0f));

      if (ImGui::Button(ICON_LC_MAGNET,
                        ImVec2(l_ButtonSize, l_ButtonSize))) {
        ImGui::OpenPopup("snap_settings_popup");
      }

      ImGui::PopFont();

      if (ImGui::IsPopupOpen("snap_settings_popup")) {
        ImGui::SetNextWindowSize(ImVec2(300, 200));
      }

      if (ImGui::BeginPopup("snap_settings_popup")) {
        ImGui::Text("Snap Settings");
        bool l_SnapTranslation =
            get_user_setting(N(snap_translation));
        bool l_SnapRotation = get_user_setting(N(snap_rotation));
        bool l_SnapScale = get_user_setting(N(snap_scale));

        Math::Vector3 l_SnapTranslationAmount =
            get_user_setting(N(snap_translation_amount));
        Math::Vector3 l_SnapRotationAmount =
            get_user_setting(N(snap_rotation_amount));
        Math::Vector3 l_SnapScaleAmount =
            get_user_setting(N(snap_scale_amount));

        ImGui::PushID("translation_snap");
        if (Base::BoolEdit("Translation", &l_SnapTranslation)) {
          set_user_setting(N(snap_translation), l_SnapTranslation);
        }
        if (Base::Vector3Edit("Amount", &l_SnapTranslationAmount)) {
          set_user_setting(N(snap_translation_amount),
                           l_SnapTranslationAmount);
        }
        ImGui::PopID();

        ImGui::PushID("rotation_snap");
        if (Base::BoolEdit("Rotation", &l_SnapRotation)) {
          set_user_setting(N(snap_rotation), l_SnapRotation);
        }
        if (Base::Vector3Edit("Amount", &l_SnapRotationAmount)) {
          set_user_setting(N(snap_rotation_amount),
                           l_SnapRotationAmount);
        }
        ImGui::PopID();

        ImGui::PushID("scale_snap");
        if (Base::BoolEdit("Scale", &l_SnapScale)) {
          set_user_setting(N(snap_scale), l_SnapScale);
        }
        if (Base::Vector3Edit("Amount", &l_SnapScaleAmount)) {
          set_user_setting(N(snap_scale_amount), l_SnapScaleAmount);
        }
        ImGui::PopID();

        ImGui::EndPopup();
      }

      ImGui::EndChild();

      ImGuizmo::SetDrawlist();
      ImGuizmo::SetOrthographic(false);
      float windowWidth = (float)ImGui::GetWindowWidth();
      float windowHeight = (float)ImGui::GetWindowHeight();
      ImGuizmo::SetRect(ImGui::GetWindowPos().x,
                        ImGui::GetWindowPos().y, windowWidth,
                        windowHeight);

      // TODO: Get near/far plane
      Math::Matrix4x4 l_ProjectionMatrix = glm::perspective(
          glm::radians(l_RenderView.get_camera_fov()),
          ((float)l_RenderView.get_dimensions().x) /
              ((float)l_RenderView.get_dimensions().y),
          0.1f, 100.0f);

      Math::Matrix4x4 l_ViewMatrix =
          glm::lookAt(l_RenderView.get_camera_position(),
                      l_RenderView.get_camera_position() +
                          l_RenderView.get_camera_direction(),
                      LOW_VECTOR3_UP);

      Core::Entity l_Entity = get_selected_entity();

      if (l_Entity.is_alive()) {
        Core::Component::Transform l_Transform =
            l_Entity.get_transform();

        Math::Matrix4x4 l_TransformMatrix =
            l_Transform.get_world_matrix();
        // glm::translate(glm::mat4(1.0f),
        // l_Transform.get_world_position()) *
        // glm::toMat4(l_Transform.get_world_rotation()) *
        // glm::scale(glm::mat4(1.0f), l_Transform.get_world_scale());

        Math::Vector3 l_Snap(0.0f);

        if (l_Operation == ImGuizmo::TRANSLATE) {
          bool l_ShouldSnap = get_user_setting(N(snap_translation));
          if (l_ShouldSnap) {
            l_Snap = get_user_setting(N(snap_translation_amount));
          }
        }
        if (l_Operation == ImGuizmo::ROTATE) {
          bool l_ShouldSnap = get_user_setting(N(snap_rotation));
          if (l_ShouldSnap) {
            l_Snap = get_user_setting(N(snap_rotation_amount));
          }
        }
        if (l_Operation == ImGuizmo::SCALE) {
          bool l_ShouldSnap = get_user_setting(N(snap_scale));
          if (l_ShouldSnap) {
            l_Snap = get_user_setting(N(snap_scale_amount));
          }
        }

        if (ImGuizmo::Manipulate(
                (float *)&l_ViewMatrix, (float *)&l_ProjectionMatrix,
                l_Operation, ImGuizmo::LOCAL,
                (float *)&l_TransformMatrix, NULL, &l_Snap.x)) {

          bool l_IsFirst = false;
          if (!get_gizmos_dragged()) {
            l_IsFirst = true;
          }

          Core::Component::Transform l_Parent =
              l_Transform.get_parent();
          if (l_Parent.is_alive()) {
            l_TransformMatrix =
                glm::inverse(l_Parent.get_world_matrix()) *
                l_TransformMatrix;
          }

          glm::vec3 scale;
          glm::quat rotation;
          glm::vec3 translation;
          glm::vec3 skew;
          glm::vec4 perspective;
          glm::decompose(l_TransformMatrix, scale, rotation,
                         translation, skew, perspective);

          Util::StoredHandle l_Before;
          Util::StoredHandle l_After;

          Util::DiffUtil::store_handle(l_Before, l_Transform);

          l_Transform.position(translation);
          l_Transform.rotation(rotation);
          l_Transform.scale(scale);

          Util::DiffUtil::store_handle(l_After, l_Transform);

          Transaction l_Transaction =
              Transaction::from_diff(l_Transform, l_Before, l_After);

          if (!l_Transaction.empty()) {
            set_gizmos_dragged(true);
          }

          if (!l_IsFirst && !l_Transaction.empty()) {
            Transaction l_OldTransaction =
                get_global_changelist().peek();

            for (uint32_t i = 0;
                 i < l_Transaction.get_operations().size(); ++i) {
              CommonOperations::PropertyEditOperation *i_Operation =
                  (CommonOperations::PropertyEditOperation *)
                      l_Transaction.get_operations()[i];
              for (uint32_t j = 0;
                   j < l_OldTransaction.get_operations().size();
                   ++j) {
                CommonOperations::PropertyEditOperation
                    *i_OldOperation =
                        (CommonOperations::PropertyEditOperation *)
                            l_OldTransaction.get_operations()[j];

                if (i_OldOperation->m_Handle ==
                        i_Operation->m_Handle &&
                    i_OldOperation->m_PropertyName ==
                        i_Operation->m_PropertyName) {
                  i_Operation->m_OldValue =
                      i_OldOperation->m_OldValue;
                }
              }
            }
            Transaction l_Old = get_global_changelist().pop();
            l_Old.cleanup();
          }
          get_global_changelist().add_entry(l_Transaction);

        } else {
          if (!ImGui::IsAnyItemActive()) {
            set_gizmos_dragged(false);
          }
        }
      }

      Core::Region l_Region = get_selected_handle().get_id();
      if (l_Region.is_alive()) {
        render_region_gizmos(p_Delta, p_RenderViewWidget, l_Region);
      }
    }

    EditingWidget::EditingWidget()
        : m_CameraSpeed(3.5f), m_SnapTranslation(false),
          m_SnapRotation(false), m_SnapScale(false),
          m_SnapTranslationAmount(0.0f), m_SnapRotationAmount(0.0f),
          m_SnapScaleAmount(0.0f)
    {
      m_RenderViewWidget = new RenderViewWidget(
          ICON_LC_VIEW " Viewport", Renderer::get_editor_renderview(),
          &render_gizmos);
      m_LastPitchYaw = Math::Vector2(0.0f, -90.0f);
    }

    void EditingWidget::set_camera_rotation(const float p_Pitch,
                                            const float p_Yaw)
    {
      Math::Quaternion l_CameraDirection =
          m_RenderViewWidget->get_renderview().get_camera_direction();

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

      m_RenderViewWidget->get_renderview().set_camera_direction(
          l_Forward);

      m_LastPitchYaw.x = l_Pitch;
      m_LastPitchYaw.y = l_Yaw;
    }

    bool EditingWidget::camera_movement(float p_Delta)
    {
      bool l_ReturnValue = false;

      if (!m_RenderViewWidget->is_focused()) {
        return false;
      }

      if (ImGui::GetIO().KeyCtrl) {
        return false;
      }

      Math::Vector3 l_CameraPosition =
          m_RenderViewWidget->get_renderview().get_camera_position();
      Math::Vector3 l_CameraDirection =
          m_RenderViewWidget->get_renderview().get_camera_direction();

      Math::Vector3 l_CameraFront = l_CameraDirection * -1.0f;

      Math::Vector3 l_CameraRight =
          glm::normalize(glm::cross(LOW_VECTOR3_UP, l_CameraFront)) *
          -1.0f;
      Math::Vector3 l_CameraUp =
          glm::cross(l_CameraFront, l_CameraRight) * -1.0f;

      if (!ImGui::IsAnyItemActive()) {
        if (ImGui::IsKeyDown(ImGuiKey_W)) {
          l_CameraPosition -=
              (l_CameraFront * p_Delta * m_CameraSpeed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
          l_CameraPosition +=
              (l_CameraFront * p_Delta * m_CameraSpeed);
        }

        if (ImGui::IsKeyDown(ImGuiKey_A)) {
          l_CameraPosition +=
              (l_CameraRight * p_Delta * m_CameraSpeed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
          l_CameraPosition -=
              (l_CameraRight * p_Delta * m_CameraSpeed);
        }

        if (ImGui::IsKeyDown(ImGuiKey_Q)) {
          l_CameraPosition += (l_CameraUp * p_Delta * m_CameraSpeed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_E)) {
          l_CameraPosition -= (l_CameraUp * p_Delta * m_CameraSpeed);
        }
      }

      float m_Sensitivity = 5000.0f * m_CameraSpeed;

      Math::Vector2 l_MousePositionDifference =
          m_LastMousePosition -
          m_RenderViewWidget->get_relative_hover_position();

      l_MousePositionDifference *= m_Sensitivity * p_Delta;

      if (m_LastMousePosition.x < 1.5f &&
          m_LastMousePosition.y < 1.5f &&
          ImGui::IsMouseDown(ImGuiMouseButton_Right)) {

        set_camera_rotation(l_MousePositionDifference.y,
                            l_MousePositionDifference.x);

        l_ReturnValue = true;
      }

      m_RenderViewWidget->get_renderview().set_camera_position(
          l_CameraPosition);

      return l_ReturnValue;
    }

    void EditingWidget::render_editing(float p_Delta)
    {
      ImGuiIO &io = ImGui::GetIO();

      bool l_AllowRendererBasedPicking =
          m_RenderViewWidget->is_hovered();

      if (m_RenderViewWidget->is_hovered()) {
        l_AllowRendererBasedPicking =
            l_AllowRendererBasedPicking && !camera_movement(p_Delta);
      }

      if (io.MouseWheel > 0.05f) {
        m_CameraSpeed += 0.25f;
      } else if (io.MouseWheel < -0.05f) {
        m_CameraSpeed -= 0.25f;
      }

      m_CameraSpeed = Math::Util::clamp(m_CameraSpeed, 0.1f, 15.0f);

      m_LastMousePosition =
          m_RenderViewWidget->get_relative_hover_position();

      if (ImGuizmo::IsOver()) {
        l_AllowRendererBasedPicking = false;
      }
      if (ImGui::IsAnyItemHovered()) {
        l_AllowRendererBasedPicking = false;
      }

      Math::Vector2 l_HoverCoordinates = {2.0f, 2.0f};

      if (l_AllowRendererBasedPicking) {
        l_HoverCoordinates =
            m_RenderViewWidget->get_relative_hover_position();

        uint32_t l_EntityIndex = 0;
        // TODO: Hover readback

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {

          Core::Entity l_Entity;
          if (l_EntityIndex != ~0u) {
            l_Entity = Core::Entity::find_by_index(l_EntityIndex);
          }

          set_selected_entity(l_Entity);
        }
      }
    }

    void EditingWidget::render(float p_Delta)
    {
      m_RenderViewWidget->render(p_Delta);

      Util::Globals::set(N(LOW_GAME_DIMENSIONS),
                         m_RenderViewWidget->get_widget_dimensions());

      if (Core::get_engine_state() == Util::EngineState::EDITING) {
        render_editing(p_Delta);
      }
    }
  } // namespace Editor
} // namespace Low
