#include "LowUtilMemory.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include "LowMath.h"

namespace Low {
  namespace Util {
    namespace Memory {
      MallocAllocator *g_DefaultMallocAllocator = nullptr;
      GeneralPurposeAllocator *g_DefaultGeneralPurposeAllocator = nullptr;

      void initialize()
      {
        if (!g_DefaultMallocAllocator) {
          g_DefaultMallocAllocator = new MallocAllocator();
        }
        if (!g_DefaultGeneralPurposeAllocator) {
          g_DefaultGeneralPurposeAllocator = new GeneralPurposeAllocator(
              default_malloc_allocator()->allocate(500 * LOW_MEGABYTE_I),
              500 * LOW_MEGABYTE_I, default_malloc_allocator());
        }
      }

      void cleanup()
      {
        delete g_DefaultGeneralPurposeAllocator;
        delete g_DefaultMallocAllocator;
      }

      MallocAllocator *default_malloc_allocator()
      {
        if (!g_DefaultMallocAllocator) {
          initialize();
        }
        return g_DefaultMallocAllocator;
      }

      Allocator *default_general_purpose_allocator()
      {
        if (!g_DefaultGeneralPurposeAllocator) {
          initialize();
        }
        return g_DefaultGeneralPurposeAllocator;
      }

      Allocator *main_allocator()
      {
        return default_general_purpose_allocator();
      }

      void *MallocAllocator::allocate(uint32_t p_Size, uint32_t p_Align)
      {
        return malloc(p_Size);
      }

      void MallocAllocator::deallocate(void *p_Pointer)
      {
        free(p_Pointer);
      }

      GeneralPurposeAllocator::GeneralPurposeAllocator(
          void *p_Memory, uint32_t p_Size, Allocator *p_ParentAllocator)
      {
        LOW_ASSERT(p_Size > 0,
                   "Cannot create general purpose allocator with no size");
        m_Buffer = p_Memory;
        m_ParentAllocator = p_ParentAllocator;
        m_Allocator = tlsf_create_with_pool(m_Buffer, p_Size);
      }

      void *GeneralPurposeAllocator::allocate(uint32_t p_Size, uint32_t p_Align)
      {
        void *l_Memory = tlsf_memalign(m_Allocator, p_Align, p_Size);
        LOW_ASSERT(l_Memory, "Failed to allocate memory");

        return l_Memory;
      }

      void GeneralPurposeAllocator::deallocate(void *p_Pointer)
      {
        tlsf_free(m_Allocator, p_Pointer);
      }

      void GeneralPurposeAllocator::destroy()
      {
        tlsf_destroy(m_Allocator);
        m_ParentAllocator->deallocate(m_Buffer);
      }
    } // namespace Memory
  }   // namespace Util
} // namespace Low
