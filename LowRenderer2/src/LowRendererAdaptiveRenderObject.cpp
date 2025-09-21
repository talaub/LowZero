#include "LowRendererAdaptiveRenderObject.h"

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

    const uint16_t AdaptiveRenderObject::TYPE_ID = 90;
    uint32_t AdaptiveRenderObject::ms_Capacity = 0u;
    uint32_t AdaptiveRenderObject::ms_PageSize = 0u;
    Low::Util::SharedMutex AdaptiveRenderObject::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        AdaptiveRenderObject::ms_PagesLock(
            AdaptiveRenderObject::ms_PagesMutex, std::defer_lock);
    Low::Util::List<AdaptiveRenderObject>
        AdaptiveRenderObject::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        AdaptiveRenderObject::ms_Pages;

    AdaptiveRenderObject::AdaptiveRenderObject()
        : Low::Util::Handle(0ull)
    {
    }
    AdaptiveRenderObject::AdaptiveRenderObject(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    AdaptiveRenderObject::AdaptiveRenderObject(
        AdaptiveRenderObject &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle
    AdaptiveRenderObject::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    AdaptiveRenderObject
    AdaptiveRenderObject::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      AdaptiveRenderObject l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = AdaptiveRenderObject::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
          l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, AdaptiveRenderObject,
                                 model, Low::Renderer::Model))
          Low::Renderer::Model();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, AdaptiveRenderObject, world_transform,
          Low::Math::Matrix4x4)) Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, AdaptiveRenderObject,
                                 material, Low::Renderer::Material))
          Low::Renderer::Material();
      ACCESSOR_TYPE_SOA(l_Handle, AdaptiveRenderObject, dirty, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, AdaptiveRenderObject, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void AdaptiveRenderObject::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());
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

    void AdaptiveRenderObject::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(AdaptiveRenderObject));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, AdaptiveRenderObject::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(AdaptiveRenderObject);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &AdaptiveRenderObject::is_alive;
      l_TypeInfo.destroy = &AdaptiveRenderObject::destroy;
      l_TypeInfo.serialize = &AdaptiveRenderObject::serialize;
      l_TypeInfo.deserialize = &AdaptiveRenderObject::deserialize;
      l_TypeInfo.find_by_index =
          &AdaptiveRenderObject::_find_by_index;
      l_TypeInfo.notify = &AdaptiveRenderObject::_notify;
      l_TypeInfo.find_by_name = &AdaptiveRenderObject::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &AdaptiveRenderObject::_make;
      l_TypeInfo.duplicate_default =
          &AdaptiveRenderObject::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &AdaptiveRenderObject::living_instances);
      l_TypeInfo.get_living_count =
          &AdaptiveRenderObject::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: model
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(model);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AdaptiveRenderObject::Data, model);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Model::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_model();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AdaptiveRenderObject, model,
              Low::Renderer::Model);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          *((Low::Renderer::Model *)p_Data) = l_Handle.get_model();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: model
      }
      {
        // Property: world_transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AdaptiveRenderObject::Data, world_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_world_transform();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AdaptiveRenderObject, world_transform,
              Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_world_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_world_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: world_transform
      }
      {
        // Property: render_scene_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_scene_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AdaptiveRenderObject::Data, render_scene_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_render_scene_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AdaptiveRenderObject, render_scene_handle,
              uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(AdaptiveRenderObject::Data, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Material::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_material();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AdaptiveRenderObject, material,
              Low::Renderer::Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_material(*(Low::Renderer::Material *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          *((Low::Renderer::Material *)p_Data) =
              l_Handle.get_material();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material
      }
      {
        // Property: object_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(object_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(AdaptiveRenderObject::Data, object_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_object_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AdaptiveRenderObject, object_id, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_object_id(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(AdaptiveRenderObject::Data, dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.is_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AdaptiveRenderObject, dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(AdaptiveRenderObject::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, AdaptiveRenderObject, name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          AdaptiveRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<AdaptiveRenderObject> l_HandleLock(
              l_Handle);
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
        l_FunctionInfo.handleType = AdaptiveRenderObject::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Scene);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderScene::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Model);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Low::Renderer::Model::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void AdaptiveRenderObject::cleanup()
    {
      Low::Util::List<AdaptiveRenderObject> l_Instances =
          ms_LivingInstances;
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

    Low::Util::Handle
    AdaptiveRenderObject::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    AdaptiveRenderObject
    AdaptiveRenderObject::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      AdaptiveRenderObject l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = AdaptiveRenderObject::TYPE_ID;

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

    AdaptiveRenderObject
    AdaptiveRenderObject::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      AdaptiveRenderObject l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = AdaptiveRenderObject::TYPE_ID;

      return l_Handle;
    }

    bool AdaptiveRenderObject::is_alive() const
    {
      if (m_Data.m_Type != AdaptiveRenderObject::TYPE_ID) {
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
      return m_Data.m_Type == AdaptiveRenderObject::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t AdaptiveRenderObject::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    AdaptiveRenderObject::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    AdaptiveRenderObject
    AdaptiveRenderObject::find_by_name(Low::Util::Name p_Name)
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

    AdaptiveRenderObject
    AdaptiveRenderObject::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      AdaptiveRenderObject l_Handle = make(p_Name);
      if (get_model().is_alive()) {
        l_Handle.set_model(get_model());
      }
      l_Handle.set_world_transform(get_world_transform());
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

    AdaptiveRenderObject
    AdaptiveRenderObject::duplicate(AdaptiveRenderObject p_Handle,
                                    Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    AdaptiveRenderObject::_duplicate(Low::Util::Handle p_Handle,
                                     Low::Util::Name p_Name)
    {
      AdaptiveRenderObject l_AdaptiveRenderObject = p_Handle.get_id();
      return l_AdaptiveRenderObject.duplicate(p_Name);
    }

    void AdaptiveRenderObject::serialize(
        Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      if (get_model().is_alive()) {
        get_model().serialize(p_Node["model"]);
      }
      p_Node["render_scene_handle"] = get_render_scene_handle();
      if (get_material().is_alive()) {
        get_material().serialize(p_Node["material"]);
      }
      p_Node["object_id"] = get_object_id();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void
    AdaptiveRenderObject::serialize(Low::Util::Handle p_Handle,
                                    Low::Util::Yaml::Node &p_Node)
    {
      AdaptiveRenderObject l_AdaptiveRenderObject = p_Handle.get_id();
      l_AdaptiveRenderObject.serialize(p_Node);
    }

    Low::Util::Handle
    AdaptiveRenderObject::deserialize(Low::Util::Yaml::Node &p_Node,
                                      Low::Util::Handle p_Creator)
    {
      AdaptiveRenderObject l_Handle =
          AdaptiveRenderObject::make(N(AdaptiveRenderObject));

      if (p_Node["model"]) {
        l_Handle.set_model(Low::Renderer::Model::deserialize(
                               p_Node["model"], l_Handle.get_id())
                               .get_id());
      }
      if (p_Node["world_transform"]) {
      }
      if (p_Node["render_scene_handle"]) {
        l_Handle.set_render_scene_handle(
            p_Node["render_scene_handle"].as<uint64_t>());
      }
      if (p_Node["material"]) {
        l_Handle.set_material(
            Low::Renderer::Material::deserialize(p_Node["material"],
                                                 l_Handle.get_id())
                .get_id());
      }
      if (p_Node["object_id"]) {
        l_Handle.set_object_id(p_Node["object_id"].as<uint32_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void AdaptiveRenderObject::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 AdaptiveRenderObject::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64
    AdaptiveRenderObject::observe(Low::Util::Name p_Observable,
                                  Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void AdaptiveRenderObject::notify(Low::Util::Handle p_Observed,
                                      Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void AdaptiveRenderObject::_notify(Low::Util::Handle p_Observer,
                                       Low::Util::Handle p_Observed,
                                       Low::Util::Name p_Observable)
    {
      AdaptiveRenderObject l_AdaptiveRenderObject =
          p_Observer.get_id();
      l_AdaptiveRenderObject.notify(p_Observed, p_Observable);
    }

    Low::Renderer::Model AdaptiveRenderObject::get_model() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_model
      // LOW_CODEGEN::END::CUSTOM:GETTER_model

      return TYPE_SOA(AdaptiveRenderObject, model,
                      Low::Renderer::Model);
    }
    void AdaptiveRenderObject::set_model(Low::Renderer::Model p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_model
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_model

      // Set new value
      TYPE_SOA(AdaptiveRenderObject, model, Low::Renderer::Model) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_model
      // LOW_CODEGEN::END::CUSTOM:SETTER_model

      broadcast_observable(N(model));
    }

    Low::Math::Matrix4x4 &
    AdaptiveRenderObject::get_world_transform() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_world_transform

      return TYPE_SOA(AdaptiveRenderObject, world_transform,
                      Low::Math::Matrix4x4);
    }
    void AdaptiveRenderObject::set_world_transform(
        Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_transform

      if (get_world_transform() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(AdaptiveRenderObject, world_transform,
                 Low::Math::Matrix4x4) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_transform
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_transform

        broadcast_observable(N(world_transform));
      }
    }

    uint64_t AdaptiveRenderObject::get_render_scene_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene_handle

      return TYPE_SOA(AdaptiveRenderObject, render_scene_handle,
                      uint64_t);
    }
    void
    AdaptiveRenderObject::set_render_scene_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene_handle

      // Set new value
      TYPE_SOA(AdaptiveRenderObject, render_scene_handle, uint64_t) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene_handle

      broadcast_observable(N(render_scene_handle));
    }

    Low::Renderer::Material AdaptiveRenderObject::get_material() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material
      // LOW_CODEGEN::END::CUSTOM:GETTER_material

      return TYPE_SOA(AdaptiveRenderObject, material,
                      Low::Renderer::Material);
    }
    void AdaptiveRenderObject::set_material(
        Low::Renderer::Material p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

      if (get_material() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(AdaptiveRenderObject, material,
                 Low::Renderer::Material) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
        // LOW_CODEGEN::END::CUSTOM:SETTER_material

        broadcast_observable(N(material));
      }
    }

    uint32_t AdaptiveRenderObject::get_object_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_object_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_object_id

      return TYPE_SOA(AdaptiveRenderObject, object_id, uint32_t);
    }
    void AdaptiveRenderObject::set_object_id(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_object_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_object_id

      if (get_object_id() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(AdaptiveRenderObject, object_id, uint32_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_object_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_object_id

        broadcast_observable(N(object_id));
      }
    }

    bool AdaptiveRenderObject::is_dirty() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

      return TYPE_SOA(AdaptiveRenderObject, dirty, bool);
    }
    void AdaptiveRenderObject::toggle_dirty()
    {
      set_dirty(!is_dirty());
    }

    void AdaptiveRenderObject::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

      // Set new value
      TYPE_SOA(AdaptiveRenderObject, dirty, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

      broadcast_observable(N(dirty));
    }

    void AdaptiveRenderObject::mark_dirty()
    {
      if (!is_dirty()) {
        TYPE_SOA(AdaptiveRenderObject, dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_dirty
      }
    }

    Low::Util::Name AdaptiveRenderObject::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(AdaptiveRenderObject, name, Low::Util::Name);
    }
    void AdaptiveRenderObject::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<AdaptiveRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(AdaptiveRenderObject, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    AdaptiveRenderObject
    AdaptiveRenderObject::make(Low::Renderer::RenderScene p_Scene,
                               Low::Renderer::Model p_Model)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      AdaptiveRenderObject l_Handle = make(p_Model.get_name());
      l_Handle.set_render_scene_handle(p_Scene.get_id());

      return l_Handle;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t AdaptiveRenderObject::create_instance(
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

    u32 AdaptiveRenderObject::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT(
          (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
          "Could not increase capacity for AdaptiveRenderObject.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, AdaptiveRenderObject::Data::get_size(),
          ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool AdaptiveRenderObject::get_page_for_index(const u32 p_Index,
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
