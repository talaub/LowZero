#include "LowEditorGui.h"

#include "imgui.h"
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
    } // namespace Gui
  }   // namespace Editor
} // namespace Low
