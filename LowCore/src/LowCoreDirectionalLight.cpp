#include "LowCoreDirectionalLight.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCorePrefabInstance.h"
namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t DirectionalLight::TYPE_ID = 27;
      uint32_t DirectionalLight::ms_Capacity = 0u;
      uint8_t *DirectionalLight::ms_Buffer = 0;
      Low::Util::Instances::Slot *DirectionalLight::ms_Slots = 0;
      Low::Util::List<DirectionalLight>
          DirectionalLight::ms_LivingInstances =
              Low::Util::List<DirectionalLight>();

      DirectionalLight::DirectionalLight() : Low::Util::Handle(0ull)
      {
      }
      DirectionalLight::DirectionalLight(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      DirectionalLight::DirectionalLight(DirectionalLight &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle
      DirectionalLight::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      DirectionalLight
      DirectionalLight::make(Low::Core::Entity p_Entity)
      {
        uint32_t l_Index = create_instance();

        DirectionalLight l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = DirectionalLight::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, DirectionalLight, color,
                                Math::ColorRGB)) Math::ColorRGB();
        ACCESSOR_TYPE_SOA(l_Handle, DirectionalLight, intensity,
                          float) = 0.0f;
        new (&ACCESSOR_TYPE_SOA(l_Handle, DirectionalLight, entity,
                                Low::Core::Entity))
            Low::Core::Entity();

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void DirectionalLight::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        Low::Util::remove_unique_id(get_unique_id());

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const DirectionalLight *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void DirectionalLight::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(DirectionalLight));

        initialize_buffer(&ms_Buffer,
                          DirectionalLightData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_DirectionalLight);
        LOW_PROFILE_ALLOC(type_slots_DirectionalLight);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(DirectionalLight);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &DirectionalLight::is_alive;
        l_TypeInfo.destroy = &DirectionalLight::destroy;
        l_TypeInfo.serialize = &DirectionalLight::serialize;
        l_TypeInfo.deserialize = &DirectionalLight::deserialize;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &DirectionalLight::_make;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &DirectionalLight::living_instances);
        l_TypeInfo.get_living_count = &DirectionalLight::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(color);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(DirectionalLightData, color);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::COLORRGB;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            DirectionalLight l_Handle = p_Handle.get_id();
            l_Handle.get_color();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, DirectionalLight, color, Math::ColorRGB);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            DirectionalLight l_Handle = p_Handle.get_id();
            l_Handle.set_color(*(Math::ColorRGB *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(intensity);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(DirectionalLightData, intensity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            DirectionalLight l_Handle = p_Handle.get_id();
            l_Handle.get_intensity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, DirectionalLight, intensity, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            DirectionalLight l_Handle = p_Handle.get_id();
            l_Handle.set_intensity(*(float *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(DirectionalLightData, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            DirectionalLight l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, DirectionalLight, entity,
                Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            DirectionalLight l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(DirectionalLightData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            DirectionalLight l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, DirectionalLight, unique_id,
                Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void DirectionalLight::cleanup()
      {
        Low::Util::List<DirectionalLight> l_Instances =
            ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_DirectionalLight);
        LOW_PROFILE_FREE(type_slots_DirectionalLight);
      }

      DirectionalLight
      DirectionalLight::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        DirectionalLight l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = DirectionalLight::TYPE_ID;

        return l_Handle;
      }

      bool DirectionalLight::is_alive() const
      {
        return m_Data.m_Type == DirectionalLight::TYPE_ID &&
               check_alive(ms_Slots,
                           DirectionalLight::get_capacity());
      }

      uint32_t DirectionalLight::get_capacity()
      {
        return ms_Capacity;
      }

      void
      DirectionalLight::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        Low::Util::Serialization::serialize(p_Node["color"],
                                            get_color());
        p_Node["intensity"] = get_intensity();
        p_Node["unique_id"] = get_unique_id();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void DirectionalLight::serialize(Low::Util::Handle p_Handle,
                                       Low::Util::Yaml::Node &p_Node)
      {
        DirectionalLight l_DirectionalLight = p_Handle.get_id();
        l_DirectionalLight.serialize(p_Node);
      }

      Low::Util::Handle
      DirectionalLight::deserialize(Low::Util::Yaml::Node &p_Node,
                                    Low::Util::Handle p_Creator)
      {
        DirectionalLight l_Handle =
            DirectionalLight::make(p_Creator.get_id());

        if (p_Node["unique_id"]) {
          Low::Util::remove_unique_id(l_Handle.get_unique_id());
          l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());
        }

        if (p_Node["color"]) {
          l_Handle.set_color(
              Low::Util::Serialization::deserialize_vector3(
                  p_Node["color"]));
        }
        if (p_Node["intensity"]) {
          l_Handle.set_intensity(p_Node["intensity"].as<float>());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Math::ColorRGB &DirectionalLight::get_color() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color
        // LOW_CODEGEN::END::CUSTOM:GETTER_color

        return TYPE_SOA(DirectionalLight, color, Math::ColorRGB);
      }
      void DirectionalLight::set_color(Math::ColorRGB &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

        // Set new value
        TYPE_SOA(DirectionalLight, color, Math::ColorRGB) = p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(color),
                  !l_Prefab.compare_property(*this, N(color)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color
        // LOW_CODEGEN::END::CUSTOM:SETTER_color
      }

      float DirectionalLight::get_intensity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_intensity
        // LOW_CODEGEN::END::CUSTOM:GETTER_intensity

        return TYPE_SOA(DirectionalLight, intensity, float);
      }
      void DirectionalLight::set_intensity(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_intensity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_intensity

        // Set new value
        TYPE_SOA(DirectionalLight, intensity, float) = p_Value;
        {
          Low::Core::Entity l_Entity = get_entity();
          if (l_Entity.has_component(
                  Low::Core::Component::PrefabInstance::TYPE_ID)) {
            Low::Core::Component::PrefabInstance l_Instance =
                l_Entity.get_component(
                    Low::Core::Component::PrefabInstance::TYPE_ID);
            Low::Core::Prefab l_Prefab = l_Instance.get_prefab();
            if (l_Prefab.is_alive()) {
              l_Instance.override(
                  TYPE_ID, N(intensity),
                  !l_Prefab.compare_property(*this, N(intensity)));
            }
          }
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_intensity
        // LOW_CODEGEN::END::CUSTOM:SETTER_intensity
      }

      Low::Core::Entity DirectionalLight::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(DirectionalLight, entity, Low::Core::Entity);
      }
      void DirectionalLight::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(DirectionalLight, entity, Low::Core::Entity) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity
      }

      Low::Util::UniqueId DirectionalLight::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(DirectionalLight, unique_id,
                        Low::Util::UniqueId);
      }
      void
      DirectionalLight::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(DirectionalLight, unique_id, Low::Util::UniqueId) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
      }

      uint32_t DirectionalLight::create_instance()
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

      void DirectionalLight::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer =
            (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                              sizeof(DirectionalLightData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(DirectionalLightData, color) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(DirectionalLightData, color) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Math::ColorRGB));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(DirectionalLightData, intensity) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(DirectionalLightData, intensity) *
                         (l_Capacity)],
              l_Capacity * sizeof(float));
        }
        {
          memcpy(&l_NewBuffer[offsetof(DirectionalLightData, entity) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(DirectionalLightData, entity) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Core::Entity));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(DirectionalLightData, unique_id) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(DirectionalLightData, unique_id) *
                         (l_Capacity)],
              l_Capacity * sizeof(Low::Util::UniqueId));
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

        LOW_LOG_DEBUG
            << "Auto-increased budget for DirectionalLight from "
            << l_Capacity << " to "
            << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }
    } // namespace Component
  }   // namespace Core
} // namespace Low
