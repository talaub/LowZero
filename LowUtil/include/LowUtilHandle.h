#pragma once

#include "LowUtilApi.h"

#include <stdint.h>

#include "LowUtilName.h"
#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#define SINGLE_ARG(...) __VA_ARGS__

#define TYPE_SOA_PTR(type, member, membertype)                       \
  ((membertype *)&(                                                  \
      ms_Buffer[offsetof(##type##Data, member) *                     \
                    type::get_capacity() +                           \
                (m_Data.m_Index * sizeof(membertype))]))

#define TYPE_SOA(type, member, membertype)                           \
  *((membertype *)&(                                                 \
      ms_Buffer[offsetof(##type##Data, member) *                     \
                    type::get_capacity() +                           \
                (m_Data.m_Index * sizeof(membertype))]))

#define ACCESSOR_TYPE_SOA(accessor, type, member, membertype)        \
  *((membertype *)&(                                                 \
      ms_Buffer[offsetof(##type##Data, member) *                     \
                    type::get_capacity() +                           \
                (accessor.m_Data.m_Index * sizeof(membertype))]))

#if 1

#define WRITE_LOCK(lockname)                                         \
  std::unique_lock<std::shared_mutex> lockname(ms_BufferMutex)

#define READ_LOCK(lockname)                                          \
  std::shared_lock<std::shared_mutex> lockname(ms_BufferMutex)

#define LOCK_UNLOCK(lockname) lockname.unlock()

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

      LOW_EXPORT void initialize_buffer(uint8_t **p_Buffer,
                                        size_t p_ElementSize,
                                        size_t p_ElementCount,
                                        Slot **p_Slots);

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

      bool check_alive(Instances::Slot *p_Slots,
                       uint32_t p_Capacity) const;

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
  } // namespace Util
} // namespace Low
