#include "LowUtilHashing.h"

namespace Low {
  namespace Util {

    constexpr u64 FNV1A_OFFSET_BASIS = 14695981039346656037ull;
    constexpr u64 FNV1A_PRIME = 1099511628211ull;

    inline u64 fnv1a_64(const void *p_Data, size_t p_Size)
    {
      const u8 *l_Bytes = static_cast<const u8 *>(p_Data);
      u64 l_Hash = FNV1A_OFFSET_BASIS;

      for (size_t i = 0; i < p_Size; ++i) {
        l_Hash ^= static_cast<u64>(l_Bytes[i]);
        l_Hash *= FNV1A_PRIME;
      }

      return l_Hash;
    }

    inline u64 fnv1a_64(const char *p_Str)
    {
      return fnv1a_64(p_Str, strlen(p_Str));
    }

    inline Util::String hash_to_string(u64 p_Hash)
    {
      char l_Buffer[17]; // 16 hex digits + null terminator
      snprintf(l_Buffer, sizeof(l_Buffer), "%016llx", p_Hash);
      return Util::String(l_Buffer);
    }

    inline u64 string_to_hash(const Util::String &p_HashStr)
    {
      return std::strtoull(p_HashStr.c_str(), nullptr, 16);
    }

  } // namespace Util
} // namespace Low
