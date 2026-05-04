#include "LowUtilHashing.h"
#include "LowUtilHandle.h"
#include "LowUtilContainers.h"

#include <inttypes.h>
#include <atomic>
#include <chrono>
#include <random>

namespace Low {
  namespace Util {

    constexpr u64 FNV1A_OFFSET_BASIS = 14695981039346656037ull;
    constexpr u64 FNV1A_PRIME = 1099511628211ull;

    u64 fnv1a_64(const void *p_Data, size_t p_Size)
    {
      const u8 *l_Bytes = static_cast<const u8 *>(p_Data);
      u64 l_Hash = FNV1A_OFFSET_BASIS;

      for (size_t i = 0; i < p_Size; ++i) {
        l_Hash ^= static_cast<u64>(l_Bytes[i]);
        l_Hash *= FNV1A_PRIME;
      }

      return l_Hash;
    }

    u64 fnv1a_64(const char *p_Str)
    {
      return fnv1a_64(p_Str, strlen(p_Str));
    }

    Util::String hash_to_string(u64 p_Hash)
    {
      char l_Buffer[17]; // 16 hex digits + null terminator
      snprintf(l_Buffer, sizeof l_Buffer, "%016" PRIx64,
               (uint64_t)p_Hash);
      return Util::String(l_Buffer);
    }

    u64 string_to_hash(const Util::String &p_HashStr)
    {
      return std::strtoull(p_HashStr.c_str(), nullptr, 16);
    }

    u64 generate_unique_id()
    {
      static std::atomic<u64> s_Counter = 0;
      static std::mt19937_64 s_Rng(
          (u64)std::chrono::high_resolution_clock::now()
              .time_since_epoch()
              .count());

      const u64 l_Time =
          (u64)std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();
      const u64 l_Counter =
          s_Counter.fetch_add(1, std::memory_order_relaxed);
      const u64 l_Random = s_Rng();

      u64 l_Value = l_Time;
      l_Value ^= l_Counter + 0x9e3779b97f4a7c15ull + (l_Value << 6) +
                 (l_Value >> 2);
      l_Value ^= l_Random + 0x9e3779b97f4a7c15ull + (l_Value << 6) +
                 (l_Value >> 2);

      l_Value &= ~(1ull << 63);

      return l_Value;
    }

    u64 make_fixed_unique_id(const char *p_Name)
    {
      return fnv1a_64(p_Name) | (1ull << 63);
    }

  } // namespace Util
} // namespace Low
