#include "LowRendererDrawCommand.h"

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
    Low::Util::Set<Low::Renderer::DrawCommand>
        Low::Renderer::DrawCommand::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t DrawCommand::TYPE_ID = 62;
    uint32_t DrawCommand::ms_Capacity = 0u;
    uint32_t DrawCommand::ms_PageSize = 0u;
    Low::Util::SharedMutex DrawCommand::ms_LivingMutex;
    Low::Util::SharedMutex DrawCommand::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        DrawCommand::ms_PagesLock(DrawCommand::ms_PagesMutex,
                                  std::defer_lock);
    Low::Util::List<DrawCommand> DrawCommand::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        DrawCommand::ms_Pages;

    DrawCommand::DrawCommand() : Low::Util::Handle(0ull)
    {
    }
    DrawCommand::DrawCommand(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    DrawCommand::DrawCommand(DrawCommand &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle DrawCommand::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    DrawCommand DrawCommand::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      DrawCommand l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = DrawCommand::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, DrawCommand, world_transform,
          Low::Math::Matrix4x4)) Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, DrawCommand, submesh,
                                 Low::Renderer::GpuSubmesh))
          Low::Renderer::GpuSubmesh();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, DrawCommand, render_object,
                                 Low::Renderer::RenderObject))
          Low::Renderer::RenderObject();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, DrawCommand, material,
                                 Low::Renderer::Material))
          Low::Renderer::Material();
      ACCESSOR_TYPE_SOA(l_Handle, DrawCommand, uploaded, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, DrawCommand, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_uploaded(false);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void DrawCommand::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<DrawCommand> l_Lock(get_id());
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

    void DrawCommand::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(DrawCommand));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, DrawCommand::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(DrawCommand);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &DrawCommand::is_alive;
      l_TypeInfo.destroy = &DrawCommand::destroy;
      l_TypeInfo.serialize = &DrawCommand::serialize;
      l_TypeInfo.deserialize = &DrawCommand::deserialize;
      l_TypeInfo.find_by_index = &DrawCommand::_find_by_index;
      l_TypeInfo.notify = &DrawCommand::_notify;
      l_TypeInfo.find_by_name = &DrawCommand::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &DrawCommand::_make;
      l_TypeInfo.duplicate_default = &DrawCommand::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &DrawCommand::living_instances);
      l_TypeInfo.get_living_count = &DrawCommand::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: world_transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommand::Data, world_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.get_world_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            world_transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_world_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_world_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: world_transform
      }
      {
        // Property: submesh
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submesh);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommand::Data, submesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::GpuSubmesh::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.get_submesh();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, DrawCommand, submesh,
              Low::Renderer::GpuSubmesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          *((Low::Renderer::GpuSubmesh *)p_Data) =
              l_Handle.get_submesh();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submesh
      }
      {
        // Property: slot
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(slot);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(DrawCommand::Data, slot);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.get_slot();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            slot, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_slot(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_slot();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: slot
      }
      {
        // Property: render_object
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_object);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommand::Data, render_object);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::RenderObject::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.get_render_object();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, DrawCommand, render_object,
              Low::Renderer::RenderObject);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          *((Low::Renderer::RenderObject *)p_Data) =
              l_Handle.get_render_object();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_object
      }
      {
        // Property: material
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommand::Data, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Material::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.get_material();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            material,
                                            Low::Renderer::Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_material(*(Low::Renderer::Material *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          *((Low::Renderer::Material *)p_Data) =
              l_Handle.get_material();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material
      }
      {
        // Property: uploaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommand::Data, uploaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.is_uploaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            uploaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_uploaded();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded
      }
      {
        // Property: render_scene_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_scene_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommand::Data, render_scene_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.get_render_scene_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, DrawCommand, render_scene_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_render_scene_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          *((uint64_t *)p_Data) = l_Handle.get_render_scene_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_scene_handle
      }
      {
        // Property: object_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(object_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommand::Data, object_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.get_object_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            object_id, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_object_id(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_object_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: object_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(DrawCommand::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<DrawCommand> l_HandleLock(l_Handle);
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
        l_FunctionInfo.handleType = DrawCommand::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderObject);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderObject::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderScene);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderScene::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Submesh);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::GpuSubmesh::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: get_sort_index
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_sort_index);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_sort_index
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void DrawCommand::cleanup()
    {
      Low::Util::List<DrawCommand> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle DrawCommand::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    DrawCommand DrawCommand::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      DrawCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = DrawCommand::TYPE_ID;

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

    DrawCommand DrawCommand::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      DrawCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = DrawCommand::TYPE_ID;

      return l_Handle;
    }

    bool DrawCommand::is_alive() const
    {
      if (m_Data.m_Type != DrawCommand::TYPE_ID) {
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
      return m_Data.m_Type == DrawCommand::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t DrawCommand::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    DrawCommand::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    DrawCommand DrawCommand::find_by_name(Low::Util::Name p_Name)
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

    DrawCommand DrawCommand::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      DrawCommand l_Handle = make(p_Name);
      l_Handle.set_world_transform(get_world_transform());
      if (get_submesh().is_alive()) {
        l_Handle.set_submesh(get_submesh());
      }
      l_Handle.set_slot(get_slot());
      if (get_render_object().is_alive()) {
        l_Handle.set_render_object(get_render_object());
      }
      if (get_material().is_alive()) {
        l_Handle.set_material(get_material());
      }
      l_Handle.set_uploaded(is_uploaded());
      l_Handle.set_render_scene_handle(get_render_scene_handle());
      l_Handle.set_object_id(get_object_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    DrawCommand DrawCommand::duplicate(DrawCommand p_Handle,
                                       Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    DrawCommand::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Name p_Name)
    {
      DrawCommand l_DrawCommand = p_Handle.get_id();
      return l_DrawCommand.duplicate(p_Name);
    }

    void DrawCommand::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void DrawCommand::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
    {
      DrawCommand l_DrawCommand = p_Handle.get_id();
      l_DrawCommand.serialize(p_Node);
    }

    Low::Util::Handle
    DrawCommand::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void DrawCommand::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 DrawCommand::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 DrawCommand::observe(Low::Util::Name p_Observable,
                             Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void DrawCommand::notify(Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void DrawCommand::_notify(Low::Util::Handle p_Observer,
                              Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      DrawCommand l_DrawCommand = p_Observer.get_id();
      l_DrawCommand.notify(p_Observed, p_Observable);
    }

    Low::Math::Matrix4x4 &DrawCommand::get_world_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_world_transform

      return TYPE_SOA(DrawCommand, world_transform,
                      Low::Math::Matrix4x4);
    }
    void
    DrawCommand::set_world_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_transform

      // Set new value
      TYPE_SOA(DrawCommand, world_transform, Low::Math::Matrix4x4) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_transform
      if (!get_render_object().is_dirty()) {
        ms_Dirty.insert(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_world_transform

      broadcast_observable(N(world_transform));
    }

    Low::Renderer::GpuSubmesh DrawCommand::get_submesh() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:GETTER_submesh

      return TYPE_SOA(DrawCommand, submesh,
                      Low::Renderer::GpuSubmesh);
    }
    void DrawCommand::set_submesh(Low::Renderer::GpuSubmesh p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submesh

      // Set new value
      TYPE_SOA(DrawCommand, submesh, Low::Renderer::GpuSubmesh) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:SETTER_submesh

      broadcast_observable(N(submesh));
    }

    uint32_t DrawCommand::get_slot() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_slot
      // LOW_CODEGEN::END::CUSTOM:GETTER_slot

      return TYPE_SOA(DrawCommand, slot, uint32_t);
    }
    void DrawCommand::set_slot(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_slot
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_slot

      // Set new value
      TYPE_SOA(DrawCommand, slot, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_slot
      // LOW_CODEGEN::END::CUSTOM:SETTER_slot

      broadcast_observable(N(slot));
    }

    Low::Renderer::RenderObject DrawCommand::get_render_object() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_object

      return TYPE_SOA(DrawCommand, render_object,
                      Low::Renderer::RenderObject);
    }
    void DrawCommand::set_render_object(
        Low::Renderer::RenderObject p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_object

      // Set new value
      TYPE_SOA(DrawCommand, render_object,
               Low::Renderer::RenderObject) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_object

      broadcast_observable(N(render_object));
    }

    Low::Renderer::Material DrawCommand::get_material() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material
      // LOW_CODEGEN::END::CUSTOM:GETTER_material

      return TYPE_SOA(DrawCommand, material, Low::Renderer::Material);
    }
    void DrawCommand::set_material(Low::Renderer::Material p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material
      if (get_material().is_alive()) {
        get_material().dereference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

      // Set new value
      TYPE_SOA(DrawCommand, material, Low::Renderer::Material) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
      if (!get_render_object().is_dirty()) {
        ms_Dirty.insert(get_id());
      }
      if (get_material().is_alive()) {
        get_material().reference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_material

      broadcast_observable(N(material));
    }

    bool DrawCommand::is_uploaded() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded

      return TYPE_SOA(DrawCommand, uploaded, bool);
    }
    void DrawCommand::toggle_uploaded()
    {
      set_uploaded(!is_uploaded());
    }

    void DrawCommand::set_uploaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded

      // Set new value
      TYPE_SOA(DrawCommand, uploaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded

      broadcast_observable(N(uploaded));
    }

    uint64_t DrawCommand::get_render_scene_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene_handle

      return TYPE_SOA(DrawCommand, render_scene_handle, uint64_t);
    }
    void DrawCommand::set_render_scene_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene_handle

      // Set new value
      TYPE_SOA(DrawCommand, render_scene_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene_handle

      broadcast_observable(N(render_scene_handle));
    }

    uint32_t DrawCommand::get_object_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_object_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_object_id

      return TYPE_SOA(DrawCommand, object_id, uint32_t);
    }
    void DrawCommand::set_object_id(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_object_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_object_id

      // Set new value
      TYPE_SOA(DrawCommand, object_id, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_object_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_object_id

      broadcast_observable(N(object_id));
    }

    Low::Util::Name DrawCommand::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(DrawCommand, name, Low::Util::Name);
    }
    void DrawCommand::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(DrawCommand, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    DrawCommand
    DrawCommand::make(Low::Renderer::RenderObject p_RenderObject,
                      Low::Renderer::RenderScene p_RenderScene,
                      Low::Renderer::GpuSubmesh p_Submesh)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      DrawCommand l_DrawCommand =
          DrawCommand::make(p_RenderObject.get_name());

      _LOW_ASSERT(p_RenderScene.is_alive());

      l_DrawCommand.set_render_object(p_RenderObject);
      l_DrawCommand.set_submesh(p_Submesh);
      l_DrawCommand.set_render_scene_handle(p_RenderScene.get_id());

      if (p_RenderObject.is_alive()) {
        l_DrawCommand.set_object_id(p_RenderObject.get_object_id());

        if (!p_RenderObject.is_dirty()) {
          ms_Dirty.insert(l_DrawCommand);
        }
      }

      return l_DrawCommand;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint64_t DrawCommand::get_sort_index() const
    {
      Low::Util::HandleLock<DrawCommand> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_sort_index
      _LOW_ASSERT(is_alive());

      union
      {
        struct
        {
          u32 materialIndex;
          u32 submeshIndex;
        };
        u64 sortIndex;
      } l_SortIndexAssembler;

      l_SortIndexAssembler.materialIndex = get_material().get_index();
      l_SortIndexAssembler.submeshIndex = get_submesh().get_index();

      return l_SortIndexAssembler.sortIndex;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_sort_index
    }

    uint32_t DrawCommand::create_instance(
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

    u32 DrawCommand::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for DrawCommand.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, DrawCommand::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool DrawCommand::get_page_for_index(const u32 p_Index,
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
