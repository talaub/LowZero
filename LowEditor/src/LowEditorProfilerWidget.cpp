#include "LowEditorProfilerWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"
#include <string>

#include "LowCoreGameLoop.h"

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

      ImGui::PlotLines("", g_FpsBuffer, BUFFER_SIZE, 0, NULL, 0.0f, 100.0f,
                       ImVec2(0.0f, 40.0f));

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low

#undef BUFFER_SIZE
