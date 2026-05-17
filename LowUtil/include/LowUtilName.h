#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

#include <stdint.h>

#define LOW_NAME(x) Low::Util::Name(x)
#define N(x) Low::Util::Name(#x)
#define _LNAME(x) Low::Util::Name(#x)

namespace Low {
  namespace Util {

    struct LOW_EXPORT Name
    {
      Name(const char *p_Name);
      Name(const Name &p_Name);
      Name(uint32_t p_Index);
      Name();

      bool operator==(const Name &p_Other) const;
      bool operator!=(const Name &p_Other) const;
      bool operator<(const Name &p_Other) const;
      bool operator>(const Name &p_Other) const;
      Name &operator=(const Name p_Other);

      char *c_str() const;
      const char *debug_c_str() const;
      bool is_valid() const;

      static void initialize();
      static void cleanup();

      static uint32_t to_hash(const char *p_String);
      static const char *debug_string_or_null(uint32_t p_Index);
      static const char *debug_string(uint32_t p_Index);

      static Name from_string(String p_String);

      uint32_t m_Index;
    };

  } // namespace Util
} // namespace Low

namespace eastl {
  template <> struct hash<Low::Util::Name>
  {
    size_t operator()(const Low::Util::Name &p_Name) const noexcept
    {
      return static_cast<size_t>(p_Name.m_Index);
    }
  };
} // namespace eastl
