#pragma once

#include "LowUtilApi.h"

#include <stdint.h>

#define LOW_NAME(x) Low::Util::Name(x)
#define N(x) Low::Util::Name(#x)

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
      bool is_valid() const;

      static void initialize();
      static void cleanup();

      static uint32_t to_hash(const char *p_String);

      uint32_t m_Index;
    };

  } // namespace Util
} // namespace Low
