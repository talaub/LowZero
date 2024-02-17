#include "LowEditorChangeWidget.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"

#include "LowEditorThemes.h"
#include "LowEditorMainWindow.h"
#include "LowEditorGui.h"

#include "LowCoreEntity.h"
#include "LowCorePrefab.h"
#include "LowCoreTransform.h"

#include "LowUtilString.h"

#include "LowRendererImGuiHelper.h"

#include "LowMathQuaternionUtil.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Low {
  namespace Editor {
    static void render_transaction(Transaction &p_Transaction,
                                   bool p_IsCurrent)
    {
      ImGui::BeginGroup();
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().icon_800);
      if (p_IsCurrent) {
        ImGui::PushStyleColor(
            ImGuiCol_Text, color_to_imvec4(theme_get_current().info));
        ImGui::Text(ICON_FA_ARROW_RIGHT);
      } else {
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            color_to_imvec4(theme_get_current().subtext));
        ImGui::Text(ICON_FA_CLOCK);
      }
      ImGui::PopStyleColor();
      ImGui::PopFont();
      ImGui::SameLine();
      ImGui::BeginGroup();
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().common_500);
      ImGui::TextWrapped(p_Transaction.m_Title.c_str());
      ImGui::PopFont();

      /*
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(128, 128, 128,
      255)); ImGui::TextWrapped(get_module_label(p_Entry));
      ImGui::PopStyleColor();
      */

      ImGui::EndGroup();
      ImGui::EndGroup();
      ImGui::Separator();
      ImGui::Spacing();
    }

    void ChangeWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_CLOCK " History");

      for (int i = get_global_changelist().m_Changelist.size();
           i-- > 0;) {
        render_transaction(
            get_global_changelist().m_Changelist[i],
            i - 1 == get_global_changelist().m_ChangePointer);
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
