#pragma once

#include "LowUtilApi.h"

#include "LowMath.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    inline LOW_EXPORT u64 fnv1a_64(const char *p_Str);
    inline LOW_EXPORT u64 fnv1a_64(const void *p_Data, size_t p_Size);

    inline LOW_EXPORT u64
    string_to_hash(const Util::String &p_HashStr);
    inline LOW_EXPORT Util::String hash_to_string(u64 p_Hash);
  } // namespace Util
} // namespace Low
