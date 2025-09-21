#include "LowRendererSkeleton.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Skeleton::TYPE_ID = 30;
    uint32_t Skeleton::ms_Capacity = 0u;
    uint32_t Skeleton::ms_PageSize = 0u;
    Low::Util::SharedMutex Skeleton::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Skeleton::ms_PagesLock(Skeleton::ms_PagesMutex,
                               std::defer_lock);
    Low::Util::List<Skeleton> Skeleton::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Skeleton::ms_Pages;

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
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Skeleton l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Skeleton::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Skeleton, root_bone, Bone))
          Bone();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Skeleton, animations,
                                 Util::List<SkeletalAnimation>))
          Util::List<SkeletalAnimation>();
      ACCESSOR_TYPE_SOA(l_Handle, Skeleton, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Skeleton::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<Skeleton> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Page->slots[l_SlotIndex].m_Occupied = false;
      l_Page->slots[l_SlotIndex].m_Generation++;

      ms_PagesLock.lock();
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
    }

    void Skeleton::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(Skeleton));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Skeleton::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Skeleton);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Skeleton::is_alive;
      l_TypeInfo.destroy = &Skeleton::destroy;
      l_TypeInfo.serialize = &Skeleton::serialize;
      l_TypeInfo.deserialize = &Skeleton::deserialize;
      l_TypeInfo.find_by_index = &Skeleton::_find_by_index;
      l_TypeInfo.notify = &Skeleton::_notify;
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
        l_PropertyInfo.dataOffset =
            offsetof(Skeleton::Data, root_bone);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);
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
            offsetof(Skeleton::Data, bone_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);
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
            offsetof(Skeleton::Data, animations);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(Skeleton::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Skeleton l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Skeleton> l_HandleLock(l_Handle);
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
      ms_PagesLock.lock();
      for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
        Low::Util::Instances::Page *i_Page = *it;
        free(i_Page->buffer);
        free(i_Page->slots);
        free(i_Page->lockWords);
        delete i_Page;
        it = ms_Pages.erase(it);
      }

      ms_Capacity = 0;

      ms_PagesLock.unlock();
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
      l_Handle.m_Data.m_Type = Skeleton::TYPE_ID;

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
        l_Handle.m_Data.m_Generation = 0;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Handle.m_Data.m_Generation =
          l_Page->slots[l_SlotIndex].m_Generation;

      return l_Handle;
    }

    Skeleton Skeleton::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Skeleton l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Skeleton::TYPE_ID;

      return l_Handle;
    }

    bool Skeleton::is_alive() const
    {
      if (m_Data.m_Type != Skeleton::TYPE_ID) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      return m_Data.m_Type == Skeleton::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
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

    void
    Skeleton::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Skeleton::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Skeleton::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Skeleton::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Skeleton::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
    {
      Skeleton l_Skeleton = p_Observer.get_id();
      l_Skeleton.notify(p_Observed, p_Observable);
    }

    Bone &Skeleton::get_root_bone() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Skeleton> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_root_bone

      // LOW_CODEGEN::END::CUSTOM:GETTER_root_bone

      return TYPE_SOA(Skeleton, root_bone, Bone);
    }
    void Skeleton::set_root_bone(Bone &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Skeleton> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_root_bone

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_root_bone

      // Set new value
      TYPE_SOA(Skeleton, root_bone, Bone) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_root_bone

      // LOW_CODEGEN::END::CUSTOM:SETTER_root_bone

      broadcast_observable(N(root_bone));
    }

    uint32_t Skeleton::get_bone_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Skeleton> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bone_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_bone_count

      return TYPE_SOA(Skeleton, bone_count, uint32_t);
    }
    void Skeleton::set_bone_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Skeleton> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bone_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bone_count

      // Set new value
      TYPE_SOA(Skeleton, bone_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bone_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_bone_count

      broadcast_observable(N(bone_count));
    }

    Util::List<SkeletalAnimation> &Skeleton::get_animations() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Skeleton> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_animations

      // LOW_CODEGEN::END::CUSTOM:GETTER_animations

      return TYPE_SOA(Skeleton, animations,
                      Util::List<SkeletalAnimation>);
    }

    Low::Util::Name Skeleton::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Skeleton> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Skeleton, name, Low::Util::Name);
    }
    void Skeleton::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Skeleton> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Skeleton, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t Skeleton::create_instance(
        u32 &p_PageIndex, u32 &p_SlotIndex,
        Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      u32 l_Index = 0;
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      bool l_FoundIndex = false;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

      for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
           ++l_PageIndex) {
        Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
            ms_Pages[l_PageIndex]->mutex);
        for (l_SlotIndex = 0;
             l_SlotIndex < ms_Pages[l_PageIndex]->size;
             ++l_SlotIndex) {
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
            l_FoundIndex = true;
            l_PageLock = std::move(i_PageLock);
            break;
          }
          l_Index++;
        }
        if (l_FoundIndex) {
          break;
        }
      }
      if (!l_FoundIndex) {
        l_SlotIndex = 0;
        l_PageIndex = create_page();
        Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
            ms_Pages[l_PageIndex]->mutex);
        l_PageLock = std::move(l_NewLock);
      }
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      p_PageLock = std::move(l_PageLock);
      LOCK_UNLOCK(l_PagesLock);
      return l_Index;
    }

    u32 Skeleton::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Skeleton.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Skeleton::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Skeleton::get_page_for_index(const u32 p_Index,
                                      u32 &p_PageIndex,
                                      u32 &p_SlotIndex)
    {
      if (p_Index >= get_capacity()) {
        p_PageIndex = LOW_UINT32_MAX;
        p_SlotIndex = LOW_UINT32_MAX;
        return false;
      }
      p_PageIndex = p_Index / ms_PageSize;
      if (p_PageIndex > (ms_Pages.size() - 1)) {
        return false;
      }
      p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);
      return true;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
