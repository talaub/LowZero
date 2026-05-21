#include "LowEditorTypeEditor.h"

#include "LowEditor.h"
#include "LowEditorMetadata.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorGui.h"
#include "LowEditorAssetWidget.h"

#include "imgui.h"

#include "LowUtil.h"
#include "LowUtilString.h"

#include "LowCore.h"
#include "LowCorePrefab.h"
#include "LowCorePrefabInstance.h"
#include <memory>

#define TYPE_MANAGER_HANDLER(eventname)                              \
  void TypeEditor::handle_##eventname(Util::Handle p_Handle,         \
                                      TypeMetadata &p_Metadata)      \
  {                                                                  \
    auto l_Entry = g_EventHandlers.find(p_Metadata.typeId);          \
    if (l_Entry != g_EventHandlers.end()) {                          \
      l_Entry->second->eventname(p_Handle, p_Metadata);              \
    } else {                                                         \
      TypeEventHandler l_Handler;                                    \
      l_Handler.eventname(p_Handle, p_Metadata);                     \
    }                                                                \
  }                                                                  \
  void TypeEditor::handle_##eventname(Util::Handle p_Handle)         \
  {                                                                  \
    TypeMetadata l_Metadata =                                        \
        get_type_metadata(p_Handle.get_type());                      \
    handle_##eventname(p_Handle, l_Metadata);                        \
  }

namespace Low {
  namespace Editor {
    Util::Map<u16, TypeEditorFactory *> g_Factories;
    Util::Map<u16, TypeEventHandler *> g_EventHandlers;

    TYPE_MANAGER_HANDLER(after_add)
    TYPE_MANAGER_HANDLER(before_delete)
    TYPE_MANAGER_HANDLER(before_save)
    TYPE_MANAGER_HANDLER(after_save)

    void TypeEditor::register_factory_for_type(
        u16 p_TypeId, TypeEditorFactory *p_Factory)
    {
      auto l_Entry = g_Factories.find(p_TypeId);
      if (l_Entry != g_Factories.end()) {
        LOW_LOG_WARN << "There is already a type editor factory "
                        "registered for "
                     << p_TypeId << LOW_LOG_END;
        return;
      }

      g_Factories[p_TypeId] = p_Factory;
    }

    void TypeEditor::register_event_handler_for_type(
        u16 p_TypeId, TypeEventHandler *p_Handler)
    {
      auto l_Entry = g_EventHandlers.find(p_TypeId);
      if (l_Entry != g_EventHandlers.end()) {
        LOW_LOG_WARN << "There is already a type event handler "
                        "registered for "
                     << p_TypeId << LOW_LOG_END;
        return;
      }

      g_EventHandlers[p_TypeId] = p_Handler;
    }

    bool TypeEditor::has_factory(u16 p_TypeId)
    {
      return g_Factories.find(p_TypeId) == g_Factories.end();
    }

    TypeEditor *TypeEditor::create(Util::Handle p_Handle)
    {
      auto l_Entry = g_Factories.find(p_Handle.get_type());
      if (l_Entry != g_Factories.end()) {
        return l_Entry->second->create(p_Handle);
      }

      return new TypeEditor(p_Handle);
    }

    std::unique_ptr<TypeEditor>
    TypeEditor::create_unique(Util::Handle p_Handle)
    {
      auto l_Entry = g_Factories.find(p_Handle.get_type());
      if (l_Entry != g_Factories.end()) {
        return l_Entry->second->create_unique(p_Handle);
      }

      return std::make_unique<TypeEditor>(p_Handle);
    }

    void TypeEditor::show_line(const Util::String p_Label,
                               Util::Function<bool()> p_Function)
    {
      PropertyEditors::render_line(p_Label, p_Function);
    }

    void TypeEditor::show_line(const Util::String p_Label,
                               const Util::String p_Content)
    {
      PropertyEditors::render_line(p_Label, [p_Content]() -> bool {
        ImGui::Text(p_Content.c_str());
        return false;
      });
    }

    void TypeEditor::show_editor(Util::Name p_PropertyName)
    {
      /*
      PropertyMetadataBase l_PropBase =
          m_Metadata.find_property_base_by_name(p_PropertyName);

      show_line(l_PropBase.friendlyName.c_str(),
                [&, p_PropertyName]() -> bool {
                  PropertyEditors::render_editor(m_Handle,
                                                 p_PropertyName);
                  return false;
                });
                */
      PropertyEditors::render_editor(m_Handle, p_PropertyName);
    }

    void TypeEditor::cleanup_registered_types()
    {
      for (auto it : g_Factories) {
        delete it.second;
      }

      g_Factories.clear();

      for (auto it : g_EventHandlers) {
        delete it.second;
      }

      g_EventHandlers.clear();
    }

    void TypeEditor::show()
    {
      render(LOW_DELTA_TIME);
    }

    void TypeEditor::render(const float p_Delta)
    {
      uint32_t l_Id = 0;
      static Util::Name l_EntityName = N(entity);

      for (auto it = m_Metadata.properties.begin();
           it != m_Metadata.properties.end(); ++it) {
        Util::String i_Id =
            LOW_TO_STRING(l_Id) + LOW_TO_STRING(m_Metadata.typeId);
        ImGui::PushID(std::stoul(i_Id.c_str()));

        PropertyMetadata l_PropMetadata = *it;

        l_Id++;

        Util::RTTI::PropertyInfo &i_PropInfo = it->propInfo;
        if (i_PropInfo.editorProperty) {
          show_editor(l_PropMetadata.name);
        }
        ImGui::PopID();
        // Saved the code after this continue statement in case we
        // need it later
        continue;

        if (i_PropInfo.editorProperty) {
          ImVec2 l_Pos = ImGui::GetCursorScreenPos();
          float l_FullWidth = ImGui::GetContentRegionAvail().x;
          float l_LabelWidth =
              l_FullWidth * LOW_EDITOR_LABEL_WIDTH_REL;
          float l_LabelHeight = 20.0f;

          if (it->enumType ||
              i_PropInfo.type == Util::RTTI::PropertyType::ENUM) {
            PropertyEditors::render_enum_selector(l_PropMetadata,
                                                  m_Handle);
          } else if (i_PropInfo.type ==
                     Util::RTTI::PropertyType::HANDLE) {
            PropertyEditors::render_handle_selector(l_PropMetadata,
                                                    m_Handle);
          } else {
            PropertyEditors::render_editor(
                l_PropMetadata, m_Handle,
                i_PropInfo.get_return(m_Handle));
          }
          ImVec2 l_PosNew = ImGui::GetCursorScreenPos();

          Util::String i_InvisButtonId = "IVB_";
          i_InvisButtonId += i_PropInfo.name.c_str() + i_Id;

          ImGui::SetCursorScreenPos(l_Pos);

          ImGui::InvisibleButton(i_InvisButtonId.c_str(),
                                 {l_LabelWidth, l_LabelHeight});
          if (ImGui::BeginPopupContextItem(i_InvisButtonId.c_str())) {
            if (m_Metadata.typeInfo.component) {
              Core::Entity i_Entity;
              m_Metadata.typeInfo.properties[l_EntityName].get(
                  m_Handle, &i_Entity);
              if (i_Entity.has_component(
                      Core::Component::PrefabInstance::type_id())) {
                Core::Component::PrefabInstance i_Instance =
                    i_Entity.get_component(
                        Core::Component::PrefabInstance::type_id());
                if (i_Instance.get_prefab().is_alive()) {
                  if (i_Instance.get_overrides().find(
                          m_Metadata.typeId) !=
                      i_Instance.get_overrides().end()) {
                    if (std::find(
                            i_Instance
                                .get_overrides()[m_Metadata.typeId]
                                .begin(),
                            i_Instance
                                .get_overrides()[m_Metadata.typeId]
                                .end(),
                            i_PropInfo.name) !=
                        i_Instance.get_overrides()[m_Metadata.typeId]
                            .end()) {
                      if (ImGui::MenuItem("Apply to prefab")) {
                        i_Instance.get_prefab().apply(
                            m_Handle, i_PropInfo.name);
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
      for (auto it = m_Metadata.virtualProperties.begin();
           it != m_Metadata.virtualProperties.end(); ++it) {
        Util::String i_Id =
            LOW_TO_STRING(l_Id) + LOW_TO_STRING(m_Metadata.typeId);
        ImGui::PushID(std::stoul(i_Id.c_str()));

        VirtualPropertyMetadata l_PropMetadata = *it;

        l_Id++;

        if (l_PropMetadata.editor) {
          show_editor(l_PropMetadata.name);
        }
        ImGui::PopID();
      }
    }
  } // namespace Editor
} // namespace Low
