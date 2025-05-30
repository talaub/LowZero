#include "LowRendererSkeleton.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Skeleton::TYPE_ID = 30;
    uint32_t Skeleton::ms_Capacity = 0u;
    uint8_t *Skeleton::ms_Buffer = 0;
    std::shared_mutex Skeleton::ms_BufferMutex;
    Low::Util::Instances::Slot *Skeleton::ms_Slots = 0;
    Low::Util::List<Skeleton> Skeleton::ms_LivingInstances =
        Low::Util::List<Skeleton>();

    Skeleton::Skeleton() : Low::Util::Handle(0ull)
    {
    }
    Skeleton::Skeleton(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Skeleton::Skeleton(Skeleton &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Skeleton::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Skeleton Skeleton::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      Skeleton l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Skeleton::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Skeleton, root_bone, Bone))
          Bone();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Skeleton, animations,
                              Util::List<SkeletalAnimation>))
          Util::List<SkeletalAnimation>();
      ACCESSOR_TYPE_SOA(l_Handle, Skeleton, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Skeleton::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Skeleton *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Skeleton::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(Skeleton));

      initialize_buffer(&ms_Buffer, SkeletonData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_Skeleton);
      LOW_PROFILE_ALLOC(type_slots_Skeleton);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Skeleton);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Skeleton::is_alive;
      l_TypeInfo.destroy = &Skeleton::destroy;
      l_TypeInfo.serialize = &Skeleton::serialize;
      l_TypeInfo.deserialize = &Skeleton::deserialize;
      l_TypeInfo.find_by_index = &Skeleton::_find_by_index;
      l_TypeInfo.find_by_name = &Skeleton::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Skeleton::_make;
      l_TypeInfo.duplicate_default = &Skeleton::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Skeleton::living_instances);
      l_TypeInfo.get_living_count = &Skeleton::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: root_bone
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(root_bone);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SkeletonData, root_bone);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_root_bone();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Skeleton,
                                            root_bone, Bone);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.set_root_bone(*(Bone *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((Bone *)p_Data) = l_Handle.get_root_bone();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: root_bone
      }
      {
        // Property: bone_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(bone_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletonData, bone_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_bone_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Skeleton,
                                            bone_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.set_bone_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_bone_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: bone_count
      }
      {
        // Property: animations
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(animations);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletonData, animations);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_animations();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Skeleton, animations,
              Util::List<SkeletalAnimation>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((Util::List<SkeletalAnimation> *)p_Data) =
              l_Handle.get_animations();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: animations
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SkeletonData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Skeleton, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Skeleton l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Skeleton l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Skeleton::cleanup()
    {
      Low::Util::List<Skeleton> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Skeleton);
      LOW_PROFILE_FREE(type_slots_Skeleton);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle Skeleton::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Skeleton Skeleton::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Skeleton l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Skeleton::TYPE_ID;

      return l_Handle;
    }

    bool Skeleton::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == Skeleton::TYPE_ID &&
             check_alive(ms_Slots, Skeleton::get_capacity());
    }

    uint32_t Skeleton::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Skeleton::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Skeleton Skeleton::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    Skeleton Skeleton::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Skeleton l_Handle = make(p_Name);
      l_Handle.set_root_bone(get_root_bone());
      l_Handle.set_bone_count(get_bone_count());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Skeleton Skeleton::duplicate(Skeleton p_Handle,
                                 Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Skeleton::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
    {
      Skeleton l_Skeleton = p_Handle.get_id();
      return l_Skeleton.duplicate(p_Name);
    }

    void Skeleton::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["bone_count"] = get_bone_count();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Skeleton::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Yaml::Node &p_Node)
    {
      Skeleton l_Skeleton = p_Handle.get_id();
      l_Skeleton.serialize(p_Node);
    }

    Low::Util::Handle
    Skeleton::deserialize(Low::Util::Yaml::Node &p_Node,
                          Low::Util::Handle p_Creator)
    {
      Skeleton l_Handle = Skeleton::make(N(Skeleton));

      if (p_Node["root_bone"]) {
      }
      if (p_Node["bone_count"]) {
        l_Handle.set_bone_count(p_Node["bone_count"].as<uint32_t>());
      }
      if (p_Node["animations"]) {
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    Bone &Skeleton::get_root_bone() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_root_bone

      // LOW_CODEGEN::END::CUSTOM:GETTER_root_bone

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Skeleton, root_bone, Bone);
    }
    void Skeleton::set_root_bone(Bone &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_root_bone

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_root_bone

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Skeleton, root_bone, Bone) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_root_bone

      // LOW_CODEGEN::END::CUSTOM:SETTER_root_bone
    }

    uint32_t Skeleton::get_bone_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bone_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_bone_count

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Skeleton, bone_count, uint32_t);
    }
    void Skeleton::set_bone_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bone_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bone_count

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Skeleton, bone_count, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bone_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_bone_count
    }

    Util::List<SkeletalAnimation> &Skeleton::get_animations() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_animations

      // LOW_CODEGEN::END::CUSTOM:GETTER_animations

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Skeleton, animations,
                      Util::List<SkeletalAnimation>);
    }

    Low::Util::Name Skeleton::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Skeleton, name, Low::Util::Name);
    }
    void Skeleton::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Skeleton, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    uint32_t Skeleton::create_instance()
    {
      uint32_t l_Index = 0u;

      for (; l_Index < get_capacity(); ++l_Index) {
        if (!ms_Slots[l_Index].m_Occupied) {
          break;
        }
      }
      if (l_Index >= get_capacity()) {
        increase_budget();
      }
      ms_Slots[l_Index].m_Occupied = true;
      return l_Index;
    }

    void Skeleton::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(SkeletonData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(SkeletonData, root_bone) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletonData, root_bone) *
                          (l_Capacity)],
               l_Capacity * sizeof(Bone));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SkeletonData, bone_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletonData, bone_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          Skeleton i_Skeleton = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(SkeletonData, animations) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::List<SkeletalAnimation>))])
              Util::List<SkeletalAnimation>();
          *i_ValPtr =
              ACCESSOR_TYPE_SOA(i_Skeleton, Skeleton, animations,
                                Util::List<SkeletalAnimation>);
        }
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(SkeletonData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(SkeletonData, name) * (l_Capacity)],
            l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity;
           i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for Skeleton from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
