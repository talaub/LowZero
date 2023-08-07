#pragma once

#include "LowEditorChangeList.h"

namespace Low {
  namespace Editor {
    namespace AssetOperations {
      struct MaterialPropertyEditOperation : public Operation
      {
        MaterialPropertyEditOperation(Util::Handle p_Handle,
                                      Util::Name p_PropertyName,
                                      Util::Variant p_OldValue,
                                      Util::Variant p_NewValue);

        void execute(ChangeList &p_ChangeList, Transaction &p_Transaction);
        Operation *invert(ChangeList &p_ChangeList, Transaction &p_Transaction);

        Util::Handle m_Handle;
        Util::Name m_PropertyName;
        Util::Variant m_OldValue;
        Util::Variant m_NewValue;
      };
    } // namespace AssetOperations
  }   // namespace Editor
} // namespace Low
