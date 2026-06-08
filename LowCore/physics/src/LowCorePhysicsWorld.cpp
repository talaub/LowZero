#include "LowCorePhysicsWorld.h"

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
#include "LowCorePhysicsBackend.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Physics {
// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
#define BACKEND_WORLD() static_cast<WorldBackend *>(get_world_ptr())
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 World::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          World::IDENTIFIER(LOW_NAME(1181529166),
                            LOW_NAME(4223024711));
      uint32_t World::ms_Capacity = 0u;
      uint32_t World::ms_PageSize = 0u;
      Low::Util::List<World> World::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> World::ms_Pages;

      Low::Util::Handle World::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      World World::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        World l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = World::ms_TypeId;

        ACCESSOR_TYPE_SOA(l_Handle, World, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_world_ptr(create_world_backend());
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void World::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          destroy_world_backend(
              static_cast<WorldBackend *>(get_world_ptr()));
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
      }

      void World::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                          N(World));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(World));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, World::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(World);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &World::is_alive;
        l_TypeInfo.destroy = &World::destroy;
        l_TypeInfo.serialize = &World::serialize;
        l_TypeInfo.deserialize = &World::deserialize;
        l_TypeInfo.find_by_index = &World::_find_by_index;
        l_TypeInfo.notify = &World::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &World::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &World::_make;
        l_TypeInfo.duplicate_default = &World::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &World::living_instances);
        l_TypeInfo.get_living_count = &World::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: world_ptr
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(world_ptr);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(World::Data, world_ptr);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            World l_Handle = p_Handle.get_id();
            l_Handle.get_world_ptr();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, World,
                                              world_ptr, void *);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            World l_Handle = p_Handle.get_id();
            *((void **)p_Data) = l_Handle.get_world_ptr();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: world_ptr
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(World::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            World l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, World, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            World l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            World l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Virtual property: gravity
          Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(gravity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            World l_Handle = p_Handle.get_id();
            Low::Math::Vector3 l_Data = l_Handle.get_gravity();
            memcpy(p_Data, &l_Data, sizeof(Low::Math::Vector3));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            World l_Handle = p_Handle.get_id();
            l_Handle.set_gravity(*(Low::Math::Vector3 *)p_Data);
          };
          l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
              l_PropertyInfo;
          // End virtual property: gravity
        }
        {
          // Function: simulate
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(simulate);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Delta);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: simulate
        }
        {
          // Function: get_gravity
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_gravity);
          l_FunctionInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_gravity
        }
        {
          // Function: set_gravity
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(set_gravity);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Value);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR3;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: set_gravity
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void World::cleanup()
      {
        Low::Util::List<World> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;
      }

      Low::Util::Handle World::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      World World::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        World l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = World::ms_TypeId;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      World World::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        World l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = World::ms_TypeId;

        return l_Handle;
      }

      bool World::is_alive() const
      {
        if (m_Data.m_Type != World::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == World::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t World::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle World::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      World World::find_by_name(Low::Util::Name p_Name)
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

      World World::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        World l_Handle = make(p_Name);
        l_Handle.set_world_ptr(get_world_ptr());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      World World::duplicate(World p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle World::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
      {
        World l_World = p_Handle.get_id();
        return l_World.duplicate(p_Name);
      }

      void World::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void World::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node)
      {
        World l_World = p_Handle.get_id();
        l_World.serialize(p_Node);
      }

      Low::Util::Handle
      World::deserialize(Low::Util::Serial::Node &p_Node,
                         Low::Util::Handle p_Creator)
      {
        World l_Handle = World::make(N(World));

        if (p_Node["world_ptr"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void
      World::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 World::observe(Low::Util::Name p_Observable,
                         Low::Util::Function<void(Low::Util::Handle,
                                                  Low::Util::Name)>
                             p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 World::observe(Low::Util::Name p_Observable,
                         Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void World::notify(Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void World::_notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        World l_World = p_Observer.get_id();
        l_World.notify(p_Observed, p_Observable);
      }

      void *World::get_world_ptr() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_ptr
        // LOW_CODEGEN::END::CUSTOM:GETTER_world_ptr

        return TYPE_SOA(World, world_ptr, void *);
      }
      void World::set_world_ptr(void *p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_ptr
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_ptr

        // Set new value
        TYPE_SOA(World, world_ptr, void *) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_ptr
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_ptr

        broadcast_observable(N(world_ptr));
      }

      Low::Util::Name World::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(World, name, Low::Util::Name);
      }
      void World::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(World, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      void World::simulate(float p_Delta)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_simulate
        simulate_world(BACKEND_WORLD(), p_Delta);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_simulate
      }

      Low::Math::Vector3 World::get_gravity()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_gravity
        return get_world_gravity(BACKEND_WORLD());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_gravity
      }

      void World::set_gravity(Low::Math::Vector3 p_Value)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_gravity
        set_world_gravity(BACKEND_WORLD(), p_Value);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_gravity
      }

      uint32_t World::create_instance(u32 &p_PageIndex,
                                      u32 &p_SlotIndex)
      {
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
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
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        return l_Index;
      }

      u32 World::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for World.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, World::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool World::get_page_for_index(const u32 p_Index,
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

    } // namespace Physics
  } // namespace Core
} // namespace Low
