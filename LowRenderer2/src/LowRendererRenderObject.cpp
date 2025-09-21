#include "LowRendererRenderObject.h"

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
#include "LowRendererDrawCommand.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    Low::Util::Set<Low::Renderer::RenderObject>
        Low::Renderer::RenderObject::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t RenderObject::TYPE_ID = 52;
    uint32_t RenderObject::ms_Capacity = 0u;
    uint32_t RenderObject::ms_PageSize = 0u;
    Low::Util::SharedMutex RenderObject::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        RenderObject::ms_PagesLock(RenderObject::ms_PagesMutex,
                                   std::defer_lock);
    Low::Util::List<RenderObject> RenderObject::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        RenderObject::ms_Pages;

    RenderObject::RenderObject() : Low::Util::Handle(0ull)
    {
    }
    RenderObject::RenderObject(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    RenderObject::RenderObject(RenderObject &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle RenderObject::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    RenderObject RenderObject::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      RenderObject l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = RenderObject::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderObject, world_transform,
          Low::Math::Matrix4x4)) Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderObject, mesh,
                                 Low::Renderer::Mesh))
          Low::Renderer::Mesh();
      ACCESSOR_TYPE_SOA(l_Handle, RenderObject, uploaded, bool) =
          false;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderObject, material,
                                 Low::Renderer::Material))
          Low::Renderer::Material();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderObject,
                                 draw_commands,
                                 Low::Util::List<DrawCommand>))
          Low::Util::List<DrawCommand>();
      ACCESSOR_TYPE_SOA(l_Handle, RenderObject, dirty, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, RenderObject, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_dirty(true);
      l_Handle.set_uploaded(false);
      l_Handle.set_object_id(LOW_UINT32_MAX);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderObject::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<RenderObject> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        for (auto it = get_draw_commands().begin();
             it != get_draw_commands().end(); ++it) {
          if (it->is_alive()) {
            it->destroy();
          }
        }

        if (get_mesh().is_alive()) {
          get_mesh().dereference(get_id());
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

    void RenderObject::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(RenderObject));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, RenderObject::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(RenderObject);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &RenderObject::is_alive;
      l_TypeInfo.destroy = &RenderObject::destroy;
      l_TypeInfo.serialize = &RenderObject::serialize;
      l_TypeInfo.deserialize = &RenderObject::deserialize;
      l_TypeInfo.find_by_index = &RenderObject::_find_by_index;
      l_TypeInfo.notify = &RenderObject::_notify;
      l_TypeInfo.find_by_name = &RenderObject::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &RenderObject::_make;
      l_TypeInfo.duplicate_default = &RenderObject::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &RenderObject::living_instances);
      l_TypeInfo.get_living_count = &RenderObject::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: world_transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, world_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.get_world_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            world_transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_world_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_world_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: world_transform
      }
      {
        // Property: mesh
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mesh);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, mesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Mesh::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.get_mesh();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderObject, mesh, Low::Renderer::Mesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((Low::Renderer::Mesh *)p_Data) = l_Handle.get_mesh();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mesh
      }
      {
        // Property: uploaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, uploaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.is_uploaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            uploaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_uploaded();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded
      }
      {
        // Property: slot
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(slot);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, slot);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.get_slot();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            slot, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_slot(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_slot();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: slot
      }
      {
        // Property: render_scene_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_scene_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, render_scene_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.get_render_scene_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderObject, render_scene_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((uint64_t *)p_Data) = l_Handle.get_render_scene_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_scene_handle
      }
      {
        // Property: material
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Material::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.get_material();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            material,
                                            Low::Renderer::Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_material(*(Low::Renderer::Material *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((Low::Renderer::Material *)p_Data) =
              l_Handle.get_material();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material
      }
      {
        // Property: draw_commands
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_commands);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, draw_commands);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.get_draw_commands();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderObject, draw_commands,
              Low::Util::List<DrawCommand>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((Low::Util::List<DrawCommand> *)p_Data) =
              l_Handle.get_draw_commands();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_commands
      }
      {
        // Property: object_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(object_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, object_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.get_object_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            object_id, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_object_id(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_object_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: object_id
      }
      {
        // Property: dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.is_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dirty
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObject::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderObject> l_HandleLock(l_Handle);
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
        l_FunctionInfo.handleType = RenderObject::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderScene);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderScene::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Mesh);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Low::Renderer::Mesh::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderObject::cleanup()
    {
      Low::Util::List<RenderObject> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle RenderObject::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    RenderObject RenderObject::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      RenderObject l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = RenderObject::TYPE_ID;

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

    RenderObject RenderObject::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      RenderObject l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = RenderObject::TYPE_ID;

      return l_Handle;
    }

    bool RenderObject::is_alive() const
    {
      if (m_Data.m_Type != RenderObject::TYPE_ID) {
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
      return m_Data.m_Type == RenderObject::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t RenderObject::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    RenderObject::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    RenderObject RenderObject::find_by_name(Low::Util::Name p_Name)
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

    RenderObject RenderObject::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      RenderObject l_Handle = make(p_Name);
      l_Handle.set_world_transform(get_world_transform());
      if (get_mesh().is_alive()) {
        l_Handle.set_mesh(get_mesh());
      }
      l_Handle.set_uploaded(is_uploaded());
      l_Handle.set_slot(get_slot());
      l_Handle.set_render_scene_handle(get_render_scene_handle());
      if (get_material().is_alive()) {
        l_Handle.set_material(get_material());
      }
      l_Handle.set_object_id(get_object_id());
      l_Handle.set_dirty(is_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    RenderObject RenderObject::duplicate(RenderObject p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    RenderObject::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      RenderObject l_RenderObject = p_Handle.get_id();
      return l_RenderObject.duplicate(p_Name);
    }

    void RenderObject::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void RenderObject::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      RenderObject l_RenderObject = p_Handle.get_id();
      l_RenderObject.serialize(p_Node);
    }

    Low::Util::Handle
    RenderObject::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void RenderObject::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 RenderObject::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 RenderObject::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void RenderObject::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void RenderObject::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      RenderObject l_RenderObject = p_Observer.get_id();
      l_RenderObject.notify(p_Observed, p_Observable);
    }

    Low::Math::Matrix4x4 &RenderObject::get_world_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_world_transform

      return TYPE_SOA(RenderObject, world_transform,
                      Low::Math::Matrix4x4);
    }
    void
    RenderObject::set_world_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_transform

      if (get_world_transform() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(RenderObject, world_transform,
                 Low::Math::Matrix4x4) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_transform
        ms_Dirty.insert(get_id());
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_transform

        broadcast_observable(N(world_transform));
      }
    }

    Low::Renderer::Mesh RenderObject::get_mesh() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mesh
      // LOW_CODEGEN::END::CUSTOM:GETTER_mesh

      return TYPE_SOA(RenderObject, mesh, Low::Renderer::Mesh);
    }
    void RenderObject::set_mesh(Low::Renderer::Mesh p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mesh
      if (get_mesh().is_alive()) {
        get_mesh().dereference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mesh

      // Set new value
      TYPE_SOA(RenderObject, mesh, Low::Renderer::Mesh) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh
      if (p_Value.is_alive()) {
        p_Value.reference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_mesh

      broadcast_observable(N(mesh));
    }

    bool RenderObject::is_uploaded() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded

      return TYPE_SOA(RenderObject, uploaded, bool);
    }
    void RenderObject::toggle_uploaded()
    {
      set_uploaded(!is_uploaded());
    }

    void RenderObject::set_uploaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded

      // Set new value
      TYPE_SOA(RenderObject, uploaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded

      broadcast_observable(N(uploaded));
    }

    uint32_t RenderObject::get_slot() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_slot
      // LOW_CODEGEN::END::CUSTOM:GETTER_slot

      return TYPE_SOA(RenderObject, slot, uint32_t);
    }
    void RenderObject::set_slot(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_slot
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_slot

      // Set new value
      TYPE_SOA(RenderObject, slot, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_slot
      // LOW_CODEGEN::END::CUSTOM:SETTER_slot

      broadcast_observable(N(slot));
    }

    uint64_t RenderObject::get_render_scene_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene_handle

      return TYPE_SOA(RenderObject, render_scene_handle, uint64_t);
    }
    void RenderObject::set_render_scene_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene_handle

      // Set new value
      TYPE_SOA(RenderObject, render_scene_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene_handle

      broadcast_observable(N(render_scene_handle));
    }

    Low::Renderer::Material RenderObject::get_material() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material
      // LOW_CODEGEN::END::CUSTOM:GETTER_material

      return TYPE_SOA(RenderObject, material,
                      Low::Renderer::Material);
    }
    void RenderObject::set_material(Low::Renderer::Material p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material
      Material l_OldMaterial = get_material();
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

      if (get_material() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(RenderObject, material, Low::Renderer::Material) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
        if (l_OldMaterial.is_alive()) {
          l_OldMaterial.dereference(get_id());
        }
        if (get_material().is_alive()) {
          get_material().reference(get_id());
        }
        ms_Dirty.insert(get_id());
        // LOW_CODEGEN::END::CUSTOM:SETTER_material

        broadcast_observable(N(material));
      }
    }

    Low::Util::List<DrawCommand> &
    RenderObject::get_draw_commands() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_commands
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_commands

      return TYPE_SOA(RenderObject, draw_commands,
                      Low::Util::List<DrawCommand>);
    }

    uint32_t RenderObject::get_object_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_object_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_object_id

      return TYPE_SOA(RenderObject, object_id, uint32_t);
    }
    void RenderObject::set_object_id(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_object_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_object_id

      if (get_object_id() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(RenderObject, object_id, uint32_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_object_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_object_id

        broadcast_observable(N(object_id));
      }
    }

    bool RenderObject::is_dirty() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

      return TYPE_SOA(RenderObject, dirty, bool);
    }
    void RenderObject::toggle_dirty()
    {
      set_dirty(!is_dirty());
    }

    void RenderObject::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

      // Set new value
      TYPE_SOA(RenderObject, dirty, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
      if (p_Value) {
        ms_Dirty.insert(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

      broadcast_observable(N(dirty));
    }

    void RenderObject::mark_dirty()
    {
      if (!is_dirty()) {
        TYPE_SOA(RenderObject, dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_dirty
      }
    }

    Low::Util::Name RenderObject::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(RenderObject, name, Low::Util::Name);
    }
    void RenderObject::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(RenderObject, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    RenderObject RenderObject::make(RenderScene p_RenderScene,
                                    Low::Renderer::Mesh p_Mesh)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      _LOW_ASSERT(p_RenderScene.is_alive());

      RenderObject l_Handle = make(N(RenderObject));
      l_Handle.set_render_scene_handle(p_RenderScene.get_id());
      l_Handle.set_mesh(p_Mesh);

      LOW_ASSERT(
          p_Mesh.is_alive(),
          "Cannot initialize render object without valid mesh");

      return l_Handle;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t RenderObject::create_instance(
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

    u32 RenderObject::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for RenderObject.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, RenderObject::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool RenderObject::get_page_for_index(const u32 p_Index,
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
