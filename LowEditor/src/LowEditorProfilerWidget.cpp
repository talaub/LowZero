#include "LowEditorProfilerWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"
#include <string>

#include "LowCoreGameLoop.h"

#include "LowUtilGlobals.h"

#define BUFFER_SIZE 256

namespace Low {
  namespace Editor {
    float g_FpsBuffer[BUFFER_SIZE];

    void ProfilerWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_STOPWATCH " Profiler");

      ImGui::Text("FPS: ");
      ImGui::SameLine();
      ImGui::Text(std::to_string(Core::GameLoop::get_fps()).c_str());

      {
        for (uint32_t i = 0; i < BUFFER_SIZE - 1; ++i) {
          g_FpsBuffer[i] = g_FpsBuffer[i + 1];
        }

        g_FpsBuffer[BUFFER_SIZE - 1] = Core::GameLoop::get_fps();
      }

      ImGui::PlotLines("", g_FpsBuffer, BUFFER_SIZE, 0, NULL, 0.0f, 160.0f,
                       ImVec2(0.0f, 40.0f));

      int l_DrawCalls = (int)Util::Globals::get(N(LOW_RENDERER_DRAWCALLS));

      int l_ComputeDispatches =
          (int)Util::Globals::get(N(LOW_RENDERER_COMPUTEDISPATCH));

      ImGui::Text("Drawcalls: ");
      ImGui::SameLine();
      ImGui::Text(std::to_string(l_DrawCalls).c_str());

      ImGui::Text("Compute: ");
      ImGui::SameLine();
      ImGui::Text(std::to_string(l_ComputeDispatches).c_str());

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low

#undef BUFFER_SIZE
