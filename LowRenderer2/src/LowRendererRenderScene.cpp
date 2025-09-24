#include "LowRendererRenderScene.h"

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
#include "LowRendererDrawCommand.h"
#include "LowRendererVkScene.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t RenderScene::TYPE_ID = 61;
    uint32_t RenderScene::ms_Capacity = 0u;
    uint32_t RenderScene::ms_PageSize = 0u;
    Low::Util::SharedMutex RenderScene::ms_LivingMutex;
    Low::Util::SharedMutex RenderScene::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        RenderScene::ms_PagesLock(RenderScene::ms_PagesMutex,
                                  std::defer_lock);
    Low::Util::List<RenderScene> RenderScene::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        RenderScene::ms_Pages;

    RenderScene::RenderScene() : Low::Util::Handle(0ull)
    {
    }
    RenderScene::RenderScene(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    RenderScene::RenderScene(RenderScene &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle RenderScene::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    RenderScene RenderScene::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      RenderScene l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = RenderScene::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderScene, draw_commands,
                                 Low::Util::List<DrawCommand>))
          Low::Util::List<DrawCommand>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderScene, pointlight_deleted_slots,
          Low::Util::Set<u32>)) Low::Util::Set<u32>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderScene, directional_light_direction,
          Low::Math::Vector3)) Low::Math::Vector3();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderScene, directional_light_color,
          Low::Math::ColorRGB)) Low::Math::ColorRGB();
      ACCESSOR_TYPE_SOA(l_Handle, RenderScene,
                        directional_light_intensity, float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, RenderScene, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // TODO: Should be moved somewhere else
      l_Handle.set_data_handle(Vulkan::Scene::make(p_Name));

      l_Handle.set_directional_light_intensity(0.0f);
      l_Handle.set_directional_light_color(Math::ColorRGB(1.0f));
      l_Handle.set_directional_light_direction(
          Math::Vector3(0.0f, -1.0f, 0.0f));
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderScene::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<RenderScene> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        for (auto it = get_draw_commands().begin();
             it != get_draw_commands().end(); ++it) {
          if (it->is_alive()) {
            it->destroy();
          }
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

    void RenderScene::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(RenderScene));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, RenderScene::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(RenderScene);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &RenderScene::is_alive;
      l_TypeInfo.destroy = &RenderScene::destroy;
      l_TypeInfo.serialize = &RenderScene::serialize;
      l_TypeInfo.deserialize = &RenderScene::deserialize;
      l_TypeInfo.find_by_index = &RenderScene::_find_by_index;
      l_TypeInfo.notify = &RenderScene::_notify;
      l_TypeInfo.find_by_name = &RenderScene::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &RenderScene::_make;
      l_TypeInfo.duplicate_default = &RenderScene::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &RenderScene::living_instances);
      l_TypeInfo.get_living_count = &RenderScene::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: draw_commands
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_commands);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderScene::Data, draw_commands);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          l_Handle.get_draw_commands();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderScene, draw_commands,
              Low::Util::List<DrawCommand>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          *((Low::Util::List<DrawCommand> *)p_Data) =
              l_Handle.get_draw_commands();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_commands
      }
      {
        // Property: pointlight_deleted_slots
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pointlight_deleted_slots);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderScene::Data, pointlight_deleted_slots);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          l_Handle.get_pointlight_deleted_slots();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderScene,
                                            pointlight_deleted_slots,
                                            Low::Util::Set<u32>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          *((Low::Util::Set<u32> *)p_Data) =
              l_Handle.get_pointlight_deleted_slots();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: pointlight_deleted_slots
      }
      {
        // Property: data_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(data_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderScene::Data, data_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          l_Handle.get_data_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderScene,
                                            data_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderScene l_Handle = p_Handle.get_id();
          l_Handle.set_data_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          *((uint64_t *)p_Data) = l_Handle.get_data_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_handle
      }
      {
        // Property: directional_light_direction
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(directional_light_direction);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderScene::Data, directional_light_direction);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          l_Handle.get_directional_light_direction();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderScene, directional_light_direction,
              Low::Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderScene l_Handle = p_Handle.get_id();
          l_Handle.set_directional_light_direction(
              *(Low::Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          *((Low::Math::Vector3 *)p_Data) =
              l_Handle.get_directional_light_direction();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: directional_light_direction
      }
      {
        // Property: directional_light_color
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(directional_light_color);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderScene::Data, directional_light_color);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::COLORRGB;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          l_Handle.get_directional_light_color();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderScene,
                                            directional_light_color,
                                            Low::Math::ColorRGB);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderScene l_Handle = p_Handle.get_id();
          l_Handle.set_directional_light_color(
              *(Low::Math::ColorRGB *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          *((Low::Math::ColorRGB *)p_Data) =
              l_Handle.get_directional_light_color();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: directional_light_color
      }
      {
        // Property: directional_light_intensity
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(directional_light_intensity);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderScene::Data, directional_light_intensity);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          l_Handle.get_directional_light_intensity();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderScene, directional_light_intensity,
              float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderScene l_Handle = p_Handle.get_id();
          l_Handle.set_directional_light_intensity(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          *((float *)p_Data) =
              l_Handle.get_directional_light_intensity();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: directional_light_intensity
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderScene::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderScene,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderScene l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderScene l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderScene> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: insert_draw_command
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(insert_draw_command);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_DrawCommand);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::DrawCommand::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: insert_draw_command
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderScene::cleanup()
    {
      Low::Util::List<RenderScene> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle RenderScene::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    RenderScene RenderScene::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      RenderScene l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = RenderScene::TYPE_ID;

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

    RenderScene RenderScene::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      RenderScene l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = RenderScene::TYPE_ID;

      return l_Handle;
    }

    bool RenderScene::is_alive() const
    {
      if (m_Data.m_Type != RenderScene::TYPE_ID) {
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
      return m_Data.m_Type == RenderScene::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t RenderScene::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    RenderScene::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    RenderScene RenderScene::find_by_name(Low::Util::Name p_Name)
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

    RenderScene RenderScene::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      RenderScene l_Handle = make(p_Name);
      l_Handle.set_pointlight_deleted_slots(
          get_pointlight_deleted_slots());
      l_Handle.set_data_handle(get_data_handle());
      l_Handle.set_directional_light_direction(
          get_directional_light_direction());
      l_Handle.set_directional_light_color(
          get_directional_light_color());
      l_Handle.set_directional_light_intensity(
          get_directional_light_intensity());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    RenderScene RenderScene::duplicate(RenderScene p_Handle,
                                       Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    RenderScene::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Name p_Name)
    {
      RenderScene l_RenderScene = p_Handle.get_id();
      return l_RenderScene.duplicate(p_Name);
    }

    void RenderScene::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void RenderScene::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
    {
      RenderScene l_RenderScene = p_Handle.get_id();
      l_RenderScene.serialize(p_Node);
    }

    Low::Util::Handle
    RenderScene::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void RenderScene::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 RenderScene::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 RenderScene::observe(Low::Util::Name p_Observable,
                             Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void RenderScene::notify(Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void RenderScene::_notify(Low::Util::Handle p_Observer,
                              Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      RenderScene l_RenderScene = p_Observer.get_id();
      l_RenderScene.notify(p_Observed, p_Observable);
    }

    Low::Util::List<DrawCommand> &
    RenderScene::get_draw_commands() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_commands
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_commands

      return TYPE_SOA(RenderScene, draw_commands,
                      Low::Util::List<DrawCommand>);
    }

    Low::Util::Set<u32> &
    RenderScene::get_pointlight_deleted_slots() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pointlight_deleted_slots
      // LOW_CODEGEN::END::CUSTOM:GETTER_pointlight_deleted_slots

      return TYPE_SOA(RenderScene, pointlight_deleted_slots,
                      Low::Util::Set<u32>);
    }
    void RenderScene::set_pointlight_deleted_slots(
        Low::Util::Set<u32> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pointlight_deleted_slots
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_pointlight_deleted_slots

      // Set new value
      TYPE_SOA(RenderScene, pointlight_deleted_slots,
               Low::Util::Set<u32>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pointlight_deleted_slots
      // LOW_CODEGEN::END::CUSTOM:SETTER_pointlight_deleted_slots

      broadcast_observable(N(pointlight_deleted_slots));
    }

    uint64_t RenderScene::get_data_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_handle

      return TYPE_SOA(RenderScene, data_handle, uint64_t);
    }
    void RenderScene::set_data_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_handle

      // Set new value
      TYPE_SOA(RenderScene, data_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_handle

      broadcast_observable(N(data_handle));
    }

    Low::Math::Vector3 &
    RenderScene::get_directional_light_direction() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_directional_light_direction
      // LOW_CODEGEN::END::CUSTOM:GETTER_directional_light_direction

      return TYPE_SOA(RenderScene, directional_light_direction,
                      Low::Math::Vector3);
    }
    void RenderScene::set_directional_light_direction(float p_X,
                                                      float p_Y,
                                                      float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_directional_light_direction(p_Val);
    }

    void RenderScene::set_directional_light_direction_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_directional_light_direction();
      l_Value.x = p_Value;
      set_directional_light_direction(l_Value);
    }

    void RenderScene::set_directional_light_direction_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_directional_light_direction();
      l_Value.y = p_Value;
      set_directional_light_direction(l_Value);
    }

    void RenderScene::set_directional_light_direction_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_directional_light_direction();
      l_Value.z = p_Value;
      set_directional_light_direction(l_Value);
    }

    void RenderScene::set_directional_light_direction(
        Low::Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_directional_light_direction
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_directional_light_direction

      // Set new value
      TYPE_SOA(RenderScene, directional_light_direction,
               Low::Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_directional_light_direction
      // LOW_CODEGEN::END::CUSTOM:SETTER_directional_light_direction

      broadcast_observable(N(directional_light_direction));
    }

    Low::Math::ColorRGB &
    RenderScene::get_directional_light_color() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_directional_light_color
      // LOW_CODEGEN::END::CUSTOM:GETTER_directional_light_color

      return TYPE_SOA(RenderScene, directional_light_color,
                      Low::Math::ColorRGB);
    }
    void RenderScene::set_directional_light_color(float p_X,
                                                  float p_Y,
                                                  float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_directional_light_color(p_Val);
    }

    void RenderScene::set_directional_light_color_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_directional_light_color();
      l_Value.x = p_Value;
      set_directional_light_color(l_Value);
    }

    void RenderScene::set_directional_light_color_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_directional_light_color();
      l_Value.y = p_Value;
      set_directional_light_color(l_Value);
    }

    void RenderScene::set_directional_light_color_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_directional_light_color();
      l_Value.z = p_Value;
      set_directional_light_color(l_Value);
    }

    void RenderScene::set_directional_light_color(
        Low::Math::ColorRGB &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_directional_light_color
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_directional_light_color

      // Set new value
      TYPE_SOA(RenderScene, directional_light_color,
               Low::Math::ColorRGB) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_directional_light_color
      // LOW_CODEGEN::END::CUSTOM:SETTER_directional_light_color

      broadcast_observable(N(directional_light_color));
    }

    float RenderScene::get_directional_light_intensity() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_directional_light_intensity
      // LOW_CODEGEN::END::CUSTOM:GETTER_directional_light_intensity

      return TYPE_SOA(RenderScene, directional_light_intensity,
                      float);
    }
    void RenderScene::set_directional_light_intensity(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_directional_light_intensity
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_directional_light_intensity

      // Set new value
      TYPE_SOA(RenderScene, directional_light_intensity, float) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_directional_light_intensity
      // LOW_CODEGEN::END::CUSTOM:SETTER_directional_light_intensity

      broadcast_observable(N(directional_light_intensity));
    }

    Low::Util::Name RenderScene::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(RenderScene, name, Low::Util::Name);
    }
    void RenderScene::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(RenderScene, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    bool RenderScene::insert_draw_command(
        Low::Renderer::DrawCommand p_DrawCommand)
    {
      Low::Util::HandleLock<RenderScene> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_insert_draw_command
      _LOW_ASSERT(is_alive());
      _LOW_ASSERT(p_DrawCommand.is_alive());

      if (get_draw_commands().empty()) {
        get_draw_commands().push_back(p_DrawCommand);
        return true;
      }

      u32 l_SavedIndex = 0;
      bool l_IndexFound = false;

      Util::List<DrawCommand> &l_DrawCommands = get_draw_commands();

      for (u32 i = 0; i < l_DrawCommands.size(); ++i) {
        if (!l_DrawCommands[i].is_alive()) {
          continue;
        }
        if (l_DrawCommands[i].get_sort_index() <
            p_DrawCommand.get_sort_index()) {
          l_SavedIndex = i;
          continue;
        } else if (l_DrawCommands[i].get_sort_index() >
                   p_DrawCommand.get_sort_index()) {
          l_IndexFound = true;
          break;
        }
      }

      if (l_IndexFound) {
        get_draw_commands().insert(get_draw_commands().begin() +
                                       l_SavedIndex,
                                   p_DrawCommand);
      } else {
        get_draw_commands().push_back(p_DrawCommand);
      }

      return true;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_insert_draw_command
    }

    uint32_t RenderScene::create_instance(
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

    u32 RenderScene::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for RenderScene.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, RenderScene::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool RenderScene::get_page_for_index(const u32 p_Index,
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
