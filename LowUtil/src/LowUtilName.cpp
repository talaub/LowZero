#include "LowUtilName.h"

#include "LowUtilContainers.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilMemory.h"

#include <stdlib.h>
#include <mutex>

// TODO TL: Define kilobyte macro
#define MAX_BUFFER_SIZE (50 * 1024)

namespace Low {
  namespace Util {
    char *g_StringBuffer;
    char *g_StringPointer;
    std::mutex g_Mutex;

    Map<uint32_t, char *> g_NameMap;

    static bool buffer_contains_name(const uint32_t p_Index)
    {
      return g_NameMap.find(p_Index) != g_NameMap.end();
    }

    bool Name::operator==(const Name &p_Other) const
    {
      return m_Index == p_Other.m_Index;
    }

    bool Name::operator!=(const Name &p_Other) const
    {
      return m_Index != p_Other.m_Index;
    }

    bool Name::operator<(const Name &p_Other) const
    {
      return m_Index < p_Other.m_Index;
    }

    bool Name::operator>(const Name &p_Other) const
    {
      return m_Index > p_Other.m_Index;
    }

    Name &Name::operator=(const Name p_Other)
    {
      m_Index = p_Other.m_Index;

      return *this;
    }

    bool Name::is_valid() const
    {
      return buffer_contains_name(m_Index);
    }

    char *Name::c_str() const
    {
      LOW_ASSERT(is_valid(), "Name not found");

      return g_NameMap[m_Index];
    }

    void Name::initialize()
    {
      g_StringBuffer =
          (char *)Memory::main_allocator()->allocate(MAX_BUFFER_SIZE);
      LOW_PROFILE_ALLOC(Name String Buffer);
      g_StringPointer = g_StringBuffer;

      LOW_LOG_DEBUG << "Name buffer setup completed" << LOW_LOG_END;
    }

    void Name::cleanup()
    {
      Memory::main_allocator()->deallocate(g_StringBuffer);
      LOW_PROFILE_FREE(Name String Buffer);

      LOW_LOG_DEBUG << "Cleaned up Name buffer" << LOW_LOG_END;
    }

    uint32_t Name::to_hash(const char *p_String)
    {
      int i, j;
      uint32_t byte, crc, mask;

      i = 0;
      crc = 0xFFFFFFFF;
      while (p_String[i] != 0) {
        byte = p_String[i];
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--) {
          mask = -(crc & 1);
          crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
      }
      return ~crc;
    }

    static void add_name_to_buffer(const uint32_t p_Index,
                                   const char *p_String)
    {
      if (buffer_contains_name(p_Index)) {
        return;
      }
      // g_Mutex.lock();

      uint32_t l_Length = static_cast<uint32_t>(strlen(p_String));

      LOW_ASSERT(g_StringPointer - g_StringBuffer + l_Length + 1 <
                     MAX_BUFFER_SIZE,
                 "Ran out of Name memory");

      void *l_Pointer = memcpy(g_StringPointer, p_String, l_Length);
      g_StringPointer[l_Length] = '\0';

      g_NameMap[p_Index] = (char *)l_Pointer;
      g_StringPointer = &g_StringPointer[l_Length + 1];

      // g_Mutex.unlock();
    }

    Name::Name() : m_Index(0u)
    {
    }

    Name::Name(uint32_t p_Index) : m_Index(p_Index)
    {
    }

    Name::Name(const char *p_Name)
    {
      m_Index = to_hash(p_Name);
      add_name_to_buffer(m_Index, p_Name);
    }

    Name::Name(const Name &p_Name)
    {
      m_Index = p_Name.m_Index;
    }

  } // namespace Util
} // namespace Low

#undef MAX_BUFFER_SIZE
