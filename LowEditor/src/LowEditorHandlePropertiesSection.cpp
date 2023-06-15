#include "LowEditorHandlePropertiesSection.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowCoreMaterial.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreRigidbody.h"

#include "LowEditorPropertyEditors.h"
#include "LowEditorMainWindow.h"

#include "LowRendererExposedObjects.h"
#include <string.h>
#include <vcruntime_string.h>

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
          }
        } else if (pit->type == Renderer::MaterialTypePropertyType::VECTOR4) {
          Math::Vector4 l_Color = l_Material.get_property(pit->name);

          if (PropertyEditors::render_color_selector(pit->name.c_str(),
                                                     &l_Color)) {
            l_Color.a = 1.0f;
            l_Material.set_property(pit->name, Util::Variant(l_Color));
          }
        }
        ImGui::PopID();
      }

      return false;
    }

    bool HandlePropertiesSection::render_default(float p_Delta)
    {
      uint32_t l_Id = 0;

      for (auto it = m_Metadata.properties.begin();
           it != m_Metadata.properties.end(); ++it) {
        ImGui::PushID(l_Id++);

        Util::RTTI::PropertyInfo &i_PropInfo = it->propInfo;

        if (i_PropInfo.editorProperty) {
          if (i_PropInfo.type == Util::RTTI::PropertyType::HANDLE) {
            PropertyEditors::render_handle_selector(i_PropInfo, m_Handle);
          } else {
            PropertyEditors::render_editor(i_PropInfo, m_Handle,
                                           i_PropInfo.get(m_Handle));
          }
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
