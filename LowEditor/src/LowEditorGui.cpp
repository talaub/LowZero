#include "LowEditorGui.h"

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

        l_DrawList->AddCircleFilled({l_Pos.x + l_CircleSize / 2.0f,
                                     l_Pos.y + (l_CircleSize / 2.0f) + 5.0f},
                                    l_CircleSize, p_Color);

        SetCursorScreenPos({l_Pos.x, l_Pos.y + 1.0f});
        Text(p_Label.c_str());
      }

      bool Vector3Edit(Math::Vector3 &p_Vector)
      {
        using namespace ImGui;

        draw_single_letter_label("X", IM_COL32(190, 0, 0, 255));
        SameLine();
        DragFloat("##x", &p_Vector.x);

        return true;
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
    } // namespace Gui
  }   // namespace Editor
} // namespace Low
