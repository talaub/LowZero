#pragma once

#include "LowUtilApi.h"

#include "LowMath.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

namespace Low {
  namespace Util {
    LOW_EXPORT u64 fnv1a_64(const char *p_Str);
    LOW_EXPORT u64 fnv1a_64(const void *p_Data, size_t p_Size);

    LOW_EXPORT u64 string_to_hash(const Util::String &p_HashStr);
    LOW_EXPORT Util::String hash_to_string(u64 p_Hash);

    LOW_EXPORT u64 generate_unique_id();

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

    namespace Serial {
      template <> struct Converter<U64Id, void>
      {
        static Node encode(const U64Id &v)
        {
          Node n;
          String hashed = hash_to_string(v.val);
          n = hashed;
          return n;
        }
        static bool decode(const Node &n, U64Id &out)
        {
          if (auto sc = std::get_if<Node::Scalar>(&n.data)) {
            if (std::holds_alternative<String>(sc->value)) {
              String hashed = std::get<String>(sc->value);
              out.val = string_to_hash(hashed);
              return true;
            }
            if (std::holds_alternative<u64>(sc->value)) {
              out.val = std::get<u64>(sc->value);
              return true;
            }
            if (std::holds_alternative<i64>(sc->value)) {
              out.val = (u64)std::get<i64>(sc->value);
              return true;
            }
            if (std::holds_alternative<float>(sc->value)) {
              out.val = (u64)std::get<float>(sc->value);
              return true;
            }
          }
          return false;
        }
      };
    } // namespace Serial
  } // namespace Util
} // namespace Low
