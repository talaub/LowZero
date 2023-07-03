#include "LowEditorGui.h"

#include "LowUtilHandle.h"

#include "LowCoreEntity.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include <string.h>

#include <algorithm>
#include <nfd.h>

namespace Low {
  namespace Editor {
    namespace Gui {
      static void draw_single_letter_label(Util::String p_Label,
                                           ImColor p_Color)
      {
        using namespace ImGui;

        float l_CircleSize = 10.0f;

        ImDrawList *l_DrawList = GetWindowDrawList();
        ImVec2 l_Pos = GetCursorScreenPos();

        /*
              l_DrawList->AddCircleFilled({l_Pos.x + l_CircleSize / 2.0f,
                                           l_Pos.y + (l_CircleSize / 2.0f)
           + 5.0f}, l_CircleSize, p_Color);
        */

        l_DrawList->AddRectFilled(l_Pos, l_Pos + ImVec2(16, 23), p_Color);

        SetCursorScreenPos({l_Pos.x + 3.0f, l_Pos.y + 2.0f});
        Text(p_Label.c_str());
        SetCursorScreenPos({l_Pos.x + 15, l_Pos.y});
      }

      static bool draw_single_coefficient_editor(Util::String p_Label,
                                                 ImColor p_Color,
                                                 float *p_Value, float p_Width,
                                                 float p_Margin, bool p_Break)
      {
        using namespace ImGui;

        ImVec2 l_Pos = GetCursorScreenPos();
        draw_single_letter_label(p_Label, p_Color);

        bool l_Changed = false;

        Util::String l_Label = "##" + p_Label;
        PushItemWidth(p_Width - 16.0f);
        if (DragFloat(l_Label.c_str(), p_Value)) {
          l_Changed = true;
        }
        PopItemWidth();

        if (!p_Break) {
          SetCursorScreenPos({l_Pos.x + p_Width + p_Margin, l_Pos.y});
        }

        return l_Changed;
      }

      static bool draw_single_coefficient_editor(Util::String p_Label,
                                                 ImColor p_Color,
                                                 float *p_Value, float p_Width,
                                                 bool p_Break)
      {
        return draw_single_coefficient_editor(p_Label, p_Color, p_Value,
                                              p_Width, 0.0f, p_Break);
      }

      static bool draw_single_coefficient_editor(Util::String p_Label,
                                                 ImColor p_Color,
                                                 float *p_Value, float p_Width,
                                                 float p_Margin)
      {
        return draw_single_coefficient_editor(p_Label, p_Color, p_Value,
                                              p_Width, p_Margin, false);
      }

      static bool draw_single_coefficient_editor(Util::String p_Label,
                                                 ImColor p_Color,
                                                 float *p_Value, float p_Width)
      {
        return draw_single_coefficient_editor(p_Label, p_Color, p_Value,
                                              p_Width, true);
      }

      bool Vector3Edit(Math::Vector3 &p_Vector)
      {
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_Spacing = LOW_EDITOR_SPACING;
        float l_Width = (l_FullWidth - (l_Spacing * 2.0f)) / 3.0f;
        bool l_Changed = false;

        if (draw_single_coefficient_editor("X", IM_COL32(204, 42, 54, 255),
                                           &p_Vector.x, l_Width, l_Spacing)) {
          l_Changed = true;
        }
        if (draw_single_coefficient_editor("Y", IM_COL32(42, 204, 54, 255),
                                           &p_Vector.y, l_Width, l_Spacing)) {
          l_Changed = true;
        }
        if (draw_single_coefficient_editor("Z", IM_COL32(0, 160, 176, 255),
                                           &p_Vector.z, l_Width)) {
          l_Changed = true;
        }

        return l_Changed;
      }

      bool Vector2Edit(Math::Vector2 &p_Vector)
      {
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_Spacing = LOW_EDITOR_SPACING;
        float l_Width = (l_FullWidth - (l_Spacing * 2.0f)) / 2.0f;
        bool l_Changed = false;

        if (draw_single_coefficient_editor("X", IM_COL32(204, 42, 54, 255),
                                           &p_Vector.x, l_Width, l_Spacing)) {
          l_Changed = true;
        }
        if (draw_single_coefficient_editor("Y", IM_COL32(42, 204, 54, 255),
                                           &p_Vector.y, l_Width)) {
          l_Changed = true;
        }

        return l_Changed;
      }

      Util::String FileExplorer()
      {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

        if (result == NFD_OKAY) {
          Util::String l_Output = outPath;
          free(outPath);

          return l_Output;
        } else if (result == NFD_CANCEL) {
        } else {
          LOW_LOG_ERROR << "File explorer encountered an error: "
                        << NFD_GetError() << LOW_LOG_END;
        }

        return "";
      }

      bool spinner(const char *label, float radius, int thickness,
                   Math::Color p_Color)
      {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
          return false;

        ImGuiContext &g = *GImGui;
        const ImGuiStyle &style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius)*2, (radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
          return false;

        // Render
        window->DrawList->PathClear();

        int num_segments = 30;
        int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

        const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
        const float a_max =
            IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

        const ImVec2 centre =
            ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

        for (int i = 0; i < num_segments; i++) {
          const float a =
              a_min + ((float)i / (float)num_segments) * (a_max - a_min);
          window->DrawList->PathLineTo(
              ImVec2(centre.x + ImCos(a + g.Time * 8) * radius,
                     centre.y + ImSin(a + g.Time * 8) * radius));
        }

        window->DrawList->PathStroke(IM_COL32(p_Color.x * 255, p_Color.y * 255,
                                              p_Color.z * 255, p_Color.a * 255),
                                     false, thickness);
      }

      void drag_handle(Util::Handle p_Handle)
      {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
          uint64_t l_Id = p_Handle.get_id();
          ImGui::SetDragDropPayload("DG_HANDLE", &l_Id, sizeof(l_Id));
          Util::Name l_Name;
          Util::RTTI::TypeInfo &l_TypeInfo =
              Util::Handle::get_type_info(p_Handle.get_type());

          if (l_TypeInfo.component) {
            Core::Entity l_Entity =
                *(uint64_t *)l_TypeInfo.properties[N(entity)].get(p_Handle);
            l_Name = l_Entity.get_name();
          } else {
            l_Name =
                *(Util::Name *)l_TypeInfo.properties[N(name)].get(p_Handle);
          }

          ImGui::Text(l_Name.c_str());

          ImGui::EndDragDropSource();
        }
      }
    } // namespace Gui
  }   // namespace Editor
} // namespace Low
