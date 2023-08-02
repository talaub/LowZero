#pragma once

#include "LowUtilHandle.h"
#include "LowUtilContainers.h"
#include "LowUtilDiffUtil.h"

namespace Low {
  namespace Editor {
    struct ChangeList;
    struct Transaction;

    struct Operation
    {
      virtual void execute(ChangeList &p_ChangeList, Transaction &p_Transaction)
      {
      }

      virtual Operation *invert(ChangeList &p_ChangeList,
                                Transaction &p_Transaction)
      {
        return 0;
      }
    };

    struct Transaction
    {
      Transaction(Util::String p_Title);

      Transaction &Transaction::add_operation(Operation *p_Operation);
      Util::List<Operation *> &Transaction::get_operations();

      void execute(ChangeList &p_ChangeList);
      Transaction invert(ChangeList &p_ChangeList) const;

      bool empty() const;
      void cleanup();

      static Transaction from_diff(Util::Handle p_Handle,
                                   Util::StoredHandle &p_H1,
                                   Util::StoredHandle &p_H2);

      Util::String m_Title;

    protected:
      Util::List<Operation *> m_Operations;
    };

    struct ChangeList
    {
      ChangeList();
      void undo();
      void redo();

      void add_entry(Transaction p_Transaction);

      bool has_mapping(Util::Handle p_Handle);
      Util::Handle get_mapping(Util::Handle p_Handle);
      void remove_mapping(Util::Handle p_Handle);
      void set_mapping(Util::Handle p_From, Util::Handle p_To);

      Transaction &peek();
      Transaction pop();

      Util::List<Transaction> m_Changelist;
      int m_ChangePointer;

    protected:
      Util::Map<Util::Handle, Util::Handle> m_Mappings;
    };
  } // namespace Editor
} // namespace Low
