#pragma once

#include "LowCoreTween.h"
#include "LowEditorApi.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreUiDisplay.h"
#include "LowMath.h"
#include "LowRendererRenderView.h"

namespace Low {
  namespace Editor {
    struct Viewport;

    enum class TransformGizmoOperation
    {
      Translate,
      Rotate,
      Scale
    };

    enum class TransformGizmoViewportMatrices
    {
      Scene3D,
      Ui2D
    };

    struct TransformGizmoConfig
    {
      TransformGizmoOperation operation =
          TransformGizmoOperation::Translate;
      const Viewport *viewport = nullptr;
      TransformGizmoViewportMatrices viewport_matrices =
          TransformGizmoViewportMatrices::Scene3D;
      bool local = true;
      bool orthographic = false;
      Math::Vector3 snap = Math::Vector3(0.0f);
      bool snap_enabled = false;

      void set_viewport(const Viewport &p_Viewport,
                        TransformGizmoViewportMatrices p_Matrices =
                            TransformGizmoViewportMatrices::Scene3D);
    };

    struct TransformGizmoResult
    {
      bool changed = false;
    };

    struct Transform3D
    {
      Math::Vector3 position = Math::Vector3(0.0f);
      Math::Quaternion rotation =
          Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
      Math::Vector3 scale = Math::Vector3(1.0f);
      bool has_parent = false;
      Math::Matrix4x4 parent_matrix = Math::Matrix4x4(1.0f);
      bool has_world_matrix = false;
      Math::Matrix4x4 world_matrix = Math::Matrix4x4(1.0f);
    };

    struct Transform2D
    {
      Math::Vector2 position = Math::Vector2(0.0f);
      float rotation = 0.0f;
      Math::Vector2 scale = Math::Vector2(1.0f);
      bool has_parent = false;
      Math::Matrix4x4 parent_matrix = Math::Matrix4x4(1.0f);
      bool has_world_matrix = false;
      Math::Matrix4x4 world_matrix = Math::Matrix4x4(1.0f);
    };

    LOW_EDITOR_API TransformGizmoResult
    render_transform_gizmo(TransformGizmoConfig p_Config,
                           Util::List<Transform3D> &p_Transforms);

    LOW_EDITOR_API TransformGizmoResult
    render_transform_gizmo(TransformGizmoConfig p_Config,
                           Util::List<Math::Vector3> &p_Positions);

    LOW_EDITOR_API TransformGizmoResult render_transform_gizmo(
        TransformGizmoConfig p_Config,
        Util::List<Core::Component::Transform> &p_Transforms);

    LOW_EDITOR_API TransformGizmoResult
    render_transform_gizmo(TransformGizmoConfig p_Config,
                           Util::List<Transform2D> &p_Transforms);

    LOW_EDITOR_API TransformGizmoResult
    render_transform_gizmo(TransformGizmoConfig p_Config,
                           Util::List<Math::Vector2> &p_Positions);

    LOW_EDITOR_API TransformGizmoResult render_transform_gizmo(
        TransformGizmoConfig p_Config,
        Util::List<Core::UI::Component::Display> &p_Displays);

    struct LOW_EDITOR_API FlyingCameraSharedState
    {
      Math::Vector2 last_mouse_position = {2.0f, 2.0f};
      Math::Vector2 last_pitch_yaw = {0.0f, -90.0f};
    };

    LOW_EDITOR_API FlyingCameraSharedState &
    get_flying_camera_shared_state();

    struct LOW_EDITOR_API FlyingCameraControls
    {
      float camera_speed = 3.5f;
      float min_camera_speed = 0.1f;
      float max_camera_speed = 15.0f;
      float camera_speed_scroll_step = 0.25f;
      float mouse_sensitivity_scale = 5000.0f;

      bool tick(Viewport &p_Viewport, float p_Delta);
      void adjust_speed_from_mouse_wheel();

    private:
      void set_camera_rotation(Viewport &p_Viewport,
                               float p_PitchDelta, float p_YawDelta);
    };

    struct LOW_EDITOR_API ObjectPickingOptions
    {
      bool enable_selection = true;
      bool enable_multi_selection = true;
      bool highlight_hovered_entity = true;
      bool block_when_imgui_item_hovered = true;
      bool block_when_imguizmo_hovered = true;
    };

    struct LOW_EDITOR_API ObjectPickingResult
    {
      Core::Entity hovered_entity;
      bool clicked = false;
      bool selection_changed = false;
      bool allowed = false;
    };

    LOW_EDITOR_API u32 read_hovered_pick_id(Viewport &p_Viewport);

    LOW_EDITOR_API Core::Entity
    read_hovered_entity_from_viewport(Viewport &p_Viewport);

    LOW_EDITOR_API ObjectPickingResult select_by_clicking(
        Viewport &p_Viewport, const ObjectPickingOptions &p_Options =
                                  ObjectPickingOptions());

    struct TransformSphere
    {
      enum class State
      {
        Normal,
        Hovered,
        Selected
      };

      ~TransformSphere();

      void toggle_visibility();
      void hide();
      void show();

      void set_state(const State p_State);

      void select()
      {
        set_state(State::Selected);
      }

      void deselect()
      {
        set_state(State::Normal);
      }

      void set_hovered(const bool p_Hovered)
      {
        if (m_State == State::Selected) {
          return;
        }
        set_state(p_Hovered ? State::Hovered : State::Normal);
      }

      void set_pick_id(const u32 p_PickId)
      {
        m_PickId = p_PickId;
      }

      Math::Vector3 get_position() const
      {
        return m_Position;
      }

      void set_position(const Math::Vector3 p_Position)
      {
        m_Position = p_Position;
      }

      void destroy()
      {
        hide();
        m_Destroyed = true;
      }

      static Util::WeakPtr<TransformSphere>
      create(const Math::Vector3 p_Position,
             Renderer::RenderView p_RenderView);

      static void tick_all();

    protected:
      void set_visibility(const bool p_Visible);

      Core::Tween m_VisibilityTween;
      Core::Tween m_StateTween;
      Core::Tween m_StatePulseTween;

      bool m_Visible;
      float m_VisibilityMultiplier = 0.0f;
      State m_State = State::Normal;
      State m_PreviousState = State::Normal;
      u32 m_PickId = LOW_UINT32_MAX;

      bool m_Destroyed = false;

      void tick();
      Renderer::RenderView m_RenderView;

      Math::Vector3 m_Position;
      TransformSphere(const Math::Vector3 p_Position,
                      Renderer::RenderView p_RenderView)
          : m_Position(p_Position), m_Visible(false),
            m_VisibilityMultiplier(0.0f),
            m_RenderView(p_RenderView), m_Destroyed(false)
      {
        show();
      }

      static Util::List<Util::SharedPtr<TransformSphere>> m_Active;
    };
  } // namespace Editor
} // namespace Low
