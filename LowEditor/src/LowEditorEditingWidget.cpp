#include "LowEditorEditingWidget.h"

#include "LowCoreMeshRenderer.h"
#include "LowCoreTaskScheduler.h"
#include "LowEditor.h"
#include "LowEditorMainWindow.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorGui.h"
#include "LowEditorBase.h"
#include "LowEditorEditingLayerHelpers.h"
#include "LowEditorFlyingCameraEditingLayer.h"
#include "LowEditorThemes.h"
#include "LowEditorViewportUi.h"
#include "LowEditorViewportRendering.h"

#include "LowMath.h"
#include "LowRendererEditorImage.h"
#include "LowRendererPointLight.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderView.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
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
#include "LowCoreBoxCollider.h"
#include "LowCoreSphereCollider.h"
#include "LowCoreCharacterController.h"
#include "LowCoreNavmeshAgent.h"
#include "LowCoreCamera.h"
#include "LowCoreAnimator.h"
#include "LowCoreTween.h"
#include "LowCoreTweenSystem.h"

#include "LowUtilEnums.h"
#include "LowUtilGlobals.h"

#include "LowMathQuaternionUtil.h"
#include <stdint.h>

namespace Low {
  namespace Editor {
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

    static bool is_viewport_handle_selected(Util::Handle p_Handle)
    {
      LOW_SELECTION(l_SelectedHandles);
      for (Util::Handle i_SelectedHandle : l_SelectedHandles) {
        if (i_SelectedHandle == p_Handle) {
          return true;
        }
      }
      return false;
    }

    static void render_viewport_handles(float p_Delta,
                                        Viewport &p_Viewport)
    {
      Renderer::RenderView l_RenderView =
          p_Viewport.get_render_view();

      for (u32 i = 0; i < Core::Entity::living_count(); ++i) {
        Core::Entity i_Entity = Core::Entity::living_instances()[i];
        ViewportHandleRenderer::show(
            l_RenderView, i_Entity,
            is_viewport_handle_selected(i_Entity));
      }

      LOW_SELECTION(l_SelectedHandles);
      for (Util::Handle i_Handle : l_SelectedHandles) {
        if (i_Handle.get_type() == Core::Entity::type_id()) {
          continue;
        }
        if (!i_Handle.is_registered_type()) {
          continue;
        }

        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(i_Handle.get_type());
        if (!l_TypeInfo.is_alive(i_Handle)) {
          continue;
        }

        ViewportHandleRenderer::show(l_RenderView, i_Handle, true);
      }
    }

    struct MainViewportEditingLayer : public FlyingCameraEditingLayer
    {
      MainViewportEditingLayer()
          : FlyingCameraEditingLayer(false, true),
            m_ActiveTool(EditorTool::Select)
      {
        m_ToolbarState.active_item = (u32)m_ActiveTool;

        add_toolbar(
            "MainViewportPlayMode", ViewportToolbarAnchor::TOP_CENTER,
            ViewportToolbarSlideDirection::UP,
            [](ViewportToolbarContext &p_Context) {
              if (p_Context.playmode_button()) {
                if (Core::get_engine_state() ==
                    Util::EngineState::EDITING) {
                  set_selected_entity(0);
                  Core::begin_playmode();
                } else if (Core::get_engine_state() ==
                           Util::EngineState::PLAYING) {
                  Core::exit_playmode();
                }
              }

              return p_Context.get_button_toolbar_width(1u);
            },
            false);

        add_toolbar(
            "MainViewportTools", ViewportToolbarAnchor::LEFT_CENTER,
            ViewportToolbarSlideDirection::LEFT,
            [this](ViewportToolbarContext &p_Context) {
              Util::List<ViewportUi::ToolbarItem> l_Items{
                  {(u32)EditorTool::Select, "Select",
                   ICON_LC_MOUSE_POINTER, 0, "1", ImGuiKey_1},
                  {(u32)EditorTool::Move, "Move", ICON_LC_MOVE_3D, 0,
                   "2", ImGuiKey_2},
                  {(u32)EditorTool::Rotate, "Rotate",
                   ICON_LC_ROTATE_3D, 0, "3", ImGuiKey_3},
                  {(u32)EditorTool::Scale, "Scale", ICON_LC_SCALE_3D,
                   0, "4", ImGuiKey_4},
              };

              p_Context.active_vertical_toolbar(l_Items,
                                                m_ToolbarState);
              m_ActiveTool = (EditorTool)m_ToolbarState.active_item;

              return p_Context.get_button_toolbar_width(1u);
            });
      }

      void tick(const EditingLayerContext &p_Context) override;

    private:
      EditorTool m_ActiveTool;
      ViewportUi::ToolbarState m_ToolbarState;

      void render_selection(Viewport &p_Viewport, float p_Delta);
      void render_picking(Viewport &p_Viewport, float p_Delta);
    };

    void MainViewportEditingLayer::tick(
        const EditingLayerContext &p_Context)
    {
      Viewport &p_Viewport = p_Context.viewport;
      const float p_Delta = p_Context.delta;

      if (Core::get_engine_state() != Util::EngineState::EDITING) {
        return;
      }

      render_selection(p_Viewport, p_Delta);
      render_picking(p_Viewport, p_Delta);
    }

    void
    MainViewportEditingLayer::render_selection(Viewport &p_Viewport,
                                               float p_Delta)
    {
      LOW_SELECTION(l_SelectedHandles);

      for (Util::Handle i_Handle : l_SelectedHandles) {
        Core::Entity l_SelectedEntity = i_Handle;
        highlight_entity(l_SelectedEntity,
                         Renderer::HighlightType::Selected);
      }

      render_viewport_handles(p_Delta, p_Viewport);

      TransformGizmoOperation l_Operation =
          TransformGizmoOperation::Translate;
      switch (m_ActiveTool) {
      case EditorTool::Move:
        l_Operation = TransformGizmoOperation::Translate;
        break;
      case EditorTool::Rotate:
        l_Operation = TransformGizmoOperation::Rotate;
        break;
      case EditorTool::Scale:
        l_Operation = TransformGizmoOperation::Scale;
        break;
      case EditorTool::Select:
        break;
      }

      Util::List<Core::Component::Transform> l_SelectedTransforms;
      for (Util::Handle i_Handle : l_SelectedHandles) {
        Core::Entity i_Entity = i_Handle.get_id();
        if (!i_Entity.is_alive()) {
          continue;
        }

        Core::Component::Transform i_Transform =
            i_Entity.get_transform();
        if (i_Transform.is_alive()) {
          l_SelectedTransforms.push_back(i_Transform);
        }
      }

      Util::List<Core::Component::Transform> l_RootSelectedTransforms;
      for (Core::Component::Transform i_Transform :
           l_SelectedTransforms) {
        bool l_HasSelectedParent = false;
        Core::Component::Transform i_Parent =
            i_Transform.get_parent();
        while (i_Parent.is_alive()) {
          for (Core::Component::Transform i_SelectedTransform :
               l_SelectedTransforms) {
            if (i_Parent == i_SelectedTransform) {
              l_HasSelectedParent = true;
              break;
            }
          }

          if (l_HasSelectedParent) {
            break;
          }

          i_Parent = i_Parent.get_parent();
        }

        if (!l_HasSelectedParent) {
          l_RootSelectedTransforms.push_back(i_Transform);
        }
      }

      if (!l_SelectedTransforms.empty() &&
          m_ActiveTool != EditorTool::Select) {
        TransformGizmoConfig l_GizmoConfig;
        l_GizmoConfig.operation = l_Operation;
        l_GizmoConfig.local = true;
        l_GizmoConfig.set_viewport(
            p_Viewport, TransformGizmoViewportMatrices::Scene3D);

        if (l_Operation == TransformGizmoOperation::Translate) {
          bool l_ShouldSnap = get_user_setting(N(snap_translation));
          if (l_ShouldSnap) {
            l_GizmoConfig.snap =
                get_user_setting(N(snap_translation_amount));
            l_GizmoConfig.snap_enabled = true;
          }
        }
        if (l_Operation == TransformGizmoOperation::Rotate) {
          bool l_ShouldSnap = get_user_setting(N(snap_rotation));
          if (l_ShouldSnap) {
            l_GizmoConfig.snap =
                get_user_setting(N(snap_rotation_amount));
            l_GizmoConfig.snap_enabled = true;
          }
        }
        if (l_Operation == TransformGizmoOperation::Scale) {
          bool l_ShouldSnap = get_user_setting(N(snap_scale));
          if (l_ShouldSnap) {
            l_GizmoConfig.snap =
                get_user_setting(N(snap_scale_amount));
            l_GizmoConfig.snap_enabled = true;
          }
        }

        Util::List<Util::StoredHandle> l_Before;
        for (Core::Component::Transform i_Transform :
             l_RootSelectedTransforms) {
          Util::StoredHandle i_Before;
          Util::DiffUtil::store_handle(i_Before, i_Transform);
          l_Before.push_back(i_Before);
        }

        TransformGizmoResult l_GizmoResult = render_transform_gizmo(
            l_GizmoConfig, l_RootSelectedTransforms);

        if (l_GizmoResult.changed) {

          bool l_IsFirst = false;
          if (!get_gizmos_dragged()) {
            l_IsFirst = true;
          }

          Transaction l_Transaction(
              l_SelectedTransforms.size() == 1
                  ? "Transform entity"
                  : "Transform selected entities");

          for (uint32_t i = 0; i < l_RootSelectedTransforms.size();
               ++i) {
            Core::Component::Transform i_Transform =
                l_RootSelectedTransforms[i];

            Util::StoredHandle l_After;
            Util::DiffUtil::store_handle(l_After, i_Transform);

            Transaction l_TransformTransaction =
                Transaction::from_diff(i_Transform, l_Before[i],
                                       l_After);

            for (uint32_t j = 0;
                 j < l_TransformTransaction.get_operations().size();
                 ++j) {
              l_Transaction.add_operation(
                  l_TransformTransaction.get_operations()[j]);
            }
          }

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
    }

    void
    MainViewportEditingLayer::render_picking(Viewport &p_Viewport,
                                             float p_Delta)
    {
      bool l_AllowRendererBasedPicking = p_Viewport.is_hovered();

      if (p_Viewport.is_hovered()) {
        EditingLayerContext l_Context{p_Delta, p_Viewport};
        l_AllowRendererBasedPicking =
            l_AllowRendererBasedPicking &&
            !tick_camera_controls(l_Context);
      }

      if (l_AllowRendererBasedPicking) {
        select_by_clicking(p_Viewport);
      }
    }

    EditingWidget::EditingWidget()
        : m_SnapTranslation(false), m_SnapRotation(false),
          m_SnapScale(false), m_SnapTranslationAmount(0.0f),
          m_SnapRotationAmount(0.0f), m_SnapScaleAmount(0.0f)
    {
      m_Viewport = new Viewport(Renderer::get_editor_renderview());
      m_Viewport->get_editing_layers().push(
          Util::make_shared<MainViewportEditingLayer>());
    }

    EditingWidget::~EditingWidget()
    {
      delete m_Viewport;
    }

    void EditingWidget::render(float p_Delta)
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(0.0f, 0.0f));
      ImGui::Begin(ICON_LC_VIEW " Viewport", nullptr,
                   ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse);

      ImVec2 l_ViewportSize = ImGui::GetContentRegionAvail();
      Math::UVector2 l_Dimensions((uint32_t)l_ViewportSize.x,
                                  (uint32_t)l_ViewportSize.y);

      if (l_Dimensions.x > 0u && l_Dimensions.y > 0u) {
        m_Viewport->set_dimensions(l_Dimensions);
        m_Viewport->tick(p_Delta);
      }

      ImGui::End();
      ImGui::PopStyleVar();

      Util::Globals::set(N(LOW_GAME_DIMENSIONS),
                         m_Viewport->get_widget_dimensions());
    }
  } // namespace Editor
} // namespace Low
