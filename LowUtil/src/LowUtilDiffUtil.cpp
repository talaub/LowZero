#include "LowUtilDiffUtil.h"

#include "LowUtilLogger.h"

namespace Low {
  namespace Util {
    namespace DiffUtil {
      void store_handle(StoredHandle &p_StoredHandle, Handle p_Handle)
      {
        p_StoredHandle.typeId = p_Handle.get_type();

        RTTI::TypeInfo &l_TypeInfo =
            Handle::get_type_info(p_StoredHandle.typeId);

        for (auto it = l_TypeInfo.properties.begin();
             it != l_TypeInfo.properties.end(); ++it) {
          Handle::fill_variants(p_Handle, it->second,
                                p_StoredHandle.properties);
        }
      }

      bool diff(StoredHandle &p_H1, StoredHandle &p_H2,
                Util::List<Util::Name> &p_Diff)
      {
        LOW_ASSERT(p_H1.typeId == p_H2.typeId,
                   "Stored handles don't have the same type");

        RTTI::TypeInfo &l_TypeInfo = Handle::get_type_info(p_H1.typeId);

        for (auto it = l_TypeInfo.properties.begin();
             it != l_TypeInfo.properties.end(); ++it) {
          if (p_H1.properties[it->first] != p_H2.properties[it->first]) {
            p_Diff.push_back(it->first);
          }
        }

        return !p_Diff.empty();
      }
    } // namespace DiffUtil
  }   // namespace Util
} // namespace Low
