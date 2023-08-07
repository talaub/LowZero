#include "LowEditorHandlePropertiesSection.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowCoreMaterial.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreRigidbody.h"
#include "LowCorePrefabInstance.h"

#include "LowEditorPropertyEditors.h"
#include "LowEditorMainWindow.h"
#include "LowEditorGui.h"
#include "LowEditorAssetWidget.h"
#include "LowEditorChangeList.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorAssetOperations.h"

#include "LowRendererExposedObjects.h"
#include "LowRendererImGuiHelper.h"

#include "LowUtilString.h"
#include "LowUtilDiffUtil.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vcruntime_string.h>
#include <algorithm>

namespace Low {
  namespace Editor {
    bool HandlePropertiesSection::render_entity(float p_Delta)
    {
      Core::Entity l_Entity = m_Handle.get_id();
      ImGui::Text("Name");
      ImGui::SameLine();

      bool l_IsChildEntity =
          Core::Component::Transform(l_Entity.get_transform().get_parent())
              .is_alive();

      bool l_IsPrefabInstance =
          l_Entity.has_component(Core::Component::PrefabInstance::TYPE_ID) &&
          Core::Component::PrefabInstance(
              l_Entity.get_component(Core::Component::PrefabInstance::TYPE_ID))
              .get_prefab()
              .is_alive();

      bool l_Added = false;

      static char l_NameBuffer[255];
      static Core::Entity l_LastEntity = 0;

      if (l_LastEntity != l_Entity) {
        memcpy(l_NameBuffer, l_Entity.get_name().c_str(),
               strlen(l_Entity.get_name().c_str()) + 1);
        l_LastEntity = l_Entity;
      }

      if (ImGui::InputText("##name", l_NameBuffer, 255,
                           ImGuiInputTextFlags_EnterReturnsTrue)) {
        l_Entity.set_name(LOW_NAME(l_NameBuffer));
      }

      if (!l_IsChildEntity) {
        ImGui::BeginGroup();
        ImGui::Text("Region: ");
        ImGui::SameLine();
        if (l_Entity.get_region().is_alive()) {
          ImGui::Text(l_Entity.get_region().get_name().c_str());
        } else {
          ImGui::Text("None");
        }

        ImGui::SameLine();
        {
          Util::String l_PopupName = "region_selector";
          if (ImGui::Button("Choose...")) {
            ImGui::OpenPopup(l_PopupName.c_str());
          }

          if (ImGui::BeginPopup(l_PopupName.c_str())) {

            for (Core::Region i_Region : Core::Region::ms_LivingInstances) {
              if (!i_Region.get_scene().is_loaded()) {
                continue;
              }
              if (ImGui::Selectable(i_Region.get_name().c_str())) {
                i_Region.add_entity(l_Entity);
              }
            }
            ImGui::EndPopup();
          }
        }
        ImGui::EndGroup();
      }

      if (l_IsPrefabInstance) {
        int l_GreyVal = 120;
        ImGui::PushStyleColor(ImGuiCol_Text,
                              IM_COL32(l_GreyVal, l_GreyVal, l_GreyVal, 255));
        ImGui::PushFont(Renderer::ImGuiHelper::fonts().common_300);
        ImGui::Text("Prefab instance");
        ImGui::PopFont();
        ImGui::PopStyleColor();
      }

      if (ImGui::Button("Add component")) {
        ImGui::OpenPopup("add_component_popup");
      }

      if (ImGui::BeginPopup("add_component_popup")) {
        for (auto it = Util::Handle::get_component_types().begin();
             it != Util::Handle::get_component_types().end(); ++it) {
          if (l_Entity.has_component(*it)) {
            continue;
          }
          Util::RTTI::TypeInfo &i_TypeInfo = Util::Handle::get_type_info(*it);

          if (ImGui::MenuItem(i_TypeInfo.name.c_str())) {
            i_TypeInfo.make_component(l_Entity.get_id());
            set_selected_entity(l_Entity);
            l_Added = true;
          }
        }
        ImGui::EndPopup();
      }

      return l_Added;
    }

    bool HandlePropertiesSection::render_material(float p_Delta)
    {
      Core::Material l_Material = m_Handle.get_id();

      Util::StoredHandle l_StoredHandle;
      Util::DiffUtil::store_handle(l_StoredHandle, m_Handle);

      Util::Map<Util::Name, Util::Variant> l_StoredProperties =
          l_Material.get_properties();

      Util::List<Util::Name> l_ChangedProperties;

      ImGui::Text("Type");
      ImGui::SameLine();

      if (ImGui::BeginCombo("##typeselector",
                            l_Material.get_material_type().get_name().c_str(),
                            0)) {
        for (auto it = Renderer::MaterialType::ms_LivingInstances.begin();
             it != Renderer::MaterialType::ms_LivingInstances.end(); ++it) {
          bool i_Selected = *it == l_Material.get_material_type();

          if (it->is_internal()) {
            continue;
          }
          if (ImGui::Selectable(it->get_name().c_str(), i_Selected)) {
            l_Material.set_material_type(*it);
          }
          if (i_Selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      Renderer::MaterialType l_MaterialType = l_Material.get_material_type();

      bool l_FirstEdit = true;

      uint32_t l_Id = 0;
      for (auto pit = l_MaterialType.get_properties().begin();
           pit != l_MaterialType.get_properties().end(); ++pit) {

        ImGui::PushID(l_Id++);

        if (pit->type == Renderer::MaterialTypePropertyType::TEXTURE2D) {
          uint64_t i_TextureId = (uint64_t)l_Material.get_property(pit->name);

          if (PropertyEditors::render_handle_selector(
                  pit->name.c_str(), Core::Texture2D::TYPE_ID, &i_TextureId)) {
            l_Material.set_property(pit->name,
                                    Util::Variant::from_handle(i_TextureId));

            l_ChangedProperties.push_back(pit->name);
          } else if (m_CurrentlyEditing == pit->name.c_str()) {
            m_CurrentlyEditing = "";
          }
        }
        if (pit->type == Renderer::MaterialTypePropertyType::VECTOR4) {
          Math::Vector4 l_Color = l_Material.get_property(pit->name);

          if (PropertyEditors::render_color_selector(pit->name.c_str(),
                                                     &l_Color)) {
            l_FirstEdit = m_CurrentlyEditing != pit->name.c_str();
            m_CurrentlyEditing = pit->name.c_str();
            l_Color.a = 1.0f;
            l_Material.set_property(pit->name, Util::Variant(l_Color));
            l_ChangedProperties.push_back(pit->name);
          } else if (m_CurrentlyEditing == pit->name.c_str()) {
            m_CurrentlyEditing = "";
          }
        }
        ImGui::PopID();
      }

      Util::StoredHandle l_AfterChange;
      Util::DiffUtil::store_handle(l_AfterChange, m_Handle);

      Transaction l_Transaction =
          Transaction::from_diff(m_Handle, l_StoredHandle, l_AfterChange);

      for (auto it = l_ChangedProperties.begin();
           it != l_ChangedProperties.end(); ++it) {
        l_Transaction.add_operation(
            new AssetOperations::MaterialPropertyEditOperation(
                m_Handle, *it, l_StoredProperties[*it],
                l_Material.get_property(*it)));
      }

      l_Transaction.m_Title = "Edit material";

      if (!l_FirstEdit) {
        Transaction l_OldTransaction = get_global_changelist().peek();

        for (uint32_t i = 0; i < l_Transaction.get_operations().size(); ++i) {
          AssetOperations::MaterialPropertyEditOperation *i_Operation =
              (AssetOperations::MaterialPropertyEditOperation *)
                  l_Transaction.get_operations()[i];
          for (uint32_t j = 0; j < l_OldTransaction.get_operations().size();
               ++j) {
            AssetOperations::MaterialPropertyEditOperation *i_OldOperation =
                (AssetOperations::MaterialPropertyEditOperation *)
                    l_OldTransaction.get_operations()[j];

            if (i_OldOperation->m_Handle == i_Operation->m_Handle &&
                i_OldOperation->m_PropertyName == i_Operation->m_PropertyName) {
              i_Operation->m_OldValue = i_OldOperation->m_OldValue;
            }
          }
        }
        Transaction l_Old = get_global_changelist().pop();
        l_Old.cleanup();
      }

      get_global_changelist().add_entry(l_Transaction);

      return false;
    }

    bool HandlePropertiesSection::render_default(float p_Delta)
    {
      uint32_t l_Id = 0;
      Util::Name l_EntityName = N(entity);

      for (auto it = m_Metadata.properties.begin();
           it != m_Metadata.properties.end(); ++it) {
        Util::String i_Id =
            LOW_TO_STRING(l_Id) + LOW_TO_STRING(m_Metadata.typeId);
        ImGui::PushID(std::stoul(i_Id.c_str()));

        l_Id++;
        Util::RTTI::PropertyInfo &i_PropInfo = it->propInfo;

        if (i_PropInfo.editorProperty) {
          ImVec2 l_Pos = ImGui::GetCursorScreenPos();
          float l_FullWidth = ImGui::GetContentRegionAvail().x;
          float l_LabelWidth = l_FullWidth * LOW_EDITOR_LABEL_WIDTH_REL;
          float l_LabelHeight = 20.0f;

          if (i_PropInfo.type == Util::RTTI::PropertyType::HANDLE) {
            PropertyEditors::render_handle_selector(i_PropInfo, m_Handle);
          } else {
            PropertyEditors::render_editor(i_PropInfo, m_Handle,
                                           i_PropInfo.get(m_Handle));
          }
          ImVec2 l_PosNew = ImGui::GetCursorScreenPos();

          Util::String i_InvisButtonId = "IVB_";
          i_InvisButtonId += i_PropInfo.name.c_str() + i_Id;

          ImGui::SetCursorScreenPos(l_Pos);

          ImGui::InvisibleButton(i_InvisButtonId.c_str(),
                                 {l_LabelWidth, l_LabelHeight});
          if (ImGui::BeginPopupContextItem(i_InvisButtonId.c_str())) {
            if (m_Metadata.typeInfo.component) {
              Core::Entity i_Entity =
                  *(Core::Entity *)m_Metadata.typeInfo.properties[l_EntityName]
                       .get(m_Handle);
              if (i_Entity.has_component(
                      Core::Component::PrefabInstance::TYPE_ID)) {
                Core::Component::PrefabInstance i_Instance =
                    i_Entity.get_component(
                        Core::Component::PrefabInstance::TYPE_ID);
                if (i_Instance.get_prefab().is_alive()) {
                  if (i_Instance.get_overrides().find(m_Metadata.typeId) !=
                      i_Instance.get_overrides().end()) {
                    if (std::find(
                            i_Instance.get_overrides()[m_Metadata.typeId]
                                .begin(),
                            i_Instance.get_overrides()[m_Metadata.typeId].end(),
                            i_PropInfo.name) !=
                        i_Instance.get_overrides()[m_Metadata.typeId].end()) {
                      if (ImGui::MenuItem("Apply to prefab")) {
                        i_Instance.get_prefab().apply(m_Handle,
                                                      i_PropInfo.name);
                        Core::Prefab i_Prefab = i_Instance.get_prefab();
                        while (Core::Prefab(i_Prefab.get_parent().get_id())
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

      return false;
    }

    void HandlePropertiesSection::render(float p_Delta)
    {
      if (!m_Metadata.typeInfo.is_alive(m_Handle)) {
        return;
      }

      if (ImGui::CollapsingHeader(m_Metadata.name.c_str(),
                                  m_DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen
                                                : 0)) {

        Util::StoredHandle l_StoredHandle;
        Util::DiffUtil::store_handle(l_StoredHandle, m_Handle);

        bool l_SkipFooter = false;

        ImGui::PushID(m_Handle.get_id());
        if (m_Handle.get_type() == Core::Material::TYPE_ID) {
          l_SkipFooter = l_SkipFooter || render_material(p_Delta);
        } else if (m_Handle.get_type() == Core::Entity::TYPE_ID) {
          l_SkipFooter = l_SkipFooter || render_entity(p_Delta);
        } else {
          l_SkipFooter = l_SkipFooter || render_default(p_Delta);
        }
        if (!l_SkipFooter && render_footer != nullptr) {
          render_footer(m_Handle, m_Metadata.typeInfo);
        }
        ImGui::PopID();

        Util::StoredHandle l_AfterChange;
        Util::DiffUtil::store_handle(l_AfterChange, m_Handle);

        if (!Core::Material::is_alive(m_Handle)) {
          Transaction l_Transaction =
              Transaction::from_diff(m_Handle, l_StoredHandle, l_AfterChange);

          get_global_changelist().add_entry(l_Transaction);
        }
      }
    }

    HandlePropertiesSection::HandlePropertiesSection(
        const Util::Handle p_Handle, bool p_DefaultOpen)
        : m_Handle(p_Handle), m_DefaultOpen(p_DefaultOpen)
    {
      m_Metadata = get_type_metadata(p_Handle.get_type());
    }
  } // namespace Editor
} // namespace Low
