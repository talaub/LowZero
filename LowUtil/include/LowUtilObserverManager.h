#pragma once

#include "LowUtilApi.h"

#include "LowMath.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Util {
    struct LOW_EXPORT ObserverKey
    {
      u64 handleId;
      u32 observableName;
      bool operator==(const ObserverKey &o) const
      {
        return handleId == o.handleId &&
               observableName == o.observableName;
      }
    };

    struct LOW_EXPORT ObserverKeyHash
    {
      size_t operator()(const ObserverKey &k) const noexcept
      {
        return std::hash<uint64_t>()(k.handleId) ^
               (std::hash<uint32_t>()(k.observableName) << 1);
      }
    };

    u64 LOW_EXPORT observe(const ObserverKey &key, Handle);
    u64 LOW_EXPORT observe(
        const ObserverKey &key,
        Util::Function<void(Util::Handle, Util::Name)> p_Function);

    void LOW_EXPORT notify(const ObserverKey &key);

    void LOW_EXPORT clear(const ObserverKey &key);
  } // namespace Util
} // namespace Low
