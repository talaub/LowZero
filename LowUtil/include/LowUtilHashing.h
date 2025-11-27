#pragma once

#include "LowUtilApi.h"

#include "LowMath.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    LOW_EXPORT u64 fnv1a_64(const char *p_Str);
    LOW_EXPORT u64 fnv1a_64(const void *p_Data, size_t p_Size);

    LOW_EXPORT u64 string_to_hash(const Util::String &p_HashStr);
    LOW_EXPORT Util::String hash_to_string(u64 p_Hash);

    struct U64Id
    {
      u64 val;

      U64Id() : val(0)
      {
      }

      U64Id(const u64 p_Val) : val(p_Val)
      {
      }

      U64Id(const String p_Val) : val(string_to_hash(p_Val))
      {
      }

      operator u64() const
      {
        return val;
      }
      operator String() const
      {
        return hash_to_string(val);
      }
    };
  } // namespace Util
} // namespace Low
