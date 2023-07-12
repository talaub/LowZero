#include "LowCoreScene.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCoreRegion.h"

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Scene::TYPE_ID = 20;
    uint32_t Scene::ms_Capacity = 0u;
    uint8_t *Scene::ms_Buffer = 0;
    Low::Util::Instances::Slot *Scene::ms_Slots = 0;
    Low::Util::List<Scene> Scene::ms_LivingInstances = Low::Util::List<Scene>();

    Scene::Scene() : Low::Util::Handle(0ull)
    {
    }
    Scene::Scene(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Scene::Scene(Scene &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Scene::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Scene Scene::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      Scene l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Scene::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Scene, regions,
                              Util::Set<Util::UniqueId>))
          Util::Set<Util::UniqueId>();
      ACCESSOR_TYPE_SOA(l_Handle, Scene, loaded, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, Scene, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      l_Handle.set_unique_id(Low::Util::generate_unique_id(l_Handle.get_id()));
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Scene::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      Low::Util::remove_unique_id(get_unique_id());

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Scene *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Scene::initialize()
    {
      ms_Capacity = Low::Util::Config::get_capacity(N(LowCore), N(Scene));

      initialize_buffer(&ms_Buffer, SceneData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Scene);
      LOW_PROFILE_ALLOC(type_slots_Scene);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Scene);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Scene::is_alive;
      l_TypeInfo.destroy = &Scene::destroy;
      l_TypeInfo.serialize = &Scene::serialize;
      l_TypeInfo.deserialize = &Scene::deserialize;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Scene::_make;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Scene::living_instances);
      l_TypeInfo.get_living_count = &Scene::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(regions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SceneData, regions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Scene l_Handle = p_Handle.get_id();
          l_Handle.get_regions();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Scene, regions,
                                            Util::Set<Util::UniqueId>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(loaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SceneData, loaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Scene l_Handle = p_Handle.get_id();
          l_Handle.is_loaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Scene, loaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SceneData, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Scene l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Scene, unique_id,
                                            Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SceneData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Scene l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Scene, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Scene l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Scene::cleanup()
    {
      Low::Util::List<Scene> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Scene);
      LOW_PROFILE_FREE(type_slots_Scene);
    }

    Scene Scene::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Scene l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Scene::TYPE_ID;

      return l_Handle;
    }

    bool Scene::is_alive() const
    {
      return m_Data.m_Type == Scene::TYPE_ID &&
             check_alive(ms_Slots, Scene::get_capacity());
    }

    uint32_t Scene::get_capacity()
    {
      return ms_Capacity;
    }

    Scene Scene::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    void Scene::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      p_Node["name"] = get_name().c_str();
      p_Node["unique_id"] = get_unique_id();

      for (auto it = get_regions().begin(); it != get_regions().end(); ++it) {
        p_Node["regions"].push_back(*it);
      }
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Scene::serialize(Low::Util::Handle p_Handle,
                          Low::Util::Yaml::Node &p_Node)
    {
      Scene l_Scene = p_Handle.get_id();
      l_Scene.serialize(p_Node);
    }

    Low::Util::Handle Scene::deserialize(Low::Util::Yaml::Node &p_Node,
                                         Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      Scene l_Scene = Scene::make(LOW_YAML_AS_NAME(p_Node["name"]));
      l_Scene.set_unique_id(p_Node["unique_id"].as<Util::UniqueId>());

      for (auto it = p_Node["regions"].begin(); it != p_Node["regions"].end();
           ++it) {
        Region i_Region =
            Util::find_handle_by_unique_id(it->as<Util::UniqueId>()).get_id();

        i_Region.set_scene(l_Scene);
      }

      return l_Scene;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    Util::Set<Util::UniqueId> &Scene::get_regions() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_regions
      // LOW_CODEGEN::END::CUSTOM:GETTER_regions

      return TYPE_SOA(Scene, regions, Util::Set<Util::UniqueId>);
    }

    bool Scene::is_loaded() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_loaded
      // LOW_CODEGEN::END::CUSTOM:GETTER_loaded

      return TYPE_SOA(Scene, loaded, bool);
    }
    void Scene::set_loaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_loaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_loaded

      // Set new value
      TYPE_SOA(Scene, loaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_loaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_loaded
    }

    Low::Util::UniqueId Scene::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Scene, unique_id, Low::Util::UniqueId);
    }
    void Scene::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Scene, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
    }

    Low::Util::Name Scene::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Scene, name, Low::Util::Name);
    }
    void Scene::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Scene, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    void Scene::load()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        it->unload();
      }

      _load();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
    }

    void Scene::unload()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload
      set_loaded(false);

      for (auto it = get_regions().begin(); it != get_regions().end(); ++it) {
        Region i_Region = Util::find_handle_by_unique_id(*it).get_id();
        if (i_Region.is_loaded()) {
          i_Region.unload_entities();
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
    }

    void Scene::_load()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__load
      for (auto it = get_regions().begin(); it != get_regions().end(); ++it) {
        Region i_Region = Util::find_handle_by_unique_id(*it).get_id();
        if (!i_Region.is_streaming_enabled()) {
          i_Region.load_entities();
        }
      }
      LOW_LOG_DEBUG << "Scene '" << get_name() << "' loaded" << LOW_LOG_END;
      set_loaded(true);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__load
    }

    Scene Scene::get_loaded_scene()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_loaded_scene
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->is_loaded()) {
          return *it;
        }
      }

      return 0;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_loaded_scene
    }

    uint32_t Scene::create_instance()
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

    void Scene::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(SceneData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr =
              new (&l_NewBuffer[offsetof(SceneData, regions) *
                                    (l_Capacity + l_CapacityIncrease) +
                                (it->get_index() *
                                 sizeof(Util::Set<Util::UniqueId>))])
                  Util::Set<Util::UniqueId>();
          *i_ValPtr = it->get_regions();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(SceneData, loaded) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SceneData, loaded) * (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SceneData, unique_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SceneData, unique_id) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::UniqueId));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SceneData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SceneData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for Scene from " << l_Capacity
                    << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Core
} // namespace Low
