#include "LowEditorUiWidget.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"

#include "LowUtilLogger.h"

#include "LowCoreUiView.h"
#include "LowCoreUiElement.h"

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

    static bool render_element(Core::UI::Element p_Element)
    {
      if (ImGui::Selectable(p_Element.get_name().c_str(),
                            p_Element.get_id() ==
                                get_selected_handle().get_id())) {
        set_selected_handle(p_Element);
        HandlePropertiesSection l_Section(p_Element, true);
        l_Section.render_footer = nullptr;
        get_details_widget()->add_section(l_Section);
      }

      return false;
    }

    static bool render_view_hierarchy(Core::UI::View p_View,
                                      bool *p_OpenedEntryPopup,
                                      float p_Delta)
    {
      bool l_Break = false;
      if (!p_View.get_elements().empty()) {
        Util::String l_IdString = "##";
        l_IdString += p_View.get_id();
        bool l_Open = ImGui::TreeNode(l_IdString.c_str());
        ImGui::SameLine();
        l_Break = render_view(p_View);

        if (l_Open) {
          for (auto it = p_View.get_elements().begin();
               it != p_View.get_elements().end(); ++it) {
            Core::UI::Element i_Element =
                Util::find_handle_by_unique_id(*it).get_id();

            if (!i_Element.is_alive()) {
              continue;
            }

            render_element(i_Element);
          }
          ImGui::TreePop();
        }
      } else {
        l_Break = render_view(p_View);
      }

      return l_Break;
    }

    void UiWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_WINDOW_RESTORE " UI-Views");

      bool l_Test = false;

      for (auto it : Core::UI::View::ms_LivingInstances) {
        render_view_hierarchy(it, &l_Test, p_Delta);
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
