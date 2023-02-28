#pragma once

#include "LowUtilApi.h"

#include <stdint.h>
#include <stdlib.h>
#include <limits>

#include <tlsf.h>

namespace Low {
  namespace Util {
    namespace Memory {
      void initialize();
      void cleanup();

      struct LOW_EXPORT Allocator
      {
        Allocator()
        {
        }

        virtual ~Allocator()
        {
        }

        virtual void *allocate(uint32_t p_Size, uint32_t p_Align = 4)
        {
          return nullptr;
        }

        virtual void deallocate(void *p_Pointer)
        {
        }

        virtual void destroy()
        {
        }
      };

      struct LOW_EXPORT MallocAllocator final : public Allocator
      {
        MallocAllocator()
        {
        }

        ~MallocAllocator() final
        {
          destroy();
        }

        void *allocate(uint32_t p_Size, uint32_t p_Align = 4) final;

        void deallocate(void *p_Pointer) final;
      };

      struct LOW_EXPORT GeneralPurposeAllocator final : public Allocator
      {
        GeneralPurposeAllocator(void *p_Memory, uint32_t p_Size,
                                Allocator *p_ParentAllocator);

        ~GeneralPurposeAllocator() final
        {
          destroy();
        }

        void *allocate(uint32_t p_Size, uint32_t p_Align = 4) final;
        void deallocate(void *p_Pointer) final;
        void destroy() final;

      private:
        tlsf_t m_Allocator;
        Allocator *m_ParentAllocator;

        void *m_Buffer;
        uint32_t m_Size;
      };

      LOW_EXPORT MallocAllocator *default_malloc_allocator();
      LOW_EXPORT Allocator *default_general_purpose_allocator();
      LOW_EXPORT Allocator *main_allocator();

      template <typename T> struct MallocAllocatorProxy
      {
        typedef size_t size_type;
        typedef void *pointer;

        MallocAllocatorProxy(const char *p_Name = "")
        {
        }

        MallocAllocatorProxy(const MallocAllocatorProxy<T> &)
        {
        }

        MallocAllocatorProxy(const MallocAllocatorProxy<T> &,
                             const char *p_Name)
        {
        }

        ~MallocAllocatorProxy() throw()
        {
        }

        MallocAllocatorProxy &operator=(const MallocAllocatorProxy<T> &)
        {
          return *this;
        }

        pointer allocate(size_type n, int p_Flags = 0)
        {
          return default_malloc_allocator()->allocate((uint32_t)n * sizeof(T));
        }

        pointer allocate(size_type p_Size, size_type p_Alignment,
                         size_type p_Offset, int p_Flags = 0)
        {
          return default_malloc_allocator()->allocate((uint32_t)p_Size *
                                                      sizeof(T));
        }

        void deallocate(pointer p, size_type num)
        {
          if (p == nullptr)
            return;
          default_malloc_allocator()->deallocate(p);
        }

        const char *get_name() const
        {
          return "";
        }
        void set_name(const char *p_Name)
        {
        }
      };

      template <typename T>
      bool operator==(const MallocAllocatorProxy<T> &a,
                      const MallocAllocatorProxy<T> &b)
      {
        return true;
      }
      template <typename T>
      bool operator!=(const MallocAllocatorProxy<T> &a,
                      const MallocAllocatorProxy<T> &b)
      {
        return false;
      }

      template <typename T> struct GeneralPurposeAllocatorProxy
      {
        typedef size_t size_type;
        typedef void *pointer;

        GeneralPurposeAllocatorProxy(const char *p_Name = "")
        {
        }

        GeneralPurposeAllocatorProxy(const GeneralPurposeAllocatorProxy<T> &)
        {
        }

        GeneralPurposeAllocatorProxy(const GeneralPurposeAllocatorProxy<T> &,
                                     const char *p_Name)
        {
        }

        ~GeneralPurposeAllocatorProxy() throw()
        {
        }

        GeneralPurposeAllocatorProxy &
        operator=(const GeneralPurposeAllocatorProxy<T> &)
        {
          return *this;
        }

        pointer allocate(size_type n, int p_Flags = 0)
        {
          return default_general_purpose_allocator()->allocate((uint32_t)n *
                                                               sizeof(T));
        }

        pointer allocate(size_type p_Size, size_type p_Alignment,
                         size_type p_Offset, int p_Flags = 0)
        {
          return default_general_purpose_allocator()->allocate(
              (uint32_t)p_Size * sizeof(T), p_Alignment);
        }

        void deallocate(pointer p, size_type num)
        {
          if (p == nullptr)
            return;
          default_general_purpose_allocator()->deallocate(p);
        }

        const char *get_name() const
        {
          return "";
        }
        void set_name(const char *p_Name)
        {
        }
      };

      template <typename T>
      bool operator==(const GeneralPurposeAllocatorProxy<T> &a,
                      const GeneralPurposeAllocatorProxy<T> &b)
      {
        return true;
      }
      template <typename T>
      bool operator!=(const GeneralPurposeAllocatorProxy<T> &a,
                      const GeneralPurposeAllocatorProxy<T> &b)
      {
        return false;
      }

    } // namespace Memory
  }   // namespace Util
} // namespace Low
