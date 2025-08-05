#include "LowRendererRenderScene.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
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
    uint8_t *RenderScene::ms_Buffer = 0;
    std::shared_mutex RenderScene::ms_BufferMutex;
    Low::Util::Instances::Slot *RenderScene::ms_Slots = 0;
    Low::Util::List<RenderScene> RenderScene::ms_LivingInstances =
        Low::Util::List<RenderScene>();

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
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      RenderScene l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderScene::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderScene, draw_commands,
                              Low::Util::List<DrawCommand>))
          Low::Util::List<DrawCommand>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, RenderScene, pointlight_deleted_slots,
          Low::Util::Set<u32>)) Low::Util::Set<u32>();
      ACCESSOR_TYPE_SOA(l_Handle, RenderScene, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // TODO: Should be moved somewhere else
      l_Handle.set_data_handle(Vulkan::Scene::make(p_Name));
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderScene::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const RenderScene *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void RenderScene::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(RenderScene));

      initialize_buffer(&ms_Buffer, RenderSceneData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_RenderScene);
      LOW_PROFILE_ALLOC(type_slots_RenderScene);

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
            offsetof(RenderSceneData, draw_commands);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
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
            offsetof(RenderSceneData, pointlight_deleted_slots);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
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
            offsetof(RenderSceneData, data_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
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
          *((uint64_t *)p_Data) = l_Handle.get_data_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_handle
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderSceneData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderScene l_Handle = p_Handle.get_id();
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
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_RenderScene);
      LOW_PROFILE_FREE(type_slots_RenderScene);
      LOCK_UNLOCK(l_Lock);
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
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderScene::TYPE_ID;

      return l_Handle;
    }

    bool RenderScene::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == RenderScene::TYPE_ID &&
             check_alive(ms_Slots, RenderScene::get_capacity());
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
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    RenderScene RenderScene::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      RenderScene l_Handle = make(p_Name);
      l_Handle.set_pointlight_deleted_slots(
          get_pointlight_deleted_slots());
      l_Handle.set_data_handle(get_data_handle());

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

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_commands
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_commands

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderScene, draw_commands,
                      Low::Util::List<DrawCommand>);
    }

    Low::Util::Set<u32> &
    RenderScene::get_pointlight_deleted_slots() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pointlight_deleted_slots
      // LOW_CODEGEN::END::CUSTOM:GETTER_pointlight_deleted_slots

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderScene, pointlight_deleted_slots,
                      Low::Util::Set<u32>);
    }
    void RenderScene::set_pointlight_deleted_slots(
        Low::Util::Set<u32> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pointlight_deleted_slots
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_pointlight_deleted_slots

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderScene, pointlight_deleted_slots,
               Low::Util::Set<u32>) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pointlight_deleted_slots
      // LOW_CODEGEN::END::CUSTOM:SETTER_pointlight_deleted_slots

      broadcast_observable(N(pointlight_deleted_slots));
    }

    uint64_t RenderScene::get_data_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderScene, data_handle, uint64_t);
    }
    void RenderScene::set_data_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderScene, data_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_handle

      broadcast_observable(N(data_handle));
    }

    Low::Util::Name RenderScene::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderScene, name, Low::Util::Name);
    }
    void RenderScene::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderScene, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    bool RenderScene::insert_draw_command(
        Low::Renderer::DrawCommand p_DrawCommand)
    {
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

    uint32_t RenderScene::create_instance()
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

    void RenderScene::increase_budget()
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
                            sizeof(RenderSceneData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          RenderScene i_RenderScene = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(RenderSceneData, draw_commands) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<DrawCommand>))])
              Low::Util::List<DrawCommand>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(i_RenderScene, RenderScene,
                                        draw_commands,
                                        Low::Util::List<DrawCommand>);
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          RenderScene i_RenderScene = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(RenderSceneData,
                                    pointlight_deleted_slots) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::Set<u32>))])
              Low::Util::Set<u32>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(i_RenderScene, RenderScene,
                                        pointlight_deleted_slots,
                                        Low::Util::Set<u32>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderSceneData, data_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderSceneData, data_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderSceneData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderSceneData, name) *
                          (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for RenderScene from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
