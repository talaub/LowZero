#include "LowEditorHandlePropertiesSection.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowCoreMaterial.h"

#include "LowEditorPropertyEditors.h"

#include "LowRendererExposedObjects.h"

namespace Low {
  namespace Editor {
    void HandlePropertiesSection::render_material(float p_Delta)
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
        }
        ImGui::PopID();
      }
    }

    void HandlePropertiesSection::render_default(float p_Delta)
    {
      uint32_t l_Id = 0;
      for (auto pit = m_TypeInfo.properties.begin();
           pit != m_TypeInfo.properties.end(); ++pit) {

        ImGui::PushID(l_Id++);

        if (pit->second.editorProperty) {
          if (pit->second.type == Util::RTTI::PropertyType::HANDLE) {
            PropertyEditors::render_handle_selector(pit->second, m_Handle);
          } else {
            PropertyEditors::render_editor(pit->second,
                                           pit->second.get(m_Handle));
          }
        }
        ImGui::PopID();
      }
    }

    void HandlePropertiesSection::render(float p_Delta)
    {
      if (!m_TypeInfo.is_alive(m_Handle)) {
        return;
      }

      if (ImGui::CollapsingHeader(m_TypeInfo.name.c_str(),
                                  m_DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen
                                                : 0)) {
        ImGui::PushID(m_Handle.get_id());
        if (m_Handle.get_type() == Core::Material::TYPE_ID) {
          render_material(p_Delta);
        } else {
          render_default(p_Delta);
        }
        if (render_footer != nullptr) {
          render_footer(m_Handle, m_TypeInfo);
        }
        ImGui::PopID();
      }
    }

    HandlePropertiesSection::HandlePropertiesSection(
        const Util::Handle p_Handle, bool p_DefaultOpen)
        : m_Handle(p_Handle), m_DefaultOpen(p_DefaultOpen)
    {
      m_TypeInfo = Util::Handle::get_type_info(p_Handle.get_type());
    }
  } // namespace Editor
} // namespace Low
