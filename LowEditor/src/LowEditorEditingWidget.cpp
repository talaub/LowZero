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

    // --- item model ----------------------------------------------
    struct ToolbarItem
    {
      EditorTool id;
      const char *label;        // tooltip / a11y text
      const char *icon_glyph;   // e.g. u8"\uf245" (Font Awesome);
                                // nullptr if using texture
      ImTextureID icon_tex = 0; // optional icon texture
      const char *shortcut = nullptr; // shown in tooltip
    };

    // --- style (tweak freely) ------------------------------------
    struct ViewportToolbarStyle
    {
      float dpi_scale = 1.0f; // if you have your own DPI logic, set
                              // it before calling
      float margin = 23.0f;   // distance from viewport content edge
      float pad_x = 8.0f;
      float pad_y = 8.0f;
      float item_gap = 12.0f; // vertical gap between icons
      float corner_radius = 10.0f;
      float icon_size = 23.0f; // glyph/em square or texture size
      float outline_thickness = 1.0f;
      bool center_vertically = true; // otherwise anchored to top
    };

    static void draw_frosted_toolbar_bg(
        ImDrawList *p_Draw, ImTextureID p_BlurTex,
        const ImVec2 &p_BarMin, const ImVec2 &p_BarMax,
        const ImVec2 &p_SceneRectMin, const ImVec2 &p_SceneRectMax,
        const ImVec2 &p_SceneUV0 = ImVec2(0, 0),
        const ImVec2 &p_SceneUV1 = ImVec2(1, 1),
        float p_Radius = 10.0f,
        ImU32 p_Tint = IM_COL32(255, 255, 255, 40), // milky veil
        ImU32 p_Border = IM_COL32(0, 0, 0, 64))     // subtle border
    {
      // Early out if no overlap with the viewport image rect.
      ImVec2 l_OverlapMin(ImMax(p_BarMin.x, p_SceneRectMin.x),
                          ImMax(p_BarMin.y, p_SceneRectMin.y));
      ImVec2 l_OverlapMax(ImMin(p_BarMax.x, p_SceneRectMax.x),
                          ImMin(p_BarMax.y, p_SceneRectMax.y));
      if (l_OverlapMax.x <= l_OverlapMin.x ||
          l_OverlapMax.y <= l_OverlapMin.y)
        return;

      // Compute UVs by mapping toolbar pixels into the viewport image
      // UV rectangle.
      ImVec2 l_SceneSize(p_SceneRectMax.x - p_SceneRectMin.x,
                         p_SceneRectMax.y - p_SceneRectMin.y);
      // Normalize overlap min/max to [0..1] in scene rect:
      ImVec2 l_NormMin(
          (l_OverlapMin.x - p_SceneRectMin.x) / l_SceneSize.x,
          (l_OverlapMin.y - p_SceneRectMin.y) / l_SceneSize.y);
      ImVec2 l_NormMax(
          (l_OverlapMax.x - p_SceneRectMin.x) / l_SceneSize.x,
          (l_OverlapMax.y - p_SceneRectMin.y) / l_SceneSize.y);

      // Lerp into the viewport image UVs (handles V-flip if
      // p_SceneUV0.y > p_SceneUV1.y).
      auto lerp2 = [](const ImVec2 &a, const ImVec2 &b,
                      const ImVec2 &t) {
        return ImVec2(a.x + (b.x - a.x) * t.x,
                      a.y + (b.y - a.y) * t.y);
      };
      ImVec2 l_UVMin = lerp2(p_SceneUV0, p_SceneUV1, l_NormMin);
      ImVec2 l_UVMax = lerp2(p_SceneUV0, p_SceneUV1, l_NormMax);

      // Draw blur slice inside the overlap region with rounded
      // corners.
#if IMGUI_VERSION_NUM >= 19080
      // AddImageRounded is available on recent ImGui versions.
      p_Draw->AddImageRounded(p_BlurTex, l_OverlapMin, l_OverlapMax,
                              l_UVMin, l_UVMax, IM_COL32_WHITE,
                              p_Radius, ImDrawFlags_RoundCornersAll);
#else
      // Fallback: use AddImage (no per-vertex rounding). You can keep
      // the clip rect tight and layer an AddRectFilled with rounding
      // as a soft mask look.
      p_Draw->AddImage(p_BlurTex, l_OverlapMin, l_OverlapMax, l_UVMin,
                       l_UVMax, IM_COL32_WHITE);
#endif

      // Frost tint on full bar (not just overlap) so edges stay soft
      // even if bar sticks outside scene.
      p_Draw->AddRectFilled(p_BarMin, p_BarMax, p_Tint, p_Radius,
                            ImDrawFlags_RoundCornersAll);

      // Border
      p_Draw->AddRect(p_BarMin, p_BarMax, p_Border, p_Radius,
                      ImDrawFlags_RoundCornersAll, 1.0f);

      // Optional: subtle inner shadow for depth (looks nice over busy
      // scenes). Path a rounded rect slightly inset and stroke with
      // low alpha.
      const float l_Inset = 1.0f;
      p_Draw->AddRect(
          ImVec2(p_BarMin.x + l_Inset, p_BarMin.y + l_Inset),
          ImVec2(p_BarMax.x - l_Inset, p_BarMax.y - l_Inset),
          IM_COL32(0, 0, 0, 25), p_Radius - l_Inset,
          ImDrawFlags_RoundCornersAll, 1.0f);
    }

    void render_viewport_with_vertical_toolbar(
        ImTextureID p_Background, const ImVec2 &p_SceneSize,
        const Util::List<ToolbarItem> &p_Items,
        EditorTool *p_ActiveTool,
        const ViewportToolbarStyle &p_StyleIn)
    {
      // 2) resolve style (apply DPI)
      ViewportToolbarStyle l_Style = p_StyleIn;
      if (l_Style.dpi_scale <= 0.0f)
        l_Style.dpi_scale = ImGui::GetIO().FontGlobalScale;
      const float l_S = l_Style.dpi_scale;

      const float l_Margin = l_Style.margin * l_S;
      const float l_PadX = l_Style.pad_x * l_S;
      const float l_PadY = l_Style.pad_y * l_S;
      const float l_Gap = l_Style.item_gap * l_S;
      const float l_Radius = l_Style.corner_radius * l_S;
      const float l_Icon = l_Style.icon_size * l_S;
      const float l_Outline = l_Style.outline_thickness * l_S;

      if (p_Items.empty())
        return;

      ImGuiWindow *l_Window = ImGui::GetCurrentWindow();
      if (!l_Window)
        return;

      const ImVec2 l_SceneRectMin = ImGui::GetItemRectMin();
      const ImVec2 l_SceneRectMax = ImGui::GetItemRectMax();

      // content rect in screen space
      const ImVec2 l_WinPos = l_Window->Pos;
      const ImVec2 l_CrMin = ImGui::GetWindowContentRegionMin();
      const ImVec2 l_CrMax = ImGui::GetWindowContentRegionMax();
      const ImVec2 l_ContentMin(l_WinPos.x + l_CrMin.x,
                                l_WinPos.y + l_CrMin.y);
      const ImVec2 l_ContentMax(l_WinPos.x + l_CrMax.x,
                                l_WinPos.y + l_CrMax.y);

      if (l_ContentMax.x <= l_ContentMin.x ||
          l_ContentMax.y <= l_ContentMin.y)
        return;

      ImDrawList *l_Draw = ImGui::GetWindowDrawList();

      // measure bar dimensions
      const float l_BarW = l_PadX * 2.0f + l_Icon;
      float l_BarH = l_PadY * 2.0f + l_Icon * p_Items.size() +
                     l_Gap * (float)(p_Items.size() - 1);

      // position: left side, centered vertically (or top-aligned)
      ImVec2 l_BarPos;
      l_BarPos.x = l_ContentMin.x + l_Margin;
      if (l_Style.center_vertically) {
        l_BarPos.y =
            l_ContentMin.y +
            ((l_ContentMax.y - l_ContentMin.y) - l_BarH) * 0.5f;
      } else {
        l_BarPos.y = l_ContentMin.y + l_Margin;
      }

      const ImVec2 l_BarMin = l_BarPos;
      const ImVec2 l_BarMax(l_BarPos.x + l_BarW, l_BarPos.y + l_BarH);

      // background & outline
      const ImU32 l_BgCol = ImGui::GetColorU32(ImGuiCol_WindowBg);
      const ImU32 l_BorderCol = ImGui::GetColorU32(ImGuiCol_Border);
      const ImU32 l_HovCol =
          ImGui::GetColorU32(ImGuiCol_ButtonHovered);
      const ImU32 l_ActCol =
          ImGui::GetColorU32(ImGuiCol_ButtonActive);
      const ImU32 l_TextCol = ImGui::GetColorU32(ImGuiCol_Text);

      if (p_Background) {
        draw_frosted_toolbar_bg(
            l_Draw, p_Background,
            /* p_BarMin */ l_BarMin,
            /* p_BarMax */ l_BarMax,
            /* scene rect */ l_SceneRectMin, l_SceneRectMax,
            /* scene UVs */ ImVec2(0, 0), ImVec2(1, 1),
            /* radius    */ l_Radius,
            /* tint      */ IM_COL32(255, 255, 255, 10),

            /* border    */ IM_COL32(0, 0, 0, 0));
      } else {
        l_Draw->AddRectFilled(l_BarMin, l_BarMax, l_BgCol, l_Radius,
                              ImDrawFlags_RoundCornersAll);
        if (l_Outline > 0.0f)
          l_Draw->AddRect(l_BarMin, l_BarMax, l_BorderCol, l_Radius,
                          ImDrawFlags_RoundCornersAll, l_Outline);
      }

      // clip toolbar
      ImGui::PushClipRect(l_BarMin, l_BarMax, true);

      ImVec2 l_Cursor(l_BarPos.x + l_PadX, l_BarPos.y + l_PadY);
      const ImVec2 l_ItemSize(l_Icon, l_Icon);

      ImGui::PushID("ViewportToolbarVertical");

      EditorTool &l_Active = *p_ActiveTool;

      for (const ToolbarItem &l_Item : p_Items) {
        ImGui::PushID((int)l_Item.id);

        // hit area
        ImGui::SetCursorScreenPos(l_Cursor);
        ImGui::InvisibleButton("btn", l_ItemSize);

        const bool l_Hovered =
            ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
        const bool l_Held = ImGui::IsItemActive();
        const bool l_Selected = (l_Active == l_Item.id);

        // hover / selected fill
        if (l_Hovered || l_Selected || l_Held) {
          const ImU32 l_Fill = l_Selected ? l_ActCol : l_HovCol;
          const ImVec2 l_Min = ImGui::GetItemRectMin();
          const ImVec2 l_Max = ImGui::GetItemRectMax();
          l_Draw->AddRectFilled(l_Min, l_Max, l_Fill, l_Radius * 0.6f,
                                ImDrawFlags_RoundCornersAll);
        }

        // icon (glyph preferred; texture also supported)
        const ImVec2 l_Center(l_Cursor.x + l_ItemSize.x * 0.5f,
                              l_Cursor.y + l_ItemSize.y * 0.5f);

        if (l_Item.icon_tex) {
          const ImVec2 l_Min(l_Center.x - l_Icon * 0.5f,
                             l_Center.y - l_Icon * 0.5f);
          const ImVec2 l_Max(l_Center.x + l_Icon * 0.5f,
                             l_Center.y + l_Icon * 0.5f);
          l_Draw->AddImage(l_Item.icon_tex, l_Min, l_Max,
                           ImVec2(0, 0), ImVec2(1, 1), l_TextCol);
        } else if (l_Item.icon_glyph) {
          // draw glyph at a target size using drawlist AddText(font,
          // size, ...)
          ImVec2 l_TextSize = ImGui::CalcTextSize(l_Item.icon_glyph);
          float l_FontSize = ImGui::GetFontSize();
          float l_Scale =
              (l_TextSize.y > 0.0f) ? (l_Icon / l_TextSize.y) : 1.0f;
          const ImVec2 l_SizeScaled(l_TextSize.x * l_Scale,
                                    l_TextSize.y * l_Scale);
          const ImVec2 l_TextPos(l_Center.x - l_SizeScaled.x * 0.5f,
                                 l_Center.y - l_SizeScaled.y * 0.5f);
          l_Draw->AddText(ImGui::GetFont(), l_FontSize * l_Scale,
                          l_TextPos, l_TextCol, l_Item.icon_glyph);
        }

        // tooltip
        if (l_Hovered && l_Item.label && *l_Item.label) {
          ImGui::BeginTooltip();
          if (l_Item.shortcut && *l_Item.shortcut)
            ImGui::Text("%s  (%s)", l_Item.label, l_Item.shortcut);
          else
            ImGui::TextUnformatted(l_Item.label);
          ImGui::EndTooltip();
        }

        // selection
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
          l_Active = l_Item.id;
        }

        // advance vertically
        l_Cursor.y += l_ItemSize.y + l_Gap;
        ImGui::PopID();
      }

      ImGui::PopID();
      ImGui::PopClipRect();

      // optional: hotkeys (only if viewport window focused)
      if (ImGui::IsWindowFocused(
              ImGuiFocusedFlags_RootAndChildWindows)) {
        // example bindings (QWER)
        if (ImGui::Shortcut(ImGuiKey_1))
          l_Active = EditorTool::Select;
        if (ImGui::Shortcut(ImGuiKey_2))
          l_Active = EditorTool::Move;
        if (ImGui::Shortcut(ImGuiKey_3))
          l_Active = EditorTool::Rotate;
        if (ImGui::Shortcut(ImGuiKey_4))
          l_Active = EditorTool::Scale;
      }
    }

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

      static EditorTool l_ActiveTool = EditorTool::Select;

      {
        ImVec2 l_Avail;
        l_Avail.x = p_RenderViewWidget.get_widget_dimensions().x;
        l_Avail.y = p_RenderViewWidget.get_widget_dimensions().y;

        Util::List<ToolbarItem> l_Items{
            {EditorTool::Select, "Select", ICON_LC_MOUSE_POINTER, 0,
             "1"},
            {EditorTool::Move, "Move", ICON_LC_MOVE_3D, 0, "2"},
            {EditorTool::Rotate, "Rotate", ICON_LC_ROTATE_3D, 0, "3"},
            {EditorTool::Scale, "Scale", ICON_LC_SCALE_3D, 0, "4"},
        };

        ViewportToolbarStyle l_Style;
        l_Style.center_vertically =
            true; // or false to pin to top-left
        l_Style.dpi_scale = ImGui::GetIO().FontGlobalScale;

        render_viewport_with_vertical_toolbar(
            p_RenderViewWidget.get_renderview()
                .get_blurred_image()
                .get_gpu()
                .get_imgui_texture_id(),
            l_Avail, l_Items, &l_ActiveTool, l_Style);
      }

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

      ImGuizmo::OPERATION l_Operation = ImGuizmo::OPERATION::BOUNDS;
      switch (l_ActiveTool) {
      case EditorTool::Move:
        l_Operation = ImGuizmo::TRANSLATE;
        break;
      case EditorTool::Rotate:
        l_Operation = ImGuizmo::ROTATE;
        break;
      case EditorTool::Scale:
        l_Operation = ImGuizmo::SCALE;
        break;
      }

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
