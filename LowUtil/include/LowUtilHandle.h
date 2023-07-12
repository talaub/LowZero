#pragma once

#include "LowUtilApi.h"

#include <stdint.h>

#include "LowUtilName.h"
#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#define SINGLE_ARG(...) __VA_ARGS__

#define TYPE_SOA(type, member, membertype)                                     \
  *((membertype *)&(                                                           \
      ms_Buffer[offsetof(##type##Data, member) * type::get_capacity() +        \
                (m_Data.m_Index * sizeof(membertype))]))

#define ACCESSOR_TYPE_SOA(accessor, type, member, membertype)                  \
  *((membertype *)&(                                                           \
      ms_Buffer[offsetof(##type##Data, member) * type::get_capacity() +        \
                (accessor.m_Data.m_Index * sizeof(membertype))]))

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
                                        size_t p_ElementCount, Slot **p_Slots);

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
          UINT32,
          UINT64,
          INT,
          BOOL,
          HANDLE,
          SHAPE
        };
      }

      typedef Util::Handle *(*LivingInstancesGetter)();

      struct LOW_EXPORT PropertyInfo
      {
        Util::Name name;
        uint32_t dataOffset;
        uint32_t type;
        uint16_t handleType;
        bool editorProperty;
        void const *(*get)(Handle);
        void (*set)(Handle, const void *);

        Variant get_variant(Handle);
      };

      struct TypeInfo
      {
        Name name;
        bool component;
        Map<Name, PropertyInfo> properties;
        uint32_t (*get_capacity)();
        bool (*is_alive)(Handle);
        void (*serialize)(Handle, Yaml::Node &);
        Handle (*deserialize)(Yaml::Node &, Handle);
        Handle (*make_default)(Name);
        Handle (*make_component)(Handle);
        void (*destroy)(Handle);
        LivingInstancesGetter get_living_instances;
        uint32_t (*get_living_count)();
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

      uint64_t get_id() const;

      uint32_t get_index() const;
      uint16_t get_generation() const;
      uint16_t get_type() const;

      bool check_alive(Instances::Slot *p_Slots, uint32_t p_Capacity) const;

      static RTTI::TypeInfo &get_type_info(uint16_t p_TypeId);
      static List<uint16_t> &get_component_types();

    protected:
      static void register_type_info(uint16_t p_TypeId,
                                     RTTI::TypeInfo &p_TypeInfo);
    };

    UniqueId LOW_EXPORT generate_unique_id(Handle p_Handle);
    void LOW_EXPORT register_unique_id(UniqueId p_UniqueId, Handle p_Handle);
    void LOW_EXPORT remove_unique_id(UniqueId p_UniqueId);

    Handle LOW_EXPORT find_handle_by_unique_id(UniqueId p_UniqueId);

  } // namespace Util
} // namespace Low
