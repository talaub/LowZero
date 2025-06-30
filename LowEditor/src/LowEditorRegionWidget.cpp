#include "LowEditorRegionWidget.h"

#include "imgui.h"
#include "IconsLucide.h"

#include "LowEditor.h"
#include "LowEditorMainWindow.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorHandlePropertiesSection.h"
#include "LowEditorSaveHelper.h"
#include "LowEditorGui.h"

#include "LowCoreRegion.h"

namespace Low {
  namespace Editor {
    void
    render_region_details_footer(Util::Handle p_Handle,
                                 Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::Region l_Asset = p_Handle.get_id();

      if (l_Asset.is_loaded()) {
        if (Gui::SaveButton()) {
          Util::String l_JobTitle = "Saving region ";
          l_JobTitle += l_Asset.get_name().c_str();
          register_editor_job(l_JobTitle, [l_Asset] {
            SaveHelper::save_region(l_Asset.get_id());
          });
        }
        if (ImGui::Button("Unload")) {
          l_Asset.unload_entities();
        }
      } else {
        if (ImGui::Button("Load")) {
          l_Asset.load_entities();
        }
      }
    }

    void RegionWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_LC_MAP_PIN " Regions");

      bool l_OpenedEntryPopup = false;

      for (auto it = Core::Region::ms_LivingInstances.begin();
           it != Core::Region::ms_LivingInstances.end(); ++it) {
        if (!it->get_scene().is_loaded()) {
          continue;
        }
        bool i_Break = false;

        Util::String i_Label(it->is_loaded() ? ICON_LC_MAP_PIN
                                             : ICON_LC_MAP_PIN_OFF);
        i_Label += " ";
        i_Label += it->get_name().c_str();
        if (ImGui::Selectable(i_Label.c_str(),
                              it->get_id() ==
                                  get_selected_handle().get_id())) {
          set_selected_handle(*it);
          HandlePropertiesSection l_Section(*it, true);
          l_Section.render_footer = &render_region_details_footer;
          get_details_widget()->add_section(l_Section);
        }

        if (ImGui::BeginPopupContextItem()) {
          l_OpenedEntryPopup = true;
          set_selected_handle(*it);
          if (ImGui::MenuItem("Delete")) {
            Core::Region::destroy(*it);
            i_Break = true;
          }
          ImGui::EndPopup();
        }

        if (i_Break) {
          break;
        }
      }

      if (!l_OpenedEntryPopup) {
        if (ImGui::BeginPopupContextWindow()) {
          if (ImGui::MenuItem("New Region")) {
            Core::Region l_Region = Core::Region::make(N(NewRegion));
            Core::Scene l_Scene = Core::Scene::get_loaded_scene();
            LOW_LOG_DEBUG << "Creating region for scene "
                          << l_Scene.get_name() << LOW_LOG_END;
            l_Region.set_scene(l_Scene);

            l_Region.set_loaded(true);

            set_selected_handle(l_Region);
          }
          ImGui::EndPopup();
        }
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
