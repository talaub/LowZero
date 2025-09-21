#pragma once

#include "LowUtilApi.h"

#include <atomic>
#include <stdint.h>
#include <chrono>
#include <type_traits>

#include "LowUtilName.h"
#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"
#include "LowUtilConcurrency.h"

// helpers to make a unique identifier
#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#ifdef __COUNTER__
#define UNIQUE_NAME(base) CONCAT(base, __COUNTER__)
#else
// fallback: ok unless you create two on the same source line
#define UNIQUE_NAME(base) CONCAT(base, __LINE__)
#endif

// your macro
#define LOCK_HANDLE(expr)                                            \
  Low::Util::HandleLock UNIQUE_NAME(_LOCK_GUARD_)(expr)

#define SINGLE_ARG(...) __VA_ARGS__

#define TYPE_SOA_PTR(type, member, membertype)                       \
  Low::Util::access_property<type, membertype>(                      \
      get_id(), offsetof(type::Data, member))

#define TYPE_SOA(type, member, membertype)                           \
  *Low::Util::access_property<type, membertype>(                     \
      get_id(), offsetof(type::Data, member))

#define ACCESSOR_TYPE_SOA(accessor, type, member, membertype)        \
  *Low::Util::access_property<type, membertype>(                     \
      accessor.get_id(), offsetof(type::Data, member))
#define ACCESSOR_TYPE_SOA_PTR(accessor, type, member, membertype)    \
  Low::Util::access_property<type, membertype>(                      \
      accessor.get_id(), offsetof(type::Data, member))

#if 1

#define WRITE_LOCK(lockname)                                         \
  std::unique_lock<std::shared_mutex> lockname(ms_BufferMutex)

#define READ_LOCK(lockname)                                          \
  std::shared_lock<std::shared_mutex> lockname(ms_BufferMutex)

#define LOCK_UNLOCK(lockname) lockname.unlock()

#define LOCK_PAGES_WRITE(lockname)                                   \
  Low::Util::UniqueLock<Low::Util::SharedMutex> lockname(            \
      ms_PagesMutex)

#else
#define WRITE_LOCK(lockname)

#define READ_LOCK(lockname)

#define LOCK_UNLOCK(lockname)
#endif

#define OBSERVABLE_DESTROY N(destroy)

namespace Low {
  namespace Util {
    struct Variant;
    typedef uint64_t UniqueId;
    namespace Instances {
      struct LOW_EXPORT Slot
      {
        bool m_Occupied;
        uint16_t m_Generation;
      };

      struct LOW_EXPORT Page
      {
        u8 *buffer;
        Slot *slots;
        std::atomic<u64> *lockWords;
        Mutex mutex;
        u32 size;
      };

      LOW_EXPORT void initialize_buffer(uint8_t **p_Buffer,
                                        size_t p_ElementSize,
                                        size_t p_ElementCount,
                                        Slot **p_Slots);

      LOW_EXPORT void initialize_page(Page *p_Page,
                                      const size_t p_ElementSize,
                                      const size_t p_ElementCount);

      void initialize();
      void cleanup();

    } // namespace Instances

    struct LOW_EXPORT Handle;

    namespace RTTI {
      namespace PropertyType {
        enum Enum
        {
          UNKNOWN,
          COLORRGB,
          VECTOR2,
          VECTOR3,
          QUATERNION,
          NAME,
          FLOAT,
          UINT8,
          UINT16,
          UINT32,
          UINT64,
          INT,
          BOOL,
          HANDLE,
          SHAPE,
          STRING,
          ENUM,
          VOID
        };
      }

      typedef Util::Handle *(*LivingInstancesGetter)();

      struct PropertyInfoBase
      {
        Util::Name name;
        uint32_t type;
        uint16_t handleType;
        bool editorProperty;
        void (*get)(Handle, void *);
        void (*set)(Handle, const void *);
      };

      struct VirtualPropertyInfo : public PropertyInfoBase
      {};

      struct PropertyInfo : public PropertyInfoBase
      {
        uint32_t dataOffset;
        void const *(*get_return)(Handle);

        Variant get_variant(Handle);
      };

      struct ParameterInfo
      {
        Name name;
        uint32_t type;
        uint16_t handleType;
      };

      struct FunctionInfo
      {
        Name name;
        uint32_t type;
        uint16_t handleType;
        List<ParameterInfo> parameters;
      };

      struct TypeInfo
      {
        Name name;
        uint16_t typeId;
        bool component;
        bool uiComponent;
        Map<Name, PropertyInfo> properties;
        Map<Name, VirtualPropertyInfo> virtualProperties;
        Map<Name, FunctionInfo> functions;
        uint32_t (*get_capacity)();
        bool (*is_alive)(Handle);
        void (*serialize)(Handle, Yaml::Node &);
        Handle (*deserialize)(Yaml::Node &, Handle);
        Handle (*duplicate_default)(Handle, Name);
        Handle (*duplicate_component)(Handle, Handle);
        Handle (*make_default)(Name);
        Handle (*make_component)(Handle);
        Handle (*find_by_index)(u32);
        Handle (*find_by_name)(Name);
        void (*destroy)(Handle);
        LivingInstancesGetter get_living_instances;
        uint32_t (*get_living_count)();
        void (*notify)(Handle, Handle, Name);
      };

      struct EnumEntryInfo
      {
        Name name;
        u8 value;
      };

      struct EnumInfo
      {
        Name name;
        u16 enumId;

        List<EnumEntryInfo> entries;

        Name (*entry_name)(u8);
        u8 (*entry_value)(Name);
      };
    } // namespace RTTI

    struct LOW_EXPORT Handle
    {
      union
      {
        uint64_t m_Id;

        struct
        {
          uint32_t m_Index;
          uint16_t m_Generation;
          uint16_t m_Type;
        } m_Data;
      };

      Handle();
      Handle(uint64_t p_Id);

      bool operator==(const Handle &p_Other) const;
      bool operator!=(const Handle &p_Other) const;
      bool operator<(const Handle &p_Other) const;

      operator uint64_t() const;

      uint64_t get_id() const;

      uint32_t get_index() const;
      uint16_t get_generation() const;
      uint16_t get_type() const;

      bool is_registered_type() const;

      static bool is_registered_type(u16 p_TypeId);
      static RTTI::TypeInfo &get_type_info(uint16_t p_TypeId);
      static List<uint16_t> &get_component_types();

      static void
      fill_variants(Util::Handle p_Handle,
                    Util::RTTI::PropertyInfo &p_PropertyInfo,
                    Util::Map<Util::Name, Util::Variant> &p_Variants);

      const static u64 DEAD;

    protected:
      static void register_type_info(uint16_t p_TypeId,
                                     RTTI::TypeInfo &p_TypeInfo);
    };

    void LOW_EXPORT register_enum_info(u16 p_EnumId,
                                       RTTI::EnumInfo &p_EnumInfo);
    LOW_EXPORT RTTI::EnumInfo &get_enum_info(u16 p_EnumId);
    LOW_EXPORT List<u16> &get_enum_ids();

    UniqueId LOW_EXPORT generate_unique_id(Handle p_Handle);
    void LOW_EXPORT register_unique_id(UniqueId p_UniqueId,
                                       Handle p_Handle);
    void LOW_EXPORT remove_unique_id(UniqueId p_UniqueId);

    Handle LOW_EXPORT find_handle_by_unique_id(UniqueId p_UniqueId);

    template <typename T> class HandleLock
    {
    public:
      HandleLock() = default;
      explicit HandleLock(const T p_Handle, bool p_Wait = true)
      {
        lock(p_Handle, p_Wait);
      }

      HandleLock(const HandleLock &) = delete;
      HandleLock &operator=(const HandleLock &) = delete;

      HandleLock(HandleLock &&p_Rhs) noexcept
      {
        move_from(std::move(p_Rhs));
      }

      HandleLock &operator=(HandleLock &&p_Rhs) noexcept
      {
        if (this != &p_Rhs) {
          unlock();
          move_from(std::move(p_Rhs));
        }

        return *this;
      }

      ~HandleLock()
      {
        unlock();
      }

    private:
      static inline u64 pack(u32 tid, u32 count)
      {
        return (u64(count) << 32) | u64(tid);
      }
      static inline u32 owner_tid(u64 w)
      {
        return u32(w & 0xFFFF'FFFFull);
      }
      static inline u32 rec_count(u64 w)
      {
        return u32(w >> 32);
      }

      static u32 current_thread_id()
      {
        // Portable way to get a stable 32-bit id out of
        // std::thread::id
        auto id = std::this_thread::get_id();
        std::hash<std::thread::id> h;
        u64 v = static_cast<u64>(h(id));
        // Fold to 32 bits in case hash is 64-bit
        return static_cast<u32>(v ^ (v >> 32));
      }

    public:
      void lock(const T p_Handle, bool p_Wait = true)
      {
        if (m_Locked) {
          return;
        }

        SharedLock<SharedMutex> l_PagesLock(T::ms_PagesMutex);

        u32 l_PageIndex = 0, l_SlotIndex = 0;

        if (!T::get_page_for_index(p_Handle.get_index(), l_PageIndex,
                                   l_SlotIndex)) {
          return;
        }

        Instances::Page *l_Page = T::ms_Pages[l_PageIndex];

        UniqueLock<Mutex> l_PageLock(l_Page->mutex);

        // TODO: DO MANUAL ALIVE CHECK
        if (!l_Page->slots[l_SlotIndex].m_Occupied) {
          return;
        }

        std::atomic<u64> &l_Word = l_Page->lockWords[l_SlotIndex];
        const u32 l_Tid = current_thread_id();

        u64 l_Cur = l_Word.load(std::memory_order_acquire);

        if (owner_tid(l_Cur) == l_Tid) {
          l_Word.fetch_add(u64(1) << 32, std::memory_order_acq_rel);
          remember(p_Handle, &l_Word);

          m_Locked = true;
          return;
        }

        if (l_Cur == 0) {
          if (l_Word.compare_exchange_strong(
                  l_Cur, pack(l_Tid, 1), std::memory_order_acq_rel,
                  std::memory_order_acquire)) {
            remember(p_Handle, &l_Word);
            m_Locked = true;
            return;
          }
        }

        if (!p_Wait) {
          return;
        }

        l_PageLock.unlock();
        backoff_wait_until(l_Word, l_Tid);
        l_PageLock.lock();

        // TODO: alive_check
        if (!l_Page->slots[l_SlotIndex].m_Occupied) {
          return;
        }

        for (;;) {
          l_Cur = l_Word.load(std::memory_order_acquire);
          if (l_Cur == 0) {
            if (l_Word.compare_exchange_weak(
                    l_Cur, pack(l_Tid, 1), std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
              break;
            }
          } else if (owner_tid(l_Cur) == l_Tid) {
            l_Word.fetch_add(u64(1) << 32, std::memory_order_acq_rel);
            break;
          } else {
            l_PageLock.unlock();
            backoff_wait_until(l_Word, l_Tid);
            l_PageLock.lock();
            if (!l_Page->slots[l_SlotIndex].m_Occupied) {
              return;
            }
          }
        }

        remember(p_Handle, &l_Word);
        m_Locked = true;
      }

      void unlock()
      {
        if (!m_Locked)
          return;
        if (!m_Word) {
          m_Locked = false;
          return;
        }

        const u32 l_Tid = current_tid();
        u64 l_Cur = m_Word->load(std::memory_order_acquire);
        for (;;) {
          if (owner_tid(l_Cur) != l_Tid) {
            break;
          }
          const u32 i_Count = rec_count(l_Cur);
          if (i_Count > 1) {
            u64 i_Desired = l_Cur - (u64(1) << 32);
            if (m_Word->compare_exchange_weak(
                    l_Cur, i_Desired, std::memory_order_acq_rel,
                    std::memory_order_acquire))
              break;
          } else {
            if (m_Word->compare_exchange_weak(
                    l_Cur, 0, std::memory_order_acq_rel,
                    std::memory_order_acquire))
              break;
          }
        }

        m_Word = nullptr;
        m_Handle = Handle::DEAD;
        m_Locked = false;
      }

      bool owns_lock() const
      {
        return m_Locked;
      }

    private:
      static u32 current_tid()
      {
        std::hash<std::thread::id> h;
        auto v = h(std::this_thread::get_id());
#if INTPTR_MAX == INT64_MAX
        return u32((u64)v ^ (u64(v) >> 32));
#else
        return u32(v);
#endif
      }

      static void backoff_wait_until(std::atomic<u64> &word, u32 tid)
      {
        using namespace std::chrono_literals;
        int spins = 0;
        for (;;) {
          u64 cur = word.load(std::memory_order_acquire);
          if (cur == 0 || owner_tid(cur) == tid)
            return;
          if (spins < 64) {
            ++spins;
            std::this_thread::yield();
          } else {
            spins = 0;
            std::this_thread::sleep_for(100us);
          }
        }
      }

      void remember(const T p_Handle, std::atomic<u64> *p_Word)
      {
        m_Handle = p_Handle;
        m_Word = p_Word;
      }
      void move_from(HandleLock &&o)
      {
        m_Handle = o.m_Handle;
        o.m_Handle = Handle::DEAD;
        m_Word = o.m_Word;
        o.m_Word = nullptr;
        m_Locked = o.m_Locked;
        o.m_Locked = false;
      }

      T m_Handle = Handle::DEAD;
      std::atomic<u64> *m_Word{nullptr};
      bool m_Locked{false};
    };

    template <class T, class M>
    inline M *access_property(T p_Handle, const size_t p_MemberOffset)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(T::get_page_for_index(p_Handle.m_Data.m_Index,
                                        l_PageIndex, l_SlotIndex));

      /*
      LOW_LOG_DEBUG << "ACCESS index: " << p_Handle.m_Data.m_Index
                    << " got page: " << l_PageIndex
                    << " and slot: " << l_SlotIndex << LOW_LOG_END;
                    */
      Low::Util::Instances::Page *l_Page = T::ms_Pages[l_PageIndex];
      /*
      LOW_LOG_DEBUG << "Memberoffset: " << p_MemberOffset
                    << LOW_LOG_END;
      LOW_LOG_DEBUG << "Membersize: " << sizeof(M) << LOW_LOG_END;
      LOW_LOG_DEBUG << "PageSize: " << l_Page->size << LOW_LOG_END;
      LOW_LOG_DEBUG << "Bufferoffset: "
                    << (p_MemberOffset * l_Page->size) +
                           (l_SlotIndex * sizeof(M))
                    << LOW_LOG_END;
      LOW_LOG_DEBUG << "-------------------" << LOW_LOG_END;
      */
      return (M *)&(l_Page->buffer[(p_MemberOffset * l_Page->size) +
                                   (l_SlotIndex * sizeof(M))]);
    }
  } // namespace Util
} // namespace Low
