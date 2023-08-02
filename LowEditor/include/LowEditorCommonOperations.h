#pragma once

#include "LowEditorChangeList.h"

namespace Low {
  namespace Editor {
    namespace CommonOperations {
      struct PropertyEditOperation : public Operation
      {
        PropertyEditOperation(Util::Handle p_Handle, Util::Name p_PropertyName,
                              Util::Variant p_OldValue,
                              Util::Variant p_NewValue);

        void execute(ChangeList &p_ChangeList, Transaction &p_Transaction);
        Operation *invert(ChangeList &p_ChangeList, Transaction &p_Transaction);

        Util::Handle m_Handle;
        Util::Name m_PropertyName;
        Util::Variant m_OldValue;
        Util::Variant m_NewValue;
      };

      struct HandleCreateOperation : public Operation
      {
        HandleCreateOperation(Util::Handle p_Handle);
        HandleCreateOperation(Util::Handle p_Handle,
                              Util::Yaml::Node &p_SerializedHandle);

        void execute(ChangeList &p_ChangeList, Transaction &p_Transaction);
        Operation *invert(ChangeList &p_ChangeList, Transaction &p_Transaction);

        Util::Handle m_Handle;
        Util::Yaml::Node m_SerializedHandle;
      };

      struct HandleDestroyOperation : public Operation
      {
        HandleDestroyOperation(Util::Handle p_Handle);

        void execute(ChangeList &p_ChangeList, Transaction &p_Transaction);
        Operation *invert(ChangeList &p_ChangeList, Transaction &p_Transaction);

        Util::Handle m_Handle;
        Util::Yaml::Node m_SerializedHandle;
      };
    } // namespace CommonOperations
  }   // namespace Editor
} // namespace Low
