#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"
#include "LowUtilName.h"
#include "LowUtilVariant.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Util {
    struct StoredHandle
    {
      Util::Map<Util::Name, Util::Variant> properties;
      uint16_t typeId;
    };

    namespace DiffUtil {
      void LOW_EXPORT store_handle(StoredHandle &p_StoredHandle,
                                   Handle p_Handle);

      bool LOW_EXPORT diff(StoredHandle &p_H1, StoredHandle &p_H2,
                           Util::List<Util::Name> &p_Diff);
    } // namespace DiffUtil
  }   // namespace Util
} // namespace Low
