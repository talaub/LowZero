#include "LowEditorChangeList.h"

#include "LowEditorCommonOperations.h"

#include "LowUtilLogger.h"

#define CHANGELIST_MAX_SIZE 15

namespace Low {
  namespace Editor {
    Transaction::Transaction(Util::String p_Title) : m_Title(p_Title)
    {
    }

    Util::List<Operation *> &Transaction::get_operations()
    {
      return m_Operations;
    }

    bool Transaction::empty() const
    {
      return m_Operations.empty();
    }

    void Transaction::cleanup()
    {
      // Deletes all of the operations in this transaction
      for (auto it = m_Operations.begin();
           it != m_Operations.end();) {
        Operation *op = *it;
        it = m_Operations.erase(it);
        delete op;
      }
    }

    void Transaction::execute(ChangeList &p_ChangeTracker)
    {
      // Executes all of the operations in this transaction one by one
      for (auto it = m_Operations.begin(); it != m_Operations.end();
           ++it) {
        (*it)->execute(p_ChangeTracker, *this);
      }
    }

    Transaction Transaction::invert(ChangeList &p_ChangeTracker) const
    {
      Transaction invertedTransaction("TempInverted");
      for (int i = m_Operations.size() - 1; i >= 0; --i) {
        invertedTransaction.add_operation(m_Operations[i]->invert(
            p_ChangeTracker, invertedTransaction));
      }

      return invertedTransaction;
    }

    Transaction &Transaction::add_operation(Operation *p_Operation)
    {
      // Adds an operation to the current transaction
      // Works corresponding to the builder pattern to it returns
      // itself
      m_Operations.push_back(p_Operation);
      return *this;
    }

    Transaction Transaction::from_diff(Util::Handle p_Handle,
                                       Util::StoredHandle &p_H1,
                                       Util::StoredHandle &p_H2)
    {
      Util::List<Util::Name> l_ChangedProperties;
      Util::DiffUtil::diff(p_H1, p_H2, l_ChangedProperties);

      Util::List<Util::Name> l_ChangedEditorProperties;

      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_H1.typeId);

      for (auto it = l_ChangedProperties.begin();
           it != l_ChangedProperties.end(); ++it) {
        if (l_TypeInfo.properties[*it].editorProperty) {
          l_ChangedEditorProperties.push_back(*it);
        }
      }

      if (l_ChangedEditorProperties.empty()) {
        return Transaction("");
      }

      Util::String l_Title = "Edit ";
      if (l_ChangedEditorProperties.size() == 1) {
        l_Title += l_ChangedEditorProperties[0].c_str();
      } else {
        l_Title += "multiple properties";
      }

      l_Title += " on ";

      l_Title += l_TypeInfo.name.c_str();

      Transaction l_Transaction(l_Title);

      for (auto it = l_ChangedEditorProperties.begin();
           it != l_ChangedEditorProperties.end(); ++it) {

        l_Transaction.add_operation(
            new CommonOperations::PropertyEditOperation(
                p_Handle, *it, p_H1.properties[*it],
                p_H2.properties[*it]));
      }

      return l_Transaction;
    }

    ChangeList::ChangeList() : m_ChangePointer(-1)
    {
    }

    void ChangeList::add_entry(Transaction p_Transaction)
    {
      if (p_Transaction.empty()) {
        return;
      }

      if (m_ChangePointer != m_Changelist.size() - 1) {
        // If the current pointer is not at the end of the changelist,
        // that means that we have to delete all the changes that
        // happened since then before pushing the new change
        uint32_t l_DeletePointer = m_Changelist.size() - 1;
        while (l_DeletePointer > m_ChangePointer) {
          m_Changelist.back().cleanup();
          m_Changelist.pop_back();
          l_DeletePointer = m_Changelist.size() - 1;
        }
      } else if (m_Changelist.size() == CHANGELIST_MAX_SIZE) {
        Util::List<Transaction> l_NewChangelist;
        for (int i = 1; i < m_Changelist.size(); ++i) {
          l_NewChangelist.push_back(m_Changelist[i]);
        }
        m_Changelist = l_NewChangelist;
      }

      // Push the new entry to the changelist and adjust the
      // changepointer to now point to the index of the newest entry
      m_Changelist.push_back(p_Transaction);
      m_ChangePointer = m_Changelist.size() - 1;
    }

    bool ChangeList::has_mapping(Util::Handle p_Handle)
    {
      return m_Mappings.find(p_Handle) != m_Mappings.end();
    }

    void ChangeList::remove_mapping(Util::Handle p_Handle)
    {
      auto l_Pos = m_Mappings.find(p_Handle);

      if (l_Pos != m_Mappings.end()) {
        m_Mappings.erase(l_Pos);
      }
    }

    void ChangeList::set_mapping(Util::Handle p_From,
                                 Util::Handle p_To)
    {
      if (p_From != p_To) {
        m_Mappings[p_From] = p_To;
      }
    }

    Util::Handle ChangeList::get_mapping(Util::Handle p_Handle)
    {
      Util::Handle l_Handle = p_Handle;
      auto l_Pos = m_Mappings.find(p_Handle);

      while (l_Pos != m_Mappings.end()) {
        l_Handle = l_Pos->second;
        l_Pos = m_Mappings.find(l_Handle);
      }
      return l_Handle;
    }

    Transaction &ChangeList::peek()
    {
      return m_Changelist[m_ChangePointer];
    }

    Transaction ChangeList::pop()
    {
      Transaction l_Transaction = peek();
      m_Changelist.erase(m_Changelist.begin() + m_ChangePointer);
      m_ChangePointer--;
      return l_Transaction;
    }

    void ChangeList::undo()
    {
      if (m_ChangePointer < 0) {
        // Undo only works if there is an entry that could be undone
        // So if this happens there are no actions left to undo
        return;
      }
      // Inverts the entry the changepointer currently points to and
      // executes it
      Transaction l_InvertedTransaction =
          m_Changelist[m_ChangePointer].invert(*this);
      l_InvertedTransaction.execute(*this);
      // The inverted transaction is only temporary so it needs  to be
      // cleaned up
      l_InvertedTransaction.cleanup();
      // Decrease the changepointer to it points to the previous entry
      m_ChangePointer--;
    }

    void ChangeList::redo()
    {
      if (m_ChangePointer == m_Changelist.size() - 1) {
        // We are already pointing to the newest entry in the
        // changelist, so there is nothing left to redo
        return;
      }

      // Move the changepointer to the next entry and execute it
      m_ChangePointer++;
      m_Changelist[m_ChangePointer].execute(*this);
    }
  } // namespace Editor
} // namespace Low

#undef CHANGELIST_MAX_SIZE
