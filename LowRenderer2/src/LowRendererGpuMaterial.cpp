#include "LowRendererGpuMaterial.h"

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

#include "LowRendererGlobals.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 GpuMaterial::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        GpuMaterial::IDENTIFIER(LOW_NAME(509652687),
                                LOW_NAME(946042408));
    uint32_t GpuMaterial::ms_Capacity = 0u;
    uint32_t GpuMaterial::ms_PageSize = 0u;
    Low::Util::List<GpuMaterial> GpuMaterial::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        GpuMaterial::ms_Pages;

    Low::Util::Handle GpuMaterial::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GpuMaterial GpuMaterial::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      GpuMaterial l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = GpuMaterial::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, GpuMaterial, data,
                                 Util::List<uint8_t>))
          Util::List<uint8_t>();
      ACCESSOR_TYPE_SOA(l_Handle, GpuMaterial, dirty, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, GpuMaterial, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      l_Handle.data().resize(MATERIAL_DATA_SIZE);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GpuMaterial::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
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

    void GpuMaterial::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(GpuMaterial));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(GpuMaterial));

      ms_PageSize = ms_Capacity;
      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, GpuMaterial::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GpuMaterial);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GpuMaterial::is_alive;
      l_TypeInfo.destroy = &GpuMaterial::destroy;
      l_TypeInfo.serialize = &GpuMaterial::serialize;
      l_TypeInfo.deserialize = &GpuMaterial::deserialize;
      l_TypeInfo.find_by_index = &GpuMaterial::_find_by_index;
      l_TypeInfo.notify = &GpuMaterial::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &GpuMaterial::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GpuMaterial::_make;
      l_TypeInfo.duplicate_default = &GpuMaterial::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GpuMaterial::living_instances);
      l_TypeInfo.get_living_count = &GpuMaterial::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: material_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuMaterial::Data, material_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMaterial l_Handle = p_Handle.get_id();
          l_Handle.get_material_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GpuMaterial, material_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMaterial l_Handle = p_Handle.get_id();
          l_Handle.set_material_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMaterial l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_material_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material_handle
      }
      {
        // Property: data
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(data);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GpuMaterial::Data, data);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data
      }
      {
        // Property: dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GpuMaterial::Data, dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMaterial l_Handle = p_Handle.get_id();
          l_Handle.is_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuMaterial,
                                            dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMaterial l_Handle = p_Handle.get_id();
          l_Handle.set_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMaterial l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(GpuMaterial::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          GpuMaterial l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GpuMaterial,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GpuMaterial l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          GpuMaterial l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: get_data
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_data);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_data
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void GpuMaterial::cleanup()
    {
      Low::Util::List<GpuMaterial> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle GpuMaterial::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GpuMaterial GpuMaterial::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GpuMaterial l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = GpuMaterial::ms_TypeId;

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

    GpuMaterial GpuMaterial::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      GpuMaterial l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = GpuMaterial::ms_TypeId;

      return l_Handle;
    }

    bool GpuMaterial::is_alive() const
    {
      if (m_Data.m_Type != GpuMaterial::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == GpuMaterial::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t GpuMaterial::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    GpuMaterial::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    GpuMaterial GpuMaterial::find_by_name(Low::Util::Name p_Name)
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

    GpuMaterial GpuMaterial::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      GpuMaterial l_Handle = make(p_Name);
      l_Handle.set_material_handle(get_material_handle());
      l_Handle.set_dirty(is_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    GpuMaterial GpuMaterial::duplicate(GpuMaterial p_Handle,
                                       Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    GpuMaterial::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Name p_Name)
    {
      GpuMaterial l_GpuMaterial = p_Handle.get_id();
      return l_GpuMaterial.duplicate(p_Name);
    }

    void GpuMaterial::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void GpuMaterial::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Serial::Node &p_Node)
    {
      GpuMaterial l_GpuMaterial = p_Handle.get_id();
      l_GpuMaterial.serialize(p_Node);
    }

    Low::Util::Handle
    GpuMaterial::deserialize(Low::Util::Serial::Node &p_Node,
                             Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void GpuMaterial::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 GpuMaterial::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 GpuMaterial::observe(Low::Util::Name p_Observable,
                             Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void GpuMaterial::notify(Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void GpuMaterial::_notify(Low::Util::Handle p_Observer,
                              Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      GpuMaterial l_GpuMaterial = p_Observer.get_id();
      l_GpuMaterial.notify(p_Observed, p_Observable);
    }

    uint64_t GpuMaterial::get_material_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material_handle

      // LOW_CODEGEN::END::CUSTOM:GETTER_material_handle

      return TYPE_SOA(GpuMaterial, material_handle, uint64_t);
    }
    void GpuMaterial::set_material_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material_handle

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material_handle

      // Set new value
      TYPE_SOA(GpuMaterial, material_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_handle

      // LOW_CODEGEN::END::CUSTOM:SETTER_material_handle

      broadcast_observable(N(material_handle));
    }

    Util::List<uint8_t> &GpuMaterial::data() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data

      // LOW_CODEGEN::END::CUSTOM:GETTER_data

      return TYPE_SOA(GpuMaterial, data, Util::List<uint8_t>);
    }

    bool GpuMaterial::is_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty

      // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

      return TYPE_SOA(GpuMaterial, dirty, bool);
    }
    void GpuMaterial::toggle_dirty()
    {
      set_dirty(!is_dirty());
    }

    void GpuMaterial::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

      // Set new value
      TYPE_SOA(GpuMaterial, dirty, bool) = p_Value;

      if (p_Value) {
        mark_dirty();
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty

      // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

      broadcast_observable(N(dirty));
    }

    void GpuMaterial::mark_dirty()
    {
      if (!is_dirty()) {
        TYPE_SOA(GpuMaterial, dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty

        // LOW_CODEGEN::END::CUSTOM:MARK_dirty
      }
    }

    Low::Util::Name GpuMaterial::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GpuMaterial, name, Low::Util::Name);
    }
    void GpuMaterial::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GpuMaterial, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    void *GpuMaterial::get_data() const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_data

      return data().data();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_data
    }

    uint32_t GpuMaterial::create_instance(u32 &p_PageIndex,
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
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
            l_FoundIndex = true;
            break;
          }
          l_Index++;
        }
        if (l_FoundIndex) {
          break;
        }
      }
      LOW_ASSERT(l_FoundIndex, "Budget blown for type GpuMaterial");
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      return l_Index;
    }

    u32 GpuMaterial::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for GpuMaterial.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, GpuMaterial::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool GpuMaterial::get_page_for_index(const u32 p_Index,
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
