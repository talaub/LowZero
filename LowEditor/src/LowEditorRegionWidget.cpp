#include "LowEditorRegionWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowEditorMainWindow.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorHandlePropertiesSection.h"

#include "LowCoreRegion.h"

namespace Low {
  namespace Editor {
    void render_region_details_footer(Util::Handle p_Handle,
                                      Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::Region l_Asset = p_Handle.get_id();

      if (l_Asset.is_loaded()) {
        if (ImGui::Button("Save")) {
          {
            Util::Yaml::Node l_Node;
            l_Asset.serialize(l_Node);
            Util::String l_Path = LOW_DATA_PATH;
            l_Path +=
                "/assets/regions/" +
                Util::String(std::to_string(l_Asset.get_unique_id()).c_str()) +
                ".region.yaml";
            Util::Yaml::write_file(l_Path.c_str(), l_Node);
          }

          {
            Util::Yaml::Node l_Node;
            l_Asset.serialize_entities(l_Node);
            Util::String l_Path = LOW_DATA_PATH;
            l_Path +=
                "/assets/regions/" +
                Util::String(std::to_string(l_Asset.get_unique_id()).c_str()) +
                ".entities.yaml";
            Util::Yaml::write_file(l_Path.c_str(), l_Node);
          }

          LOW_LOG_INFO << "Saved region '" << l_Asset.get_name() << "' to file."
                       << LOW_LOG_END;
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
      ImGui::Begin(ICON_FA_MAP_MARKER_ALT " Regions");

      for (auto it = Core::Region::ms_LivingInstances.begin();
           it != Core::Region::ms_LivingInstances.end(); ++it) {

        Util::String i_Label(it->is_loaded() ? ICON_FA_MAP_MARKER_ALT
                                             : ICON_FA_POWER_OFF);
        i_Label += " ";
        i_Label += it->get_name().c_str();
        if (ImGui::Selectable(i_Label.c_str(),
                              it->get_id() == get_selected_handle().get_id())) {
          set_selected_handle(*it);
          HandlePropertiesSection l_Section(*it, true);
          l_Section.render_footer = &render_region_details_footer;
          get_details_widget()->add_section(l_Section);
        }
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
