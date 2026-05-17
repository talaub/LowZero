#include "LowEditorRegionWidget.h"

#include "imgui.h"
#include "IconsLucide.h"

#include "LowEditor.h"
#include "LowEditorMainWindow.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorHandlePropertiesSection.h"
#include "LowEditorSaveHelper.h"
#include "LowEditorGui.h"
#include "LowEditorEntityTree.h"

#include "LowCoreEntity.h"
#include "LowCoreRegion.h"
#include "LowCoreTransform.h"

#include "LowUtilString.h"

namespace Low {
  namespace Editor {
    void
    render_region_details_footer(Util::Handle p_Handle,
                                 Util::RTTI::TypeInfo &p_TypeInfo);

    static bool region_matches_search(Core::Region p_Region,
                                      const Util::String &p_Search)
    {
      if (p_Search.empty()) {
        return true;
      }

      Util::String l_LowName = p_Region.get_name().c_str();
      l_LowName.make_lower();
      if (Util::StringHelper::contains(l_LowName, p_Search)) {
        return true;
      }

      for (auto it = Core::Entity::ms_LivingInstances.begin();
           it != Core::Entity::ms_LivingInstances.end(); ++it) {
        if (it->get_region() != p_Region) {
          continue;
        }
        if (EntityTree::matches_search(*it, p_Search)) {
          return true;
        }
      }

      return false;
    }

    static void select_region(Core::Region p_Region)
    {
      set_selected_handle(p_Region);
      HandlePropertiesSection l_Section(p_Region, true);
      l_Section.render_footer = &render_region_details_footer;
      get_details_widget()->add_section(l_Section);
    }

    static void accept_entity_drop_on_region(Core::Region p_Region)
    {
      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *l_Payload =
                ImGui::AcceptDragDropPayload("DG_HANDLE")) {
          Core::Entity l_Entity = *(uint64_t *)l_Payload->Data;
          if (l_Entity.is_alive()) {
            EntityTree::detach_from_parent_preserve_world(l_Entity);
            p_Region.add_entity(l_Entity);
          }
        }
        ImGui::EndDragDropTarget();
      }
    }

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
      (void)p_Delta;

      ImGui::Begin(ICON_LC_MAP_PIN " Regions");

      if (Gui::AddButton("Create")) {
        Core::Region l_Region = Core::Region::make(N(NewRegion));
        Core::Scene l_Scene = Core::Scene::get_loaded_scene();
        LOW_LOG_DEBUG << "Creating region for scene "
                      << l_Scene.get_name() << LOW_LOG_END;
        l_Region.set_scene(l_Scene);

        l_Region.set_loaded(true);

        select_region(l_Region);
      }

      ImGui::SameLine();

      static char l_Search[128] = "";
      Gui::SearchField("##regionsearchinput", l_Search,
                       IM_ARRAYSIZE(l_Search), {0.0f, 2.0f});

      ImGui::Dummy({0.0f, 4.0f});

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      bool l_OpenedEntryPopup = false;

      for (auto it = Core::Region::ms_LivingInstances.begin();
           it != Core::Region::ms_LivingInstances.end(); ++it) {
        if (!it->get_scene().is_loaded()) {
          continue;
        }
        if (!region_matches_search(*it, l_SearchString)) {
          continue;
        }

        bool i_Break = false;

        Util::String i_Label(it->is_loaded() ? ICON_LC_MAP_PIN
                                             : ICON_LC_MAP_PIN_OFF);
        i_Label += " ";
        i_Label += it->get_name().c_str();

        ImGuiTreeNodeFlags i_Flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (is_selected(it->get_id())) {
          i_Flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (!l_SearchString.empty()) {
          i_Flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        Util::String i_IdString = LOW_TO_STRING(it->get_id());
        ImGui::PushID(i_IdString.c_str());
        bool i_Open = ImGui::TreeNodeEx("##region_row", i_Flags,
                                        "%s", i_Label.c_str());

        if (ImGui::IsItemClicked()) {
          select_region(*it);
        }

        accept_entity_drop_on_region(*it);

        if (ImGui::BeginPopupContextItem()) {
          l_OpenedEntryPopup = true;
          select_region(*it);
          if (ImGui::MenuItem("Delete")) {
            Core::Region::destroy(*it);
            i_Break = true;
          }
          ImGui::EndPopup();
        }

        if (i_Open && !i_Break) {
          for (auto entityIt = Core::Entity::ms_LivingInstances.begin();
               entityIt != Core::Entity::ms_LivingInstances.end();
               ++entityIt) {
            if (entityIt->get_region() != *it) {
              continue;
            }

            Core::Component::Transform i_Parent =
                entityIt->get_transform().get_parent();
            if (i_Parent.is_alive() &&
                i_Parent.get_entity().get_region() == *it) {
              continue;
            }
            if (!EntityTree::matches_search(*entityIt,
                                            l_SearchString)) {
              continue;
            }

            EntityTree::RenderOptions i_Options;
            i_Options.filterToRegion = true;
            i_Options.regionFilter = *it;
            i_Options.moveDroppedEntitiesToParentRegion = true;

            if (EntityTree::render(*entityIt, &l_OpenedEntryPopup,
                                   l_SearchString, i_Options)) {
              i_Break = true;
              break;
            }
          }
          ImGui::TreePop();
        }

        ImGui::PopID();

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

            select_region(l_Region);
          }
          ImGui::EndPopup();
        }
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
