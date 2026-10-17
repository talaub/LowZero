#include "LowEditorEditWidget.h"

#include "LowCoreUiWidgetAsset.h"
#include "LowEditor.h"
#include "LowMath.h"
#include "LowUtilLogger.h"
#include <imgui.h>
#include "imgui_internal.h"
#include "IconsLucide.h"

#include "LowRendererTexture.h"

namespace Low {
  namespace Editor {
    // Area a maximized window fills: the monitor work area when the
    // window is floating as its own OS window, otherwise the work
    // area of the viewport it lives in
    static void get_maximize_area(ImGuiWindow *p_Window,
                                  ImVec2 &p_Position, ImVec2 &p_Size)
    {
      if (!p_Window || !p_Window->Viewport) {
        ImGuiViewport *l_MainViewport = ImGui::GetMainViewport();
        p_Position = l_MainViewport->WorkPos;
        p_Size = l_MainViewport->WorkSize;
        return;
      }

      if (p_Window->ViewportOwned) {
        const ImGuiPlatformMonitor *l_Monitor =
            ImGui::GetViewportPlatformMonitor(p_Window->Viewport);
        p_Position = l_Monitor->WorkPos;
        p_Size = l_Monitor->WorkSize;
        return;
      }

      p_Position = p_Window->Viewport->WorkPos;
      p_Size = p_Window->Viewport->WorkSize;
    }

    // Icon button styled like the window controls in the main
    // window's title bar
    static bool title_bar_button(ImGuiWindow *p_Window, const char *p_Id,
                                 const char *p_Icon, const ImRect &p_Rect,
                                 bool p_Close, ImDrawFlags p_Corners)
    {
      const ImGuiID l_Id = p_Window->GetID(p_Id);
      ImGui::ItemAdd(p_Rect, l_Id);
      bool l_Hovered = false;
      bool l_Held = false;
      const bool l_Pressed =
          ImGui::ButtonBehavior(p_Rect, l_Id, &l_Hovered, &l_Held);

      if (l_Hovered) {
        ImVec4 l_Color = p_Close ? ImVec4(0.75f, 0.12f, 0.12f, 1.0f)
                                 : ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
        if (l_Held) {
          l_Color = p_Close ? ImVec4(0.88f, 0.16f, 0.16f, 1.0f)
                            : ImVec4(0.28f, 0.28f, 0.28f, 1.0f);
        }
        p_Window->DrawList->AddRectFilled(
            p_Rect.Min, p_Rect.Max, ImGui::GetColorU32(l_Color),
            p_Window->WindowRounding, p_Corners);
      }

      const ImVec2 l_IconSize = ImGui::CalcTextSize(p_Icon);
      const ImVec2 l_Center = p_Rect.GetCenter();
      ImGui::RenderText(
          ImVec2(ImTrunc(l_Center.x - (l_IconSize.x * 0.5f)),
                 ImTrunc(l_Center.y - (l_IconSize.y * 0.5f))),
          p_Icon);

      return l_Pressed;
    }

    EditWidget::EditWidget(const Util::Handle p_Handle)
        : m_Handle(p_Handle),
          m_Metadata(get_type_metadata(p_Handle.get_type()))

    {
      m_Title = ((Util::Name *)m_Metadata.typeInfo.properties[N(name)]
                     .get_return(p_Handle))
                    ->c_str();
      m_Title += " (";
      m_Title += m_Metadata.name.c_str();
      m_Title += ")";

      m_Editor = TypeEditor::create(p_Handle);
    }

    EditWidget::~EditWidget()
    {
      delete m_Editor;
    }

    void EditWidget::render(float p_Delta)
    {
      ImGuiWindowFlags l_Flags = ImGuiWindowFlags_NoSavedSettings;
      ImGuiWindow *l_PreviousWindow =
          ImGui::FindWindowByName(m_Title.c_str());

      if (m_Maximized) {
        ImVec2 l_Position;
        ImVec2 l_Size;
        get_maximize_area(l_PreviousWindow, l_Position, l_Size);
        ImGui::SetNextWindowPos(l_Position, ImGuiCond_Always);
        ImGui::SetNextWindowSize(l_Size, ImGuiCond_Always);
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
        l_Flags |= ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoDocking;
      } else if (m_RestorePending) {
        ImGui::SetNextWindowPos(
            ImVec2(m_RestorePosition.x, m_RestorePosition.y),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2(m_RestoreSize.x, m_RestoreSize.y),
            ImGuiCond_Always);
        m_RestorePending = false;
      } else {
        const Math::UVector2 l_Dimensions =
            m_Editor->get_edit_widget_dimensions();
        ImGui::SetNextWindowSize(
            ImVec2(l_Dimensions.x, l_Dimensions.y),
            ImGuiCond_FirstUseEver);
      }

      // Floating windows get our own close button next to maximize.
      // Docked ones keep ImGui's, which lives on the dock tab
      const bool l_Docked =
          l_PreviousWindow && l_PreviousWindow->DockIsActive;
      ImGui::Begin(m_Title.c_str(), l_Docked ? &m_Open : nullptr,
                   l_Flags);
      render_title_bar_buttons();
      m_Editor->render(p_Delta);
      ImGui::End();
    }

    void EditWidget::render_title_bar_buttons()
    {
      ImGuiWindow *l_Window = ImGui::GetCurrentWindow();

      // Docked windows have a tab instead of a title bar
      if (l_Window->DockIsActive) {
        m_Maximized = false;
        return;
      }
      if (l_Window->Flags & ImGuiWindowFlags_NoTitleBar) {
        return;
      }

      ImGuiContext &g = *ImGui::GetCurrentContext();
      const ImGuiStyle &l_Style = g.Style;
      const ImRect l_TitleBar = l_Window->TitleBarRect();
      const float l_ButtonHeight = l_TitleBar.GetHeight();
      const float l_ButtonWidth = ImTrunc(l_ButtonHeight * 1.5f);

      // Leave room for ImGui's collapse button if it sits on the right
      float l_Right = l_TitleBar.Max.x;
      ImDrawFlags l_CloseCorners = ImDrawFlags_RoundCornersTopRight;
      if (!(l_Window->Flags & ImGuiWindowFlags_NoCollapse) &&
          l_Style.WindowMenuButtonPosition == ImGuiDir_Right) {
        l_Right -= l_Style.FramePadding.x + g.FontSize +
                   l_Style.ItemInnerSpacing.x;
        l_CloseCorners = ImDrawFlags_RoundCornersNone;
      }

      const ImRect l_CloseRect(
          ImVec2(l_Right - l_ButtonWidth, l_TitleBar.Min.y),
          ImVec2(l_Right, l_TitleBar.Max.y));
      const ImRect l_MaximizeRect(
          ImVec2(l_CloseRect.Min.x - l_ButtonWidth, l_TitleBar.Min.y),
          ImVec2(l_CloseRect.Min.x, l_TitleBar.Max.y));

      ImGui::PushClipRect(l_TitleBar.Min, l_TitleBar.Max, false);

      ImGuiWindow *l_HighlightWindow =
          g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
      const bool l_TitleActive =
          l_HighlightWindow &&
          l_Window->RootWindowForTitleBarHighlight ==
              l_HighlightWindow->RootWindowForTitleBarHighlight;
      const ImU32 l_TitleColor = ImGui::GetColorU32(
          l_Window->Collapsed ? ImGuiCol_TitleBgCollapsed
          : l_TitleActive     ? ImGuiCol_TitleBgActive
                              : ImGuiCol_TitleBg);
      // ImGui doesn't know about these buttons, so paint over whatever
      // part of a long title runs underneath them
      l_Window->DrawList->AddRectFilled(
          ImVec2(l_MaximizeRect.Min.x - l_Style.ItemInnerSpacing.x,
                 l_TitleBar.Min.y),
          l_CloseRect.Max, l_TitleColor, l_Window->WindowRounding,
          l_CloseCorners);

      ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
      const bool l_MaximizePressed = title_bar_button(
          l_Window, "#MAXIMIZE",
          m_Maximized ? ICON_LC_MINIMIZE_2 : ICON_LC_MAXIMIZE_2,
          l_MaximizeRect, false, ImDrawFlags_RoundCornersNone);
      const bool l_ClosePressed =
          title_bar_button(l_Window, "#CLOSE", ICON_LC_X, l_CloseRect,
                           true, l_CloseCorners);
      ImGui::PopItemFlag();

      ImGui::PopClipRect();

      if (l_ClosePressed) {
        m_Open = false;
      }

      if (l_MaximizePressed) {
        if (m_Maximized) {
          m_Maximized = false;
          m_RestorePending = true;
        } else {
          m_RestorePosition =
              Math::Vector2(l_Window->Pos.x, l_Window->Pos.y);
          m_RestoreSize = Math::Vector2(l_Window->SizeFull.x,
                                        l_Window->SizeFull.y);
          m_Maximized = true;
        }
      }
    }
  } // namespace Editor
} // namespace Low
