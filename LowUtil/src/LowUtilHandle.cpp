#include "LowUtilHandle.h"

#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilLogger.h"
#include "LowUtilVariant.h"

#include <stdint.h>
#include <stdlib.h>
#include <vcruntime_string.h>

namespace Low {
  namespace Util {
    Map<uint16_t, RTTI::TypeInfo> g_TypeInfos;
    List<uint16_t> g_ComponentTypes;
    Map<UniqueId, Handle> g_UniqueIdRegistry;

    union UniqueIdCombination
    {
      UniqueId id;
      struct
      {
        uint32_t nameHash;
        uint16_t type;
        uint16_t randomComponent;
      } data;
    };

    UniqueId generate_unique_id(Handle p_Handle)
    {
      static Name l_NameName = N(name);
      static Name l_EntityPropertyName = N(entity);

      RTTI::TypeInfo &l_TypeInfo = Handle::get_type_info(p_Handle.get_type());

      UniqueIdCombination l_Combinator;

      RTTI::TypeInfo l_TypeInfoForName = l_TypeInfo;
      Handle l_HandleForName = p_Handle;

      if (l_TypeInfo.component) {
        LOW_ASSERT(l_TypeInfo.properties.find(l_EntityPropertyName) !=
                       l_TypeInfo.properties.end(),
                   "Could not find entity property for component while "
                   "generating uniqueId");

        l_HandleForName =
            *(Handle *)l_TypeInfo.properties[l_EntityPropertyName].get(
                p_Handle);

        l_TypeInfoForName = Handle::get_type_info(l_HandleForName.get_type());
      }

      LOW_ASSERT(l_TypeInfoForName.properties.find(l_NameName) !=
                     l_TypeInfoForName.properties.end(),
                 "Could not find name property for unique id generation");

      l_Combinator.data.nameHash =
          ((Name *)l_TypeInfoForName.properties[l_NameName].get(
               l_HandleForName))
              ->m_Index;
      l_Combinator.data.type = p_Handle.get_type();
      do {
        l_Combinator.data.randomComponent =
            (rand() % static_cast<int>(LOW_UINT16_MAX + 1));
      } while (g_UniqueIdRegistry.find(l_Combinator.id) !=
               g_UniqueIdRegistry.end());

      return l_Combinator.id;
    }

    void register_unique_id(UniqueId p_UniqueId, Handle p_Handle)
    {
      LOW_ASSERT(g_UniqueIdRegistry.find(p_UniqueId) ==
                     g_UniqueIdRegistry.end(),
                 "UniqueId collision");

      g_UniqueIdRegistry[p_UniqueId] = p_Handle;
    }

    void remove_unique_id(UniqueId p_UniqueId)
    {
      g_UniqueIdRegistry.erase(p_UniqueId);
    }

    Handle find_handle_by_unique_id(UniqueId p_UniqueId)
    {
      if (g_UniqueIdRegistry.find(p_UniqueId) == g_UniqueIdRegistry.end()) {
        return 0;
      }
      return g_UniqueIdRegistry[p_UniqueId];
    }

    Handle::Handle()
    {
    }

    Handle::Handle(uint64_t p_Id) : m_Id(p_Id)
    {
    }

    bool Handle::operator==(const Handle &p_Other) const
    {
      return m_Id == p_Other.m_Id;
    }

    bool Handle::operator!=(const Handle &p_Other) const
    {
      return m_Id != p_Other.m_Id;
    }

    bool Handle::operator<(const Handle &p_Other) const
    {
      return m_Id < p_Other.m_Id;
    }

    uint64_t Handle::get_id() const
    {
      return m_Id;
    }

    uint32_t Handle::get_index() const
    {
      return m_Data.m_Index;
    }

    uint16_t Handle::get_generation() const
    {
      return m_Data.m_Generation;
    }

    uint16_t Handle::get_type() const
    {
      return m_Data.m_Type;
    }

    Handle::operator uint64_t() const
    {
      return get_id();
    }

    bool Handle::check_alive(Instances::Slot *p_Slots,
                             uint32_t p_Capacity) const
    {
      if (m_Data.m_Index >= p_Capacity) {
        return false;
      }

      return p_Slots[m_Data.m_Index].m_Occupied &&
             p_Slots[m_Data.m_Index].m_Generation == m_Data.m_Generation;
    }

    void Handle::register_type_info(uint16_t p_TypeId,
                                    RTTI::TypeInfo &p_TypeInfo)
    {
      LOW_ASSERT(g_TypeInfos.find(p_TypeId) == g_TypeInfos.end(),
                 "Type info for this type id has already been registered");

      g_TypeInfos[p_TypeId] = p_TypeInfo;

      if (p_TypeInfo.component) {
        g_ComponentTypes.push_back(p_TypeId);
      }
    }

    RTTI::TypeInfo &Handle::get_type_info(uint16_t p_TypeId)
    {
      LOW_ASSERT(g_TypeInfos.find(p_TypeId) != g_TypeInfos.end(),
                 "Type info has not been registered for type id");

      return g_TypeInfos[p_TypeId];
    }

    List<uint16_t> &Handle::get_component_types()
    {
      return g_ComponentTypes;
    }

    void Handle::fill_variants(Util::Handle p_Handle,
                               Util::RTTI::PropertyInfo &p_PropertyInfo,
                               Util::Map<Util::Name, Util::Variant> &p_Variants)
    {
      if (p_PropertyInfo.type == Util::RTTI::PropertyType::UNKNOWN) {
        return;
      }
      if (p_PropertyInfo.type == Util::RTTI::PropertyType::SHAPE) {
        Math::Shape l_Shape = *(Math::Shape *)p_PropertyInfo.get(p_Handle);
        Util::String l_BaseString = p_PropertyInfo.name.c_str();
        l_BaseString += "__";

        if (l_Shape.type == Math::ShapeType::BOX) {
          p_Variants[LOW_NAME((l_BaseString + "type").c_str())] = N(BOX);

          p_Variants[LOW_NAME((l_BaseString + "box_position").c_str())] =
              l_Shape.box.position;
          p_Variants[LOW_NAME((l_BaseString + "box_rotation").c_str())] =
              l_Shape.box.rotation;
          p_Variants[LOW_NAME((l_BaseString + "box_halfextents").c_str())] =
              l_Shape.box.halfExtents;
        } else {
          LOW_ASSERT(false, "Reading component property while populating "
                            "prefab failed. Unsupported shape type");
        }

        return;
      }

      p_Variants[p_PropertyInfo.name] = p_PropertyInfo.get_variant(p_Handle);
    }

    Variant RTTI::PropertyInfo::get_variant(Handle p_Handle)
    {
      void const *l_Ptr = get(p_Handle);

      if (!l_Ptr) {
        return Variant(0);
      }

      if (type == Util::RTTI::PropertyType::BOOL) {
        return Variant(*(bool *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::FLOAT) {
        return Variant(*(float *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::INT) {
        return Variant(*(int *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::UINT32) {
        return Variant(*(uint32_t *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::UINT64) {
        return Variant(*(uint64_t *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::VECTOR2) {
        return Variant(*(Math::Vector2 *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::COLORRGB) {
        return Variant(*(Math::Vector3 *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::VECTOR3) {
        return Variant(*(Math::Vector3 *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::QUATERNION) {
        return Variant(*(Math::Quaternion *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::NAME) {
        return Variant(*(Name *)l_Ptr);
      }
      if (type == Util::RTTI::PropertyType::HANDLE) {
        return Variant::from_handle(*(uint64_t *)l_Ptr);
      }

      LOW_ASSERT(false, "Getting property value as variant not supported for "
                        "this property type");
    }

    namespace Instances {

      void initialize_buffer(uint8_t **p_Buffer, size_t p_ElementSize,
                             size_t p_ElementCount, Slot **p_Slots)
      {
        void *l_Buffer =
            calloc(p_ElementCount * p_ElementSize, sizeof(uint8_t));
        (*p_Buffer) = (uint8_t *)l_Buffer;

        void *l_SlotBuffer =
            malloc(p_ElementCount * sizeof(Low::Util::Instances::Slot));
        (*p_Slots) = (Low::Util::Instances::Slot *)l_SlotBuffer;

        // Initialize slots with unoccupied
        for (int i_Iter = 0; i_Iter < p_ElementCount; i_Iter++) {
          (*p_Slots)[i_Iter].m_Occupied = false;
          (*p_Slots)[i_Iter].m_Generation = 0;
        }
      }
    } // namespace Instances
  }   // namespace Util
} // namespace Low
