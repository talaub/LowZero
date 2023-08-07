#include "LowEditorAssetOperations.h"

#include "LowEditorMainWindow.h"

#include "LowUtilLogger.h"

#include "LowCoreMaterial.h"

#include "LowRendererExposedObjects.h"

namespace Low {
  namespace Editor {
    namespace AssetOperations {
      MaterialPropertyEditOperation::MaterialPropertyEditOperation(
          Util::Handle p_Handle, Util::Name p_PropertyName,
          Util::Variant p_OldValue, Util::Variant p_NewValue)
          : m_Handle(p_Handle), m_PropertyName(p_PropertyName),
            m_OldValue(p_OldValue), m_NewValue(p_NewValue)
      {
      }

      void MaterialPropertyEditOperation::execute(ChangeList &p_ChangeList,
                                                  Transaction &p_Transaction)
      {
        Util::Handle l_OldHandle = m_Handle;
        m_Handle = p_ChangeList.get_mapping(m_Handle);

        Core::Material l_Material = m_Handle.get_id();
        if (l_Material.is_alive()) {
          uint8_t l_PropertyType = 0;
          for (auto it =
                   l_Material.get_material_type().get_properties().begin();
               it != l_Material.get_material_type().get_properties().end();
               ++it) {
            if (it->name == m_PropertyName) {
              l_PropertyType = it->type;
            }
          }
          if (l_PropertyType == Renderer::MaterialTypePropertyType::TEXTURE2D) {
            m_NewValue.set_handle(p_ChangeList.get_mapping(m_NewValue));
          }
          l_Material.set_property(m_PropertyName, m_NewValue);
        }
      }

      Operation *
      MaterialPropertyEditOperation::invert(ChangeList &p_ChangeList,
                                            Transaction &p_Transaction)
      {
        return new MaterialPropertyEditOperation(m_Handle, m_PropertyName,
                                                 m_NewValue, m_OldValue);
      }

    } // namespace AssetOperations
  }   // namespace Editor
} // namespace Low
