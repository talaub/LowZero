#include "LowRendererPointLight.h"

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
    uint32_t PointLight::ms_PageSize = 0u;
    Low::Util::SharedMutex PointLight::ms_LivingMutex;
    Low::Util::SharedMutex PointLight::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        PointLight::ms_PagesLock(PointLight::ms_PagesMutex,
                                 std::defer_lock);
    Low::Util::List<PointLight> PointLight::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        PointLight::ms_Pages;

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
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      PointLight l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = PointLight::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, PointLight, world_position,
                                 Low::Math::Vector3))
          Low::Math::Vector3();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, PointLight, color,
                                 Low::Math::ColorRGB))
          Low::Math::ColorRGB();
      ACCESSOR_TYPE_SOA(l_Handle, PointLight, intensity, float) =
          0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, PointLight, range, float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, PointLight, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.mark_dirty();
      l_Handle.set_slot(LOW_UINT32_MAX);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void PointLight::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<PointLight> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        RenderScene l_RenderScene = get_render_scene_handle();
        if (l_RenderScene.is_alive()) {
          l_RenderScene.get_pointlight_deleted_slots().insert(
              get_slot());
        }
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
      Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
      l_LivingLock.unlock();
    }

    void PointLight::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(PointLight));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, PointLight::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(PointLight);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &PointLight::is_alive;
      l_TypeInfo.destroy = &PointLight::destroy;
      l_TypeInfo.serialize = &PointLight::serialize;
      l_TypeInfo.deserialize = &PointLight::deserialize;
      l_TypeInfo.find_by_index = &PointLight::_find_by_index;
      l_TypeInfo.notify = &PointLight::_notify;
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
            offsetof(PointLight::Data, world_position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(PointLight::Data, color);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::COLORRGB;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
            offsetof(PointLight::Data, intensity);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(PointLight::Data, range);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
            offsetof(PointLight::Data, render_scene_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
          l_Handle.get_render_scene_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, PointLight, render_scene_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          PointLight l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(PointLight::Data, slot);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(PointLight::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          PointLight l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<PointLight> l_HandleLock(l_Handle);
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

    Low::Util::Handle PointLight::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    PointLight PointLight::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      PointLight l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = PointLight::TYPE_ID;

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

    PointLight PointLight::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      PointLight l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = PointLight::TYPE_ID;

      return l_Handle;
    }

    bool PointLight::is_alive() const
    {
      if (m_Data.m_Type != PointLight::TYPE_ID) {
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
      return m_Data.m_Type == PointLight::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
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

    void PointLight::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 PointLight::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 PointLight::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void PointLight::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void PointLight::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      PointLight l_PointLight = p_Observer.get_id();
      l_PointLight.notify(p_Observed, p_Observable);
    }

    Low::Math::Vector3 &PointLight::get_world_position() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_position
      // LOW_CODEGEN::END::CUSTOM:GETTER_world_position

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
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_position
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_position

      if (get_world_position() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(PointLight, world_position, Low::Math::Vector3) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_position
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_position

        broadcast_observable(N(world_position));
      }
    }

    Low::Math::ColorRGB &PointLight::get_color() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color
      // LOW_CODEGEN::END::CUSTOM:GETTER_color

      return TYPE_SOA(PointLight, color, Low::Math::ColorRGB);
    }
    void PointLight::set_color(float p_X, float p_Y, float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_color(p_Val);
    }

    void PointLight::set_color_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_color();
      l_Value.x = p_Value;
      set_color(l_Value);
    }

    void PointLight::set_color_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_color();
      l_Value.y = p_Value;
      set_color(l_Value);
    }

    void PointLight::set_color_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_color();
      l_Value.z = p_Value;
      set_color(l_Value);
    }

    void PointLight::set_color(Low::Math::ColorRGB &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

      if (get_color() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(PointLight, color, Low::Math::ColorRGB) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color
        // LOW_CODEGEN::END::CUSTOM:SETTER_color

        broadcast_observable(N(color));
      }
    }

    float PointLight::get_intensity() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_intensity
      // LOW_CODEGEN::END::CUSTOM:GETTER_intensity

      return TYPE_SOA(PointLight, intensity, float);
    }
    void PointLight::set_intensity(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_intensity
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_intensity

      if (get_intensity() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(PointLight, intensity, float) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_intensity
        // LOW_CODEGEN::END::CUSTOM:SETTER_intensity

        broadcast_observable(N(intensity));
      }
    }

    float PointLight::get_range() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_range
      // LOW_CODEGEN::END::CUSTOM:GETTER_range

      return TYPE_SOA(PointLight, range, float);
    }
    void PointLight::set_range(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_range
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_range

      if (get_range() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(PointLight, range, float) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_range
        // LOW_CODEGEN::END::CUSTOM:SETTER_range

        broadcast_observable(N(range));
      }
    }

    uint64_t PointLight::get_render_scene_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene_handle

      return TYPE_SOA(PointLight, render_scene_handle, uint64_t);
    }
    void PointLight::set_render_scene_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene_handle

      // Set new value
      TYPE_SOA(PointLight, render_scene_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene_handle

      broadcast_observable(N(render_scene_handle));
    }

    uint32_t PointLight::get_slot() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_slot
      // LOW_CODEGEN::END::CUSTOM:GETTER_slot

      return TYPE_SOA(PointLight, slot, uint32_t);
    }
    void PointLight::set_slot(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_slot
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_slot

      // Set new value
      TYPE_SOA(PointLight, slot, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_slot
      // LOW_CODEGEN::END::CUSTOM:SETTER_slot

      broadcast_observable(N(slot));
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
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(PointLight, name, Low::Util::Name);
    }
    void PointLight::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<PointLight> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(PointLight, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
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

    uint32_t PointLight::create_instance(
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

    u32 PointLight::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for PointLight.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, PointLight::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool PointLight::get_page_for_index(const u32 p_Index,
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
