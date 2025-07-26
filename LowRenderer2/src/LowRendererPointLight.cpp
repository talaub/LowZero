#include "LowRendererPointLight.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererRenderScene.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    Low::Util::Set<Low::Renderer::PointLight>
        Low::Renderer::PointLight::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t PointLight::TYPE_ID = 65;
    uint32_t PointLight::ms_Capacity = 0u;
    uint8_t *PointLight::ms_Buffer = 0;
    std::shared_mutex PointLight::ms_BufferMutex;
    Low::Util::Instances::Slot *PointLight::ms_Slots = 0;
    Low::Util::List<PointLight> PointLight::ms_LivingInstances =
        Low::Util::List<PointLight>();

    PointLight::PointLight() : Low::Util::Handle(0ull)
    {
    }
    PointLight::PointLight(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    PointLight::PointLight(PointLight &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle PointLight::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    PointLight PointLight::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      PointLight l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = PointLight::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, PointLight, world_position,
                              Low::Math::Vector3))
          Low::Math::Vector3();
      new (&ACCESSOR_TYPE_SOA(l_Handle, PointLight, color,
                              Low::Math::ColorRGB))
          Low::Math::ColorRGB();
      ACCESSOR_TYPE_SOA(l_Handle, PointLight, intensity, float) =
          0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, PointLight, range, float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, PointLight, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.mark_dirty();
      l_Handle.set_slot(LOW_UINT32_MAX);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void PointLight::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      RenderScene l_RenderScene = get_render_scene_handle();
      if (l_RenderScene.is_alive()) {
        l_RenderScene.get_pointlight_deleted_slots().insert(
            get_slot());
      }
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const PointLight *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void PointLight::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(PointLight));

      initialize_buffer(&ms_Buffer, PointLightData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_PointLight);
      LOW_PROFILE_ALLOC(type_slots_PointLight);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(PointLight);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &PointLight::is_alive;
      l_TypeInfo.destroy = &PointLight::destroy;
      l_TypeInfo.serialize = &PointLight::serialize;
      l_TypeInfo.deserialize = &PointLight::deserialize;
      l_TypeInfo.find_by_index = &PointLight::_find_by_index;
      l_TypeInfo.find_by_name = &PointLight::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &PointLight::_make;
      l_TypeInfo.duplicate_default = &PointLight::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &PointLight::living_instances);
      l_TypeInfo.get_living_count = &PointLight::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: world_position
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_position);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(PointLightData, world_position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.get_world_position();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, PointLight,
                                            world_position,
                                            Low::Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.set_world_position(*(Low::Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          PointLight l_Handle = p_Handle.get_id();
          *((Low::Math::Vector3 *)p_Data) =
              l_Handle.get_world_position();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: world_position
      }
      {
        // Property: color
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(color);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(PointLightData, color);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::COLORRGB;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.get_color();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, PointLight, color, Low::Math::ColorRGB);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.set_color(*(Low::Math::ColorRGB *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          PointLight l_Handle = p_Handle.get_id();
          *((Low::Math::ColorRGB *)p_Data) = l_Handle.get_color();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: color
      }
      {
        // Property: intensity
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(intensity);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(PointLightData, intensity);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.get_intensity();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, PointLight,
                                            intensity, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.set_intensity(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          PointLight l_Handle = p_Handle.get_id();
          *((float *)p_Data) = l_Handle.get_intensity();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: intensity
      }
      {
        // Property: range
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(range);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(PointLightData, range);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.get_range();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, PointLight,
                                            range, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.set_range(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          PointLight l_Handle = p_Handle.get_id();
          *((float *)p_Data) = l_Handle.get_range();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: range
      }
      {
        // Property: render_scene_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_scene_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(PointLightData, render_scene_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.get_render_scene_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, PointLight, render_scene_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          PointLight l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_render_scene_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_scene_handle
      }
      {
        // Property: slot
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(slot);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(PointLightData, slot);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.get_slot();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, PointLight,
                                            slot, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.set_slot(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          PointLight l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_slot();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: slot
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(PointLightData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, PointLight,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          PointLight l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          PointLight l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = PointLight::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderScene);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderScene::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void PointLight::cleanup()
    {
      Low::Util::List<PointLight> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_PointLight);
      LOW_PROFILE_FREE(type_slots_PointLight);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle PointLight::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    PointLight PointLight::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      PointLight l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = PointLight::TYPE_ID;

      return l_Handle;
    }

    bool PointLight::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == PointLight::TYPE_ID &&
             check_alive(ms_Slots, PointLight::get_capacity());
    }

    uint32_t PointLight::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    PointLight::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    PointLight PointLight::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    PointLight PointLight::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      PointLight l_Handle = make(p_Name);
      l_Handle.set_world_position(get_world_position());
      l_Handle.set_color(get_color());
      l_Handle.set_intensity(get_intensity());
      l_Handle.set_range(get_range());
      l_Handle.set_render_scene_handle(get_render_scene_handle());
      l_Handle.set_slot(get_slot());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    PointLight PointLight::duplicate(PointLight p_Handle,
                                     Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    PointLight::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
    {
      PointLight l_PointLight = p_Handle.get_id();
      return l_PointLight.duplicate(p_Name);
    }

    void PointLight::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      Low::Util::Serialization::serialize(p_Node["world_position"],
                                          get_world_position());
      Low::Util::Serialization::serialize(p_Node["color"],
                                          get_color());
      p_Node["intensity"] = get_intensity();
      p_Node["range"] = get_range();
      p_Node["render_scene_handle"] = get_render_scene_handle();
      p_Node["slot"] = get_slot();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void PointLight::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Yaml::Node &p_Node)
    {
      PointLight l_PointLight = p_Handle.get_id();
      l_PointLight.serialize(p_Node);
    }

    Low::Util::Handle
    PointLight::deserialize(Low::Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator)
    {
      PointLight l_Handle = PointLight::make(N(PointLight));

      if (p_Node["world_position"]) {
        l_Handle.set_world_position(
            Low::Util::Serialization::deserialize_vector3(
                p_Node["world_position"]));
      }
      if (p_Node["color"]) {
        l_Handle.set_color(
            Low::Util::Serialization::deserialize_vector3(
                p_Node["color"]));
      }
      if (p_Node["intensity"]) {
        l_Handle.set_intensity(p_Node["intensity"].as<float>());
      }
      if (p_Node["range"]) {
        l_Handle.set_range(p_Node["range"].as<float>());
      }
      if (p_Node["render_scene_handle"]) {
        l_Handle.set_render_scene_handle(
            p_Node["render_scene_handle"].as<uint64_t>());
      }
      if (p_Node["slot"]) {
        l_Handle.set_slot(p_Node["slot"].as<uint32_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    Low::Math::Vector3 &PointLight::get_world_position() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_position
      // LOW_CODEGEN::END::CUSTOM:GETTER_world_position

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(PointLight, world_position, Low::Math::Vector3);
    }
    void PointLight::set_world_position(float p_X, float p_Y,
                                        float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_world_position(p_Val);
    }

    void PointLight::set_world_position_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_world_position();
      l_Value.x = p_Value;
      set_world_position(l_Value);
    }

    void PointLight::set_world_position_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_world_position();
      l_Value.y = p_Value;
      set_world_position(l_Value);
    }

    void PointLight::set_world_position_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_world_position();
      l_Value.z = p_Value;
      set_world_position(l_Value);
    }

    void PointLight::set_world_position(Low::Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_position
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_position

      if (get_world_position() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(PointLight, world_position, Low::Math::Vector3) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_position
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_position
      }
    }

    Low::Math::ColorRGB &PointLight::get_color() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color
      // LOW_CODEGEN::END::CUSTOM:GETTER_color

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(PointLight, color, Low::Math::ColorRGB);
    }
    void PointLight::set_color(Low::Math::ColorRGB &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

      if (get_color() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(PointLight, color, Low::Math::ColorRGB) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color
        // LOW_CODEGEN::END::CUSTOM:SETTER_color
      }
    }

    float PointLight::get_intensity() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_intensity
      // LOW_CODEGEN::END::CUSTOM:GETTER_intensity

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(PointLight, intensity, float);
    }
    void PointLight::set_intensity(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_intensity
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_intensity

      if (get_intensity() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(PointLight, intensity, float) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_intensity
        // LOW_CODEGEN::END::CUSTOM:SETTER_intensity
      }
    }

    float PointLight::get_range() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_range
      // LOW_CODEGEN::END::CUSTOM:GETTER_range

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(PointLight, range, float);
    }
    void PointLight::set_range(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_range
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_range

      if (get_range() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(PointLight, range, float) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_range
        // LOW_CODEGEN::END::CUSTOM:SETTER_range
      }
    }

    uint64_t PointLight::get_render_scene_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(PointLight, render_scene_handle, uint64_t);
    }
    void PointLight::set_render_scene_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(PointLight, render_scene_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene_handle
    }

    uint32_t PointLight::get_slot() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_slot
      // LOW_CODEGEN::END::CUSTOM:GETTER_slot

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(PointLight, slot, uint32_t);
    }
    void PointLight::set_slot(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_slot
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_slot

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(PointLight, slot, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_slot
      // LOW_CODEGEN::END::CUSTOM:SETTER_slot
    }

    void PointLight::mark_dirty()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
      ms_Dirty.insert(get_id());
      // LOW_CODEGEN::END::CUSTOM:MARK_dirty
    }

    Low::Util::Name PointLight::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(PointLight, name, Low::Util::Name);
    }
    void PointLight::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(PointLight, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    PointLight
    PointLight::make(Low::Renderer::RenderScene p_RenderScene)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      PointLight l_PointLight = PointLight::make(N(PLight));

      l_PointLight.set_render_scene_handle(p_RenderScene.get_id());

      l_PointLight.mark_dirty();

      return l_PointLight;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t PointLight::create_instance()
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

    void PointLight::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(PointLightData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(PointLightData, world_position) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(PointLightData, world_position) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Vector3));
      }
      {
        memcpy(&l_NewBuffer[offsetof(PointLightData, color) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(PointLightData, color) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::ColorRGB));
      }
      {
        memcpy(&l_NewBuffer[offsetof(PointLightData, intensity) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(PointLightData, intensity) *
                          (l_Capacity)],
               l_Capacity * sizeof(float));
      }
      {
        memcpy(&l_NewBuffer[offsetof(PointLightData, range) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(PointLightData, range) *
                          (l_Capacity)],
               l_Capacity * sizeof(float));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(PointLightData,
                                  render_scene_handle) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(PointLightData, render_scene_handle) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(PointLightData, slot) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(PointLightData, slot) * (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(PointLightData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(PointLightData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for PointLight from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
