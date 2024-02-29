#include "LowEditorUiWidget.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"

#include "LowUtilLogger.h"

#include "LowCoreUiView.h"

#include "LowEditorMainWindow.h"
#include "LowEditorSaveHelper.h"
#include "LowEditorHandlePropertiesSection.h"
#include "LowEditorDetailsWidget.h"

namespace Low {
  namespace Editor {
    void render_view_details_footer(Util::Handle p_Handle,
                                    Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::UI::View l_Asset = p_Handle.get_id();

      if (l_Asset.is_loaded()) {
        if (ImGui::Button("Save")) {
          Util::String l_JobTitle = "Saving view ";
          l_JobTitle += l_Asset.get_name().c_str();
          register_editor_job(l_JobTitle, [l_Asset] {
            // SaveHelper::save_region(l_Asset.get_id());
          });
        }
        if (ImGui::Button("Unload")) {
          // l_Asset.unload_entities();
        }
      } else {
        if (ImGui::Button("Load")) {
          // l_Asset.load_entities();
        }
      }
    }

    static bool render_view(Core::UI::View p_View)
    {
      if (ImGui::Selectable(p_View.get_name().c_str(),
                            p_View.get_id() ==
                                get_selected_handle().get_id())) {
        set_selected_handle(p_View);
        HandlePropertiesSection l_Section(p_View, true);
        l_Section.render_footer = &render_view_details_footer;
        get_details_widget()->add_section(l_Section);
      }

      return false;
    }

    void UiWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_WINDOW_RESTORE " UI-Views");

      for (auto it : Core::UI::View::ms_LivingInstances) {
        render_view(it);
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
