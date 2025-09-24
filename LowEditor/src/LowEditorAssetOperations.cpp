#include "LowEditorAssetOperations.h"

#include "LowEditorMainWindow.h"

#include "LowUtilLogger.h"

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

        Renderer::Material l_Material = m_Handle.get_id();
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
