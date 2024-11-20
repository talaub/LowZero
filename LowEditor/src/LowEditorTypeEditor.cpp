#include "LowEditorTypeEditor.h"

#include "LowEditor.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorGui.h"
#include "LowEditorAssetWidget.h"

#include "imgui.h"

#include "LowUtil.h"
#include "LowUtilString.h"

#include "LowCorePrefab.h"
#include "LowCorePrefabInstance.h"

namespace Low {
  namespace Editor {
    Util::Map<u16, TypeEditor *> g_TypeEditors;

    void TypeEditor::register_for_type(u16 p_TypeId,
                                       TypeEditor *p_Editor)
    {
      auto l_Entry = g_TypeEditors.find(p_TypeId);
      if (l_Entry != g_TypeEditors.end()) {
        LOW_LOG_WARN
            << "There is already a type editor registered for "
            << p_TypeId << LOW_LOG_END;
        return;
      }

      g_TypeEditors[p_TypeId] = p_Editor;
    }

    void TypeEditor::show_editor(Util::Handle p_Handle,
                                 TypeMetadata &p_Metadata,
                                 Util::Name p_PropertyName)
    {
      PropertyEditors::render_editor(p_Handle, p_Metadata,
                                     p_PropertyName);
    }

    void TypeEditor::show_editor(Util::Handle p_Handle,
                                 Util::Name p_PropertyName)
    {
      PropertyEditors::render_editor(p_Handle, p_PropertyName);
    }

    void TypeEditor::cleanup_registered_types()
    {
      for (auto it : g_TypeEditors) {
        delete it.second;
      }

      g_TypeEditors.clear();
    }

    void TypeEditor::show(Util::Handle p_Handle)
    {
      TypeMetadata l_Metadata =
          get_type_metadata(p_Handle.get_type());

      show(p_Handle, l_Metadata);
    }

    void TypeEditor::show(Util::Handle p_Handle,
                          TypeMetadata &p_Metadata)
    {
      auto l_Entry = g_TypeEditors.find(p_Metadata.typeId);

      if (l_Entry != g_TypeEditors.end()) {
        l_Entry->second->render(p_Handle, p_Metadata);
      } else {
        TypeEditor l_Editor;
        l_Editor.render(p_Handle, p_Metadata);
      }
    }

    void TypeEditor::handle_after_add(Util::Handle p_Handle,
                                      TypeMetadata &p_Metadata)
    {
      auto l_Entry = g_TypeEditors.find(p_Metadata.typeId);

      if (l_Entry != g_TypeEditors.end()) {
        l_Entry->second->after_add(p_Handle, p_Metadata);
      } else {
        TypeEditor l_Editor;
        l_Editor.after_add(p_Handle, p_Metadata);
      }
    }

    void TypeEditor::handle_after_add(Util::Handle p_Handle)
    {
      TypeMetadata l_Metadata =
          get_type_metadata(p_Handle.get_type());

      handle_after_add(p_Handle, l_Metadata);
    }

    void TypeEditor::handle_before_delete(Util::Handle p_Handle,
                                          TypeMetadata &p_Metadata)
    {
      auto l_Entry = g_TypeEditors.find(p_Metadata.typeId);

      if (l_Entry != g_TypeEditors.end()) {
        l_Entry->second->before_delete(p_Handle, p_Metadata);
      } else {
        TypeEditor l_Editor;
        l_Editor.before_delete(p_Handle, p_Metadata);
      }
    }

    void TypeEditor::handle_before_delete(Util::Handle p_Handle)
    {
      TypeMetadata l_Metadata =
          get_type_metadata(p_Handle.get_type());

      handle_before_delete(p_Handle, l_Metadata);
    }

    void TypeEditor::render(Util::Handle p_Handle,
                            TypeMetadata &p_Metadata)
    {
      uint32_t l_Id = 0;
      static Util::Name l_EntityName = N(entity);

      for (auto it = p_Metadata.properties.begin();
           it != p_Metadata.properties.end(); ++it) {
        Util::String i_Id =
            LOW_TO_STRING(l_Id) + LOW_TO_STRING(p_Metadata.typeId);
        ImGui::PushID(std::stoul(i_Id.c_str()));

        PropertyMetadata l_PropMetadata = *it;

        l_Id++;
        Util::RTTI::PropertyInfo &i_PropInfo = it->propInfo;

        if (i_PropInfo.editorProperty) {
          ImVec2 l_Pos = ImGui::GetCursorScreenPos();
          float l_FullWidth = ImGui::GetContentRegionAvail().x;
          float l_LabelWidth =
              l_FullWidth * LOW_EDITOR_LABEL_WIDTH_REL;
          float l_LabelHeight = 20.0f;

          if (it->enumType ||
              i_PropInfo.type == Util::RTTI::PropertyType::ENUM) {
            PropertyEditors::render_enum_selector(l_PropMetadata,
                                                  p_Handle);
          } else if (i_PropInfo.type ==
                     Util::RTTI::PropertyType::HANDLE) {
            PropertyEditors::render_handle_selector(l_PropMetadata,
                                                    p_Handle);
          } else {
            PropertyEditors::render_editor(l_PropMetadata, p_Handle,
                                           i_PropInfo.get(p_Handle));
          }
          ImVec2 l_PosNew = ImGui::GetCursorScreenPos();

          Util::String i_InvisButtonId = "IVB_";
          i_InvisButtonId += i_PropInfo.name.c_str() + i_Id;

          ImGui::SetCursorScreenPos(l_Pos);

          ImGui::InvisibleButton(i_InvisButtonId.c_str(),
                                 {l_LabelWidth, l_LabelHeight});
          if (ImGui::BeginPopupContextItem(i_InvisButtonId.c_str())) {
            if (p_Metadata.typeInfo.component) {
              Core::Entity i_Entity =
                  *(Core::Entity *)p_Metadata.typeInfo
                       .properties[l_EntityName]
                       .get(p_Handle);
              if (i_Entity.has_component(
                      Core::Component::PrefabInstance::TYPE_ID)) {
                Core::Component::PrefabInstance i_Instance =
                    i_Entity.get_component(
                        Core::Component::PrefabInstance::TYPE_ID);
                if (i_Instance.get_prefab().is_alive()) {
                  if (i_Instance.get_overrides().find(
                          p_Metadata.typeId) !=
                      i_Instance.get_overrides().end()) {
                    if (std::find(
                            i_Instance
                                .get_overrides()[p_Metadata.typeId]
                                .begin(),
                            i_Instance
                                .get_overrides()[p_Metadata.typeId]
                                .end(),
                            i_PropInfo.name) !=
                        i_Instance.get_overrides()[p_Metadata.typeId]
                            .end()) {
                      if (ImGui::MenuItem("Apply to prefab")) {
                        i_Instance.get_prefab().apply(
                            p_Handle, i_PropInfo.name);
                        Core::Prefab i_Prefab =
                            i_Instance.get_prefab();
                        while (Core::Prefab(
                                   i_Prefab.get_parent().get_id())
                                   .is_alive()) {
                          i_Prefab = i_Prefab.get_parent().get_id();
                        }
                        AssetWidget::save_prefab_asset(i_Prefab);
                      }
                    }
                  }
                }
              }
            }
            ImGui::EndPopup();
          }
          ImGui::SetCursorScreenPos(l_PosNew);
        }
        ImGui::PopID();
      }
    }
  } // namespace Editor
} // namespace Low
