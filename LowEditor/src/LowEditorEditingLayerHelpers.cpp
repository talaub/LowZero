#include "LowEditorEditingLayerHelpers.h"

#include "LowCoreDebugGeometry.h"
#include "LowCoreTween.h"
#include "LowCoreTweenEase.h"
#include "LowCoreTweenSystem.h"
#include "LowEditor.h"
#include "LowEditorThemes.h"
#include "LowEditorViewport.h"

#include "LowCoreAnimator.h"
#include "LowCoreEntity.h"
#include "LowCoreMeshRenderer.h"
#include "LowMath.h"
#include "LowRenderer.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderView.h"

#include "ImGuizmo.h"
#include "LowUtilContainers.h"
#include "imgui.h"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/quaternion.hpp"

#define TRANSFORM_SPHERE_TWEEN_TIME 0.3f
#define TRANSFORM_SPHERE_STATE_TWEEN_TIME 0.14f
#define TRANSFORM_SPHERE_STATE_PULSE_TWEEN_TIME 0.22f
#define TRANSFORM_SPHERE_STATE_PULSE_STRENGTH 0.22f
#define TRANSFORM_SPHERE_RADIUS 0.1f

namespace Low {
  namespace Editor {
    void TransformGizmoConfig::set_viewport(
        const Viewport &p_Viewport,
        TransformGizmoViewportMatrices p_Matrices)
    {
      viewport = &p_Viewport;
      viewport_matrices = p_Matrices;
      orthographic =
          p_Matrices == TransformGizmoViewportMatrices::Ui2D;
    }

    static ImGuizmo::OPERATION
    to_imguizmo_operation(TransformGizmoOperation p_Operation)
    {
      switch (p_Operation) {
      case TransformGizmoOperation::Translate:
        return ImGuizmo::TRANSLATE;
      case TransformGizmoOperation::Rotate:
        return ImGuizmo::ROTATE;
      case TransformGizmoOperation::Scale:
        return ImGuizmo::SCALE;
      }

      return ImGuizmo::TRANSLATE;
    }

    static Math::Matrix4x4
    build_transform_3d_matrix(const Transform3D &p_Transform)
    {
      if (p_Transform.has_world_matrix) {
        return p_Transform.world_matrix;
      }

      Math::Matrix4x4 l_Matrix =
          glm::translate(Math::Matrix4x4(1.0f),
                         p_Transform.position) *
          glm::toMat4(p_Transform.rotation) *
          glm::scale(Math::Matrix4x4(1.0f), p_Transform.scale);

      if (p_Transform.has_parent) {
        l_Matrix = p_Transform.parent_matrix * l_Matrix;
      }

      return l_Matrix;
    }

    static Math::Matrix4x4
    build_transform_2d_matrix(const Transform2D &p_Transform)
    {
      if (p_Transform.has_world_matrix) {
        return p_Transform.world_matrix;
      }

      Math::Matrix4x4 l_Matrix =
          glm::translate(Math::Matrix4x4(1.0f),
                         Math::Vector3(p_Transform.position.x,
                                       p_Transform.position.y, 0.0f));
      l_Matrix *= glm::toMat4(glm::angleAxis(
          p_Transform.rotation, Math::Vector3(0.0f, 0.0f, 1.0f)));
      l_Matrix = glm::scale(l_Matrix,
                            Math::Vector3(p_Transform.scale.x,
                                          p_Transform.scale.y, 1.0f));

      if (p_Transform.has_parent) {
        l_Matrix = p_Transform.parent_matrix * l_Matrix;
      }

      return l_Matrix;
    }

    static Math::Matrix4x4 calculate_gizmo_matrix(
        const Util::List<Math::Matrix4x4> &p_Matrices)
    {
      if (p_Matrices.size() == 1) {
        return p_Matrices[0];
      }

      Math::Vector3 l_Min = Math::Vector3(p_Matrices[0][3]);
      Math::Vector3 l_Max = l_Min;

      for (uint32_t i = 1; i < p_Matrices.size(); ++i) {
        Math::Vector3 i_Position = Math::Vector3(p_Matrices[i][3]);
        l_Min = glm::min(l_Min, i_Position);
        l_Max = glm::max(l_Max, i_Position);
      }

      return glm::translate(Math::Matrix4x4(1.0f),
                            (l_Min + l_Max) * 0.5f);
    }

    static bool
    get_viewport_gizmo_matrices(const TransformGizmoConfig &p_Config,
                                Math::Matrix4x4 &p_ViewMatrix,
                                Math::Matrix4x4 &p_ProjectionMatrix)
    {
      if (!p_Config.viewport) {
        return false;
      }

      Renderer::RenderView l_RenderView =
          p_Config.viewport->get_render_view();
      if (!l_RenderView.is_alive()) {
        return false;
      }

      if (p_Config.viewport_matrices ==
          TransformGizmoViewportMatrices::Ui2D) {
        p_ViewMatrix = l_RenderView.get_ui_view_matrix();
        p_ProjectionMatrix = l_RenderView.get_ui_projection_matrix();
        return true;
      }

      const Math::UVector2 l_Dimensions =
          l_RenderView.get_dimensions();
      if (l_Dimensions.x == 0u || l_Dimensions.y == 0u) {
        return false;
      }

      p_ProjectionMatrix = glm::perspective(
          glm::radians(l_RenderView.get_camera_fov()),
          ((float)l_Dimensions.x) / ((float)l_Dimensions.y), 0.1f,
          100.0f);

      p_ViewMatrix =
          glm::lookAt(l_RenderView.get_camera_position(),
                      l_RenderView.get_camera_position() +
                          l_RenderView.get_camera_direction(),
                      LOW_VECTOR3_UP);
      return true;
    }

    static TransformGizmoResult
    manipulate_matrices(TransformGizmoConfig p_Config,
                        Util::List<Math::Matrix4x4> &p_Matrices)
    {
      TransformGizmoResult l_Result;

      if (p_Matrices.empty()) {
        return l_Result;
      }

      if (!p_Config.viewport) {
        return l_Result;
      }

      Math::Matrix4x4 l_ViewMatrix(1.0f);
      Math::Matrix4x4 l_ProjectionMatrix(1.0f);
      if (!get_viewport_gizmo_matrices(p_Config, l_ViewMatrix,
                                       l_ProjectionMatrix)) {
        return l_Result;
      }

      const Math::Vector2 l_RectPosition =
          p_Config.viewport->get_widget_rect_position();
      const Math::Vector2 l_RectSize =
          p_Config.viewport->get_widget_rect_size();

      ImGuizmo::SetDrawlist();
      ImGuizmo::SetOrthographic(p_Config.orthographic);
      ImGuizmo::SetRect(l_RectPosition.x, l_RectPosition.y,
                        l_RectSize.x, l_RectSize.y);

      Math::Matrix4x4 l_GizmoMatrix =
          calculate_gizmo_matrix(p_Matrices);
      Math::Matrix4x4 l_OriginalGizmoMatrix = l_GizmoMatrix;

      float *l_Snap =
          p_Config.snap_enabled ? &p_Config.snap.x : nullptr;

      l_Result.changed = ImGuizmo::Manipulate(
          &l_ViewMatrix[0][0], &l_ProjectionMatrix[0][0],
          to_imguizmo_operation(p_Config.operation),
          p_Config.local ? ImGuizmo::LOCAL : ImGuizmo::WORLD,
          &l_GizmoMatrix[0][0], nullptr, l_Snap);

      if (!l_Result.changed) {
        return l_Result;
      }

      Math::Matrix4x4 l_GizmoDelta =
          l_GizmoMatrix * glm::inverse(l_OriginalGizmoMatrix);

      for (uint32_t i = 0; i < p_Matrices.size(); ++i) {
        p_Matrices[i] = l_GizmoDelta * p_Matrices[i];
      }

      return l_Result;
    }

    TransformGizmoResult
    render_transform_gizmo(TransformGizmoConfig p_Config,
                           Util::List<Transform3D> &p_Transforms)
    {
      Util::List<Math::Matrix4x4> l_WorldMatrices;
      for (Transform3D &i_Transform : p_Transforms) {
        l_WorldMatrices.push_back(
            build_transform_3d_matrix(i_Transform));
      }

      TransformGizmoResult l_Result =
          manipulate_matrices(p_Config, l_WorldMatrices);

      if (!l_Result.changed) {
        return l_Result;
      }

      for (uint32_t i = 0; i < p_Transforms.size(); ++i) {
        Math::Matrix4x4 l_LocalMatrix = l_WorldMatrices[i];
        if (p_Transforms[i].has_parent) {
          l_LocalMatrix =
              glm::inverse(p_Transforms[i].parent_matrix) *
              l_LocalMatrix;
        }

        Math::Vector3 l_Scale;
        Math::Quaternion l_Rotation;
        Math::Vector3 l_Translation;
        Math::Vector3 l_Skew;
        Math::Vector4 l_Perspective;
        glm::decompose(l_LocalMatrix, l_Scale, l_Rotation,
                       l_Translation, l_Skew, l_Perspective);

        p_Transforms[i].position = l_Translation;
        p_Transforms[i].rotation = l_Rotation;
        p_Transforms[i].scale = l_Scale;
        p_Transforms[i].has_world_matrix = false;
      }

      return l_Result;
    }

    TransformGizmoResult
    render_transform_gizmo(TransformGizmoConfig p_Config,
                           Util::List<Math::Vector3> &p_Positions)
    {
      Util::List<Transform3D> l_Transforms;
      for (Math::Vector3 &i_Position : p_Positions) {
        Transform3D i_Transform;
        i_Transform.position = i_Position;
        l_Transforms.push_back(i_Transform);
      }

      p_Config.operation = TransformGizmoOperation::Translate;
      TransformGizmoResult l_Result =
          render_transform_gizmo(p_Config, l_Transforms);

      if (!l_Result.changed) {
        return l_Result;
      }

      for (uint32_t i = 0; i < p_Positions.size(); ++i) {
        p_Positions[i] = l_Transforms[i].position;
      }

      return l_Result;
    }

    TransformGizmoResult render_transform_gizmo(
        TransformGizmoConfig p_Config,
        Util::List<Core::Component::Transform> &p_Transforms)
    {
      Util::List<Transform3D> l_Transforms;
      for (Core::Component::Transform &i_Transform : p_Transforms) {
        Transform3D i_HelperTransform;
        i_HelperTransform.position = i_Transform.position();
        i_HelperTransform.rotation = i_Transform.rotation();
        i_HelperTransform.scale = i_Transform.scale();
        i_HelperTransform.has_world_matrix = true;
        i_HelperTransform.world_matrix =
            i_Transform.get_world_matrix();

        Core::Component::Transform l_Parent =
            i_Transform.get_parent();
        if (l_Parent.is_alive()) {
          i_HelperTransform.has_parent = true;
          i_HelperTransform.parent_matrix =
              l_Parent.get_world_matrix();
        }

        l_Transforms.push_back(i_HelperTransform);
      }

      TransformGizmoResult l_Result =
          render_transform_gizmo(p_Config, l_Transforms);

      if (!l_Result.changed) {
        return l_Result;
      }

      for (uint32_t i = 0; i < p_Transforms.size(); ++i) {
        p_Transforms[i].position(l_Transforms[i].position);
        p_Transforms[i].rotation(l_Transforms[i].rotation);
        p_Transforms[i].scale(l_Transforms[i].scale);
      }

      return l_Result;
    }

    TransformGizmoResult
    render_transform_gizmo(TransformGizmoConfig p_Config,
                           Util::List<Transform2D> &p_Transforms)
    {
      Util::List<Math::Matrix4x4> l_WorldMatrices;
      for (Transform2D &i_Transform : p_Transforms) {
        l_WorldMatrices.push_back(
            build_transform_2d_matrix(i_Transform));
      }

      TransformGizmoResult l_Result =
          manipulate_matrices(p_Config, l_WorldMatrices);

      if (!l_Result.changed) {
        return l_Result;
      }

      for (uint32_t i = 0; i < p_Transforms.size(); ++i) {
        Math::Matrix4x4 l_LocalMatrix = l_WorldMatrices[i];
        if (p_Transforms[i].has_parent) {
          l_LocalMatrix =
              glm::inverse(p_Transforms[i].parent_matrix) *
              l_LocalMatrix;
        }

        Math::Vector3 l_Scale;
        Math::Quaternion l_Rotation;
        Math::Vector3 l_Translation;
        Math::Vector3 l_Skew;
        Math::Vector4 l_Perspective;
        glm::decompose(l_LocalMatrix, l_Scale, l_Rotation,
                       l_Translation, l_Skew, l_Perspective);

        p_Transforms[i].position =
            Math::Vector2(l_Translation.x, l_Translation.y);
        p_Transforms[i].rotation = glm::eulerAngles(l_Rotation).z;
        p_Transforms[i].scale = Math::Vector2(l_Scale.x, l_Scale.y);
        p_Transforms[i].has_world_matrix = false;
      }

      return l_Result;
    }

    TransformGizmoResult
    render_transform_gizmo(TransformGizmoConfig p_Config,
                           Util::List<Math::Vector2> &p_Positions)
    {
      Util::List<Transform2D> l_Transforms;
      for (Math::Vector2 &i_Position : p_Positions) {
        Transform2D i_Transform;
        i_Transform.position = i_Position;
        l_Transforms.push_back(i_Transform);
      }

      p_Config.operation = TransformGizmoOperation::Translate;
      TransformGizmoResult l_Result =
          render_transform_gizmo(p_Config, l_Transforms);

      if (!l_Result.changed) {
        return l_Result;
      }

      for (uint32_t i = 0; i < p_Positions.size(); ++i) {
        p_Positions[i] = l_Transforms[i].position;
      }

      return l_Result;
    }

    TransformGizmoResult render_transform_gizmo(
        TransformGizmoConfig p_Config,
        Util::List<Core::UI::Component::Display> &p_Displays)
    {
      Util::List<Transform2D> l_Transforms;
      for (Core::UI::Component::Display &i_Display : p_Displays) {
        Transform2D i_HelperTransform;
        i_HelperTransform.position =
            i_Display.get_absolute_pixel_position();
        i_HelperTransform.rotation =
            i_Display.get_absolute_rotation();
        i_HelperTransform.scale =
            i_Display.get_absolute_pixel_scale();
        l_Transforms.push_back(i_HelperTransform);
      }

      TransformGizmoResult l_Result =
          render_transform_gizmo(p_Config, l_Transforms);

      if (!l_Result.changed) {
        return l_Result;
      }

      for (uint32_t i = 0; i < p_Displays.size(); ++i) {
        Math::Vector2 l_Position = l_Transforms[i].position;
        float l_Rotation = l_Transforms[i].rotation;

        Core::UI::Component::Display l_Parent =
            p_Displays[i].get_parent();
        if (l_Parent.is_alive()) {
          l_Position -= l_Parent.get_absolute_pixel_position();
          l_Rotation -= l_Parent.get_absolute_rotation();
        }

        p_Displays[i].pixel_position(l_Position);
        p_Displays[i].rotation(l_Rotation);
        p_Displays[i].pixel_scale(l_Transforms[i].scale);
      }

      return l_Result;
    }

    static void highlight_renderobject(
        Renderer::RenderObject p_RenderObject,
        const Renderer::HighlightType p_HighlightType)
    {
      if (!p_RenderObject.is_alive()) {
        return;
      }
      for (Renderer::DrawCommand i_DrawCommand :
           p_RenderObject.get_draw_commands()) {
        Renderer::HighlightDrawSolid i_HighlightDraw;
        i_HighlightDraw.drawCommand = i_DrawCommand;
        i_HighlightDraw.highlightType = p_HighlightType;
        Renderer::get_editor_renderview()
            .get_highlight_draws_solid()
            .push_back(i_HighlightDraw);
      }
    }

    static void highlight_skeletal_renderobject(
        Renderer::SkeletalRenderObject p_RenderObject,
        const Renderer::HighlightType p_HighlightType)
    {
      if (!p_RenderObject.is_alive()) {
        return;
      }
      for (Renderer::DrawCommand i_DrawCommand :
           p_RenderObject.get_draw_commands()) {
        Renderer::HighlightDrawSolid i_HighlightDraw;
        i_HighlightDraw.drawCommand = i_DrawCommand;
        i_HighlightDraw.highlightType = p_HighlightType;
        Renderer::get_editor_renderview()
            .get_highlight_draws_solid()
            .push_back(i_HighlightDraw);
      }
    }

    static void
    highlight_entity(Core::Entity p_Entity,
                     const Renderer::HighlightType p_HighlightType)
    {
      if (!p_Entity.is_alive()) {
        return;
      }
      Core::Component::MeshRenderer l_MeshRenderer =
          p_Entity.get_component(
              Core::Component::MeshRenderer::type_id());
      Core::Component::Animator l_Animator = p_Entity.get_component(
          Core::Component::Animator::type_id());
      if (l_MeshRenderer.is_alive()) {
        if (l_MeshRenderer.get_render_object().is_alive()) {
          highlight_renderobject(l_MeshRenderer.get_render_object(),
                                 p_HighlightType);
          return;
        }
      }
      if (l_Animator.is_alive()) {
        if (l_Animator.get_render_object().is_alive()) {
          highlight_skeletal_renderobject(
              l_Animator.get_render_object(), p_HighlightType);
          return;
        }
      }
    }

    static FlyingCameraSharedState g_FlyingCameraSharedState;

    FlyingCameraSharedState &get_flying_camera_shared_state()
    {
      return g_FlyingCameraSharedState;
    }

    void FlyingCameraControls::set_camera_rotation(
        Viewport &p_Viewport, const float p_PitchDelta,
        const float p_YawDelta)
    {
      FlyingCameraSharedState &l_State =
          get_flying_camera_shared_state();

      float l_Pitch = l_State.last_pitch_yaw.x;
      float l_Yaw = l_State.last_pitch_yaw.y;
      l_Pitch -= p_PitchDelta;
      l_Yaw -= p_YawDelta;

      const float l_PitchMin = -85.f;
      const float l_PitchMax = 85.f;
      l_Pitch = Math::Util::clamp(l_Pitch, l_PitchMin, l_PitchMax);

      const float l_YawRadian = glm::radians(l_Yaw);
      const float l_PitchRadian = glm::radians(l_Pitch);

      Math::Vector3 l_Forward = Math::Vector3(
          cos(l_PitchRadian) * cos(l_YawRadian), -sin(l_PitchRadian),
          cos(l_PitchRadian) * sin(l_YawRadian));

      p_Viewport.get_render_view().set_camera_direction(l_Forward);

      l_State.last_pitch_yaw.x = l_Pitch;
      l_State.last_pitch_yaw.y = l_Yaw;
    }

    void FlyingCameraControls::adjust_speed_from_mouse_wheel()
    {
      ImGuiIO &l_Io = ImGui::GetIO();

      if (l_Io.MouseWheel > 0.05f) {
        camera_speed += camera_speed_scroll_step;
      } else if (l_Io.MouseWheel < -0.05f) {
        camera_speed -= camera_speed_scroll_step;
      }

      camera_speed = Math::Util::clamp(camera_speed, min_camera_speed,
                                       max_camera_speed);
    }

    bool FlyingCameraControls::tick(Viewport &p_Viewport,
                                    const float p_Delta)
    {
      bool l_ConsumedMouse = false;
      FlyingCameraSharedState &l_State =
          get_flying_camera_shared_state();

      if (!p_Viewport.is_focused()) {
        l_State.last_mouse_position =
            p_Viewport.get_relative_hover_position();
        return false;
      }

      if (ImGui::GetIO().KeyCtrl) {
        l_State.last_mouse_position =
            p_Viewport.get_relative_hover_position();
        return false;
      }

      Math::Vector3 l_CameraPosition =
          p_Viewport.get_render_view().get_camera_position();
      Math::Vector3 l_CameraDirection =
          p_Viewport.get_render_view().get_camera_direction();

      Math::Vector3 l_CameraFront = l_CameraDirection * -1.0f;

      Math::Vector3 l_CameraRight =
          glm::normalize(glm::cross(LOW_VECTOR3_UP, l_CameraFront)) *
          -1.0f;
      Math::Vector3 l_CameraUp =
          glm::cross(l_CameraFront, l_CameraRight) * -1.0f;

      if (!ImGui::IsAnyItemActive()) {
        if (ImGui::IsKeyDown(ImGuiKey_W)) {
          l_CameraPosition -=
              (l_CameraFront * p_Delta * camera_speed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
          l_CameraPosition +=
              (l_CameraFront * p_Delta * camera_speed);
        }

        if (ImGui::IsKeyDown(ImGuiKey_A)) {
          l_CameraPosition +=
              (l_CameraRight * p_Delta * camera_speed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
          l_CameraPosition -=
              (l_CameraRight * p_Delta * camera_speed);
        }

        if (ImGui::IsKeyDown(ImGuiKey_Q)) {
          l_CameraPosition += (l_CameraUp * p_Delta * camera_speed);
        }
        if (ImGui::IsKeyDown(ImGuiKey_E)) {
          l_CameraPosition -= (l_CameraUp * p_Delta * camera_speed);
        }
      }

      Math::Vector2 l_MousePositionDifference =
          l_State.last_mouse_position -
          p_Viewport.get_relative_hover_position();

      l_MousePositionDifference *=
          mouse_sensitivity_scale * camera_speed * p_Delta;

      if (l_State.last_mouse_position.x < 1.5f &&
          l_State.last_mouse_position.y < 1.5f &&
          ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        set_camera_rotation(p_Viewport, l_MousePositionDifference.y,
                            l_MousePositionDifference.x);
        l_ConsumedMouse = true;
      }

      p_Viewport.get_render_view().set_camera_position(
          l_CameraPosition);

      l_State.last_mouse_position =
          p_Viewport.get_relative_hover_position();

      return l_ConsumedMouse;
    }

    u32 read_hovered_pick_id(Viewport &p_Viewport)
    {
      Math::Vector2 l_Relative =
          p_Viewport.get_relative_hover_position();
      Math::UVector2 l_Dimensions =
          p_Viewport.get_widget_dimensions();

      uint32_t l_PickId = LOW_UINT32_MAX;

      if (l_Dimensions.x > 0u && l_Dimensions.y > 0u) {
        Math::UVector2 l_Pixel;
        l_Pixel.x = Math::Util::clamp(
            (uint32_t)(l_Relative.x * (float)l_Dimensions.x), 0u,
            l_Dimensions.x - 1u);
        l_Pixel.y = Math::Util::clamp(
            (uint32_t)(l_Relative.y * (float)l_Dimensions.y), 0u,
            l_Dimensions.y - 1u);

        l_PickId =
            p_Viewport.get_render_view().read_object_id_px(l_Pixel);
      }

      return l_PickId;
    }

    Core::Entity
    read_hovered_entity_from_viewport(Viewport &p_Viewport)
    {

      u32 l_EntityIndex = read_hovered_pick_id(p_Viewport);

      Core::Entity l_Entity;
      if (l_EntityIndex != ~0u) {
        l_EntityIndex = LOW_MATH_MIN(
            l_EntityIndex, Core::Entity::get_capacity() - 1);
        l_Entity = Core::Entity::find_by_index(l_EntityIndex);
      }

      return l_Entity;
    }

    ObjectPickingResult
    select_by_clicking(Viewport &p_Viewport,
                       const ObjectPickingOptions &p_Options)
    {
      ObjectPickingResult l_Result;
      l_Result.allowed = p_Viewport.is_hovered();

      if (p_Options.block_when_imguizmo_hovered &&
          ImGuizmo::IsOver()) {
        l_Result.allowed = false;
      }
      if (p_Options.block_when_imgui_item_hovered &&
          ImGui::IsAnyItemHovered()) {
        l_Result.allowed = false;
      }

      if (!l_Result.allowed) {
        return l_Result;
      }

      l_Result.hovered_entity =
          read_hovered_entity_from_viewport(p_Viewport);

      if (p_Options.highlight_hovered_entity &&
          l_Result.hovered_entity.is_alive()) {
        highlight_entity(l_Result.hovered_entity,
                         Renderer::HighlightType::Hovered);
      }

      if (!p_Options.enable_selection ||
          !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return l_Result;
      }

      l_Result.clicked = true;

      ImGuiIO &l_Io = ImGui::GetIO();
      if (p_Options.enable_multi_selection && l_Io.KeyCtrl) {
        add_entity_selection(l_Result.hovered_entity);
      } else {
        set_selected_entity(l_Result.hovered_entity);
      }
      l_Result.selection_changed = true;

      return l_Result;
    }

    Util::List<Util::SharedPtr<TransformSphere>>
        TransformSphere::m_Active;

    TransformSphere::~TransformSphere()
    {
      if (m_VisibilityTween.is_alive()) {
        m_VisibilityTween.destroy();
      }
      if (m_StateTween.is_alive()) {
        m_StateTween.destroy();
      }
      if (m_StatePulseTween.is_alive()) {
        m_StatePulseTween.destroy();
      }
    }

    static Math::Color
    transform_sphere_color(TransformSphere::State p_State)
    {
      const Theme &l_Theme = theme_get_current();

      switch (p_State) {
      case TransformSphere::State::Normal:
        return l_Theme.debug;
      case TransformSphere::State::Hovered:
        return l_Theme.info;
      case TransformSphere::State::Selected:
        return l_Theme.warning;
      }

      return l_Theme.debug;
    }

    void TransformSphere::set_state(const State p_State)
    {
      if (m_State == p_State) {
        return;
      }

      m_PreviousState = m_State;
      m_State = p_State;

      if (m_StateTween.is_alive()) {
        m_StateTween.destroy();
      }

      m_StateTween =
          Core::Tween::start(TRANSFORM_SPHERE_STATE_TWEEN_TIME,
                             Core::TweenEase::OUTCUBIC);

      if (m_State != State::Normal) {
        if (m_StatePulseTween.is_alive()) {
          m_StatePulseTween.destroy();
        }

        m_StatePulseTween =
            Core::Tween::start(TRANSFORM_SPHERE_STATE_PULSE_TWEEN_TIME,
                               Core::TweenEase::OUTCUBIC);
      }
    }

    void TransformSphere::toggle_visibility()
    {
      if (m_Visible) {
        hide();
      } else {
        show();
      }
    }

    void TransformSphere::show()
    {
      if (m_Visible) {
        return;
      }
      set_visibility(true);
    }

    void TransformSphere::hide()
    {
      if (!m_Visible) {
        return;
      }
      set_visibility(false);
    }

    void TransformSphere::set_visibility(const bool p_Visible)
    {
      if (m_Destroyed) {
        return;
      }
      if (m_Visible == p_Visible) {
        return;
      }

      m_Visible = p_Visible;

      if (m_VisibilityTween.is_alive()) {
        m_VisibilityTween.destroy();
      }

      m_VisibilityTween =
          Core::Tween::start(TRANSFORM_SPHERE_TWEEN_TIME,
                             m_Visible ? Core::TweenEase::OUTBACK
                                       : Core::TweenEase::OUTCUBIC);
    }

    Util::WeakPtr<TransformSphere>
    TransformSphere::create(const Math::Vector3 p_Position,
                            Renderer::RenderView p_RenderView)
    {
      Util::SharedPtr<TransformSphere> l_Sphere(
          new TransformSphere(p_Position, p_RenderView));
      m_Active.push_back(l_Sphere);
      return Util::WeakPtr<TransformSphere>(l_Sphere);
    }

    void TransformSphere::tick()
    {
      Math::Sphere l_Sphere;
      l_Sphere.position = m_Position;

      if (m_Visible) {
        l_Sphere.radius = TRANSFORM_SPHERE_RADIUS;

        if (m_VisibilityTween.is_alive() &&
            !m_VisibilityTween.is_finished()) {
          l_Sphere.radius = Core::System::Tween::get_eased_progress(
                                m_VisibilityTween) *
                            l_Sphere.radius;
        }

      } else {
        l_Sphere.radius = TRANSFORM_SPHERE_RADIUS;
        if (m_VisibilityTween.is_alive() &&
            !m_VisibilityTween.is_finished()) {
          l_Sphere.radius =
              (1.0f - Core::System::Tween::get_eased_progress(
                          m_VisibilityTween)) *
              l_Sphere.radius;
        } else {
          l_Sphere.radius = 0.0f;
        }
      }

      if (m_VisibilityTween.is_alive() &&
          m_VisibilityTween.is_finished()) {
        m_VisibilityTween.destroy();
      }

      if (m_StateTween.is_alive() && m_StateTween.is_finished()) {
        m_StateTween.destroy();
      }

      if (m_StatePulseTween.is_alive()) {
        if (m_StatePulseTween.is_finished()) {
          m_StatePulseTween.destroy();
        } else {
          const float l_PulseProgress =
              Core::System::Tween::get_eased_progress(
                  m_StatePulseTween);
          const float l_Pulse =
              glm::sin(l_PulseProgress * glm::pi<float>()) *
              TRANSFORM_SPHERE_STATE_PULSE_STRENGTH;
          l_Sphere.radius *= 1.0f + l_Pulse;
        }
      }

      const bool l_Wireframe = false;
      const bool l_DepthTest = false;

      const Math::Color l_PreviousColor =
          transform_sphere_color(m_PreviousState);
      const Math::Color l_TargetColor =
          transform_sphere_color(m_State);
      const float l_StateProgress =
          Core::System::Tween::get_eased_progress(m_StateTween);
      const Math::Color l_Color =
          glm::mix(l_PreviousColor, l_TargetColor, l_StateProgress);

      Core::DebugGeometry::render_sphere(
          l_Sphere, l_Color, l_DepthTest, l_Wireframe, m_PickId);
    }

    void TransformSphere::tick_all()
    {
      for (auto it = m_Active.begin(); it != m_Active.end();) {

        if ((*it)->m_Destroyed) {
          if (!(*it)->m_VisibilityTween.is_alive() ||
              (*it)->m_VisibilityTween.is_finished()) {
            it = m_Active.erase(it);
            continue;
          }
        }

        (*it)->tick();
        ++it;
      }
    }
  } // namespace Editor
} // namespace Low
