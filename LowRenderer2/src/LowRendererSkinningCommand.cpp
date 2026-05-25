#include "LowRendererSkinningCommand.h"

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
#include "LowRendererVulkan.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 SkinningCommand::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        SkinningCommand::IDENTIFIER(LOW_NAME(509652687),
                                    LOW_NAME(2115189742));
    uint32_t SkinningCommand::ms_Capacity = 0u;
    uint32_t SkinningCommand::ms_PageSize = 0u;
    Low::Util::List<SkinningCommand>
        SkinningCommand::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        SkinningCommand::ms_Pages;

    Low::Util::Handle SkinningCommand::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    SkinningCommand SkinningCommand::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      SkinningCommand l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = SkinningCommand::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SkinningCommand, instance,
                                 SkinningInstance))
          SkinningInstance();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SkinningCommand, submesh,
                                 GpuSubmesh)) GpuSubmesh();
      ACCESSOR_TYPE_SOA(l_Handle, SkinningCommand, weights_start,
                        uint32_t) = 0u;
      ACCESSOR_TYPE_SOA(l_Handle, SkinningCommand,
                        skinned_vertex_start, uint32_t) = 0u;
      ACCESSOR_TYPE_SOA(l_Handle, SkinningCommand, vertex_count,
                        uint32_t) = 0u;
      ACCESSOR_TYPE_SOA(l_Handle, SkinningCommand, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void SkinningCommand::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        if (get_vertex_count() > 0u) {
          Vulkan::Global::get_skinned_vertex_output_buffer().free(
              get_skinned_vertex_start(), get_vertex_count());
        }
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

    void SkinningCommand::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(SkinningCommand));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(SkinningCommand));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, SkinningCommand::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SkinningCommand);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SkinningCommand::is_alive;
      l_TypeInfo.destroy = &SkinningCommand::destroy;
      l_TypeInfo.serialize = &SkinningCommand::serialize;
      l_TypeInfo.deserialize = &SkinningCommand::deserialize;
      l_TypeInfo.find_by_index = &SkinningCommand::_find_by_index;
      l_TypeInfo.notify = &SkinningCommand::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &SkinningCommand::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &SkinningCommand::_make;
      l_TypeInfo.duplicate_default = &SkinningCommand::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &SkinningCommand::living_instances);
      l_TypeInfo.get_living_count = &SkinningCommand::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: instance
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(instance);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningCommand::Data, instance);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = SkinningInstance::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.get_instance();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkinningCommand, instance, SkinningInstance);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningCommand l_Handle = p_Handle.get_id();
          *((SkinningInstance *)p_Data) = l_Handle.get_instance();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: instance
      }
      {
        // Property: submesh
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submesh);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningCommand::Data, submesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = GpuSubmesh::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.get_submesh();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningCommand,
                                            submesh, GpuSubmesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningCommand l_Handle = p_Handle.get_id();
          *((GpuSubmesh *)p_Data) = l_Handle.get_submesh();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submesh
      }
      {
        // Property: weights_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(weights_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningCommand::Data, weights_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.get_weights_start();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningCommand,
                                            weights_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.set_weights_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningCommand l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_weights_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: weights_start
      }
      {
        // Property: skinned_vertex_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(skinned_vertex_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningCommand::Data, skinned_vertex_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.get_skinned_vertex_start();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningCommand,
                                            skinned_vertex_start,
                                            uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.set_skinned_vertex_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningCommand l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_skinned_vertex_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: skinned_vertex_start
      }
      {
        // Property: vertex_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningCommand::Data, vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.get_vertex_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningCommand,
                                            vertex_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningCommand l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_vertex_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertex_count
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkinningCommand::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkinningCommand,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkinningCommand l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkinningCommand l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = SkinningCommand::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Instance);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = SkinningInstance::type_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Submesh);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = GpuSubmesh::type_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void SkinningCommand::cleanup()
    {
      Low::Util::List<SkinningCommand> l_Instances =
          ms_LivingInstances;
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

    Low::Util::Handle
    SkinningCommand::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    SkinningCommand SkinningCommand::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      SkinningCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = SkinningCommand::ms_TypeId;

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

    SkinningCommand
    SkinningCommand::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      SkinningCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = SkinningCommand::ms_TypeId;

      return l_Handle;
    }

    bool SkinningCommand::is_alive() const
    {
      if (m_Data.m_Type != SkinningCommand::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == SkinningCommand::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t SkinningCommand::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    SkinningCommand::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    SkinningCommand
    SkinningCommand::find_by_name(Low::Util::Name p_Name)
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

    SkinningCommand
    SkinningCommand::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      SkinningCommand l_Handle = make(p_Name);
      if (get_instance().is_alive()) {
        l_Handle.set_instance(get_instance());
      }
      if (get_submesh().is_alive()) {
        l_Handle.set_submesh(get_submesh());
      }
      l_Handle.set_weights_start(get_weights_start());
      l_Handle.set_vertex_count(get_vertex_count());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    SkinningCommand
    SkinningCommand::duplicate(SkinningCommand p_Handle,
                               Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    SkinningCommand::_duplicate(Low::Util::Handle p_Handle,
                                Low::Util::Name p_Name)
    {
      SkinningCommand l_SkinningCommand = p_Handle.get_id();
      return l_SkinningCommand.duplicate(p_Name);
    }

    void
    SkinningCommand::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void SkinningCommand::serialize(Low::Util::Handle p_Handle,
                                    Low::Util::Serial::Node &p_Node)
    {
      SkinningCommand l_SkinningCommand = p_Handle.get_id();
      l_SkinningCommand.serialize(p_Node);
    }

    Low::Util::Handle
    SkinningCommand::deserialize(Low::Util::Serial::Node &p_Node,
                                 Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void SkinningCommand::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 SkinningCommand::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 SkinningCommand::observe(Low::Util::Name p_Observable,
                                 Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void SkinningCommand::notify(Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void SkinningCommand::_notify(Low::Util::Handle p_Observer,
                                  Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
    {
      SkinningCommand l_SkinningCommand = p_Observer.get_id();
      l_SkinningCommand.notify(p_Observed, p_Observable);
    }

    SkinningInstance SkinningCommand::get_instance() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_instance
      // LOW_CODEGEN::END::CUSTOM:GETTER_instance

      return TYPE_SOA(SkinningCommand, instance, SkinningInstance);
    }
    void SkinningCommand::set_instance(SkinningInstance p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_instance
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_instance

      // Set new value
      TYPE_SOA(SkinningCommand, instance, SkinningInstance) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_instance
      // LOW_CODEGEN::END::CUSTOM:SETTER_instance

      broadcast_observable(N(instance));
    }

    GpuSubmesh SkinningCommand::get_submesh() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:GETTER_submesh

      return TYPE_SOA(SkinningCommand, submesh, GpuSubmesh);
    }
    void SkinningCommand::set_submesh(GpuSubmesh p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submesh

      // Set new value
      TYPE_SOA(SkinningCommand, submesh, GpuSubmesh) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submesh
      if (get_vertex_count() > 0u) {
        Vulkan::Global::get_skinned_vertex_output_buffer().free(
            get_skinned_vertex_start(), get_vertex_count());
      }

      set_vertex_count(0);
      set_weights_start(0);
      set_skinned_vertex_start(0);

      if (p_Value.is_alive()) {
        LOW_ASSERT(p_Value.get_bone_weight_count() ==
                       p_Value.get_vertex_count(),
                   "Skinning command needs one bone weight entry per "
                   "vertex.");

        u32 l_SkinnedVertexStart = 0u;
        LOW_ASSERT(
            Vulkan::Global::get_skinned_vertex_output_buffer()
                .reserve(p_Value.get_vertex_count(),
                         &l_SkinnedVertexStart),
            "Failed to reserve skinned vertex output buffer range.");

        set_vertex_count(p_Value.get_vertex_count());
        set_weights_start(p_Value.get_bone_weight_start());
        set_skinned_vertex_start(l_SkinnedVertexStart);
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_submesh

      broadcast_observable(N(submesh));
    }

    uint32_t SkinningCommand::get_weights_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_weights_start
      // LOW_CODEGEN::END::CUSTOM:GETTER_weights_start

      return TYPE_SOA(SkinningCommand, weights_start, uint32_t);
    }
    void SkinningCommand::set_weights_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_weights_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_weights_start

      // Set new value
      TYPE_SOA(SkinningCommand, weights_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_weights_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_weights_start

      broadcast_observable(N(weights_start));
    }

    uint32_t SkinningCommand::get_skinned_vertex_start() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skinned_vertex_start
      // LOW_CODEGEN::END::CUSTOM:GETTER_skinned_vertex_start

      return TYPE_SOA(SkinningCommand, skinned_vertex_start,
                      uint32_t);
    }
    void SkinningCommand::set_skinned_vertex_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skinned_vertex_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_skinned_vertex_start

      // Set new value
      TYPE_SOA(SkinningCommand, skinned_vertex_start, uint32_t) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skinned_vertex_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_skinned_vertex_start

      broadcast_observable(N(skinned_vertex_start));
    }

    uint32_t SkinningCommand::get_vertex_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_count

      return TYPE_SOA(SkinningCommand, vertex_count, uint32_t);
    }
    void SkinningCommand::set_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_count

      // Set new value
      TYPE_SOA(SkinningCommand, vertex_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_count

      broadcast_observable(N(vertex_count));
    }

    Low::Util::Name SkinningCommand::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(SkinningCommand, name, Low::Util::Name);
    }
    void SkinningCommand::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(SkinningCommand, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    SkinningCommand SkinningCommand::make(SkinningInstance p_Instance,
                                          GpuSubmesh p_Submesh)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      LOW_ASSERT(
          p_Instance.is_alive(),
          "Failed to create skinning command - instance dead.");
      LOW_ASSERT(p_Submesh.is_alive(),
                 "Failed to create skinning command - submesh dead.");

      SkinningCommand l_Command = make(p_Submesh.get_name());
      l_Command.set_submesh(p_Submesh);
      l_Command.set_instance(p_Instance);

      p_Instance.get_skinning_commands().push_back(l_Command);

      return l_Command;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t SkinningCommand::create_instance(u32 &p_PageIndex,
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
      if (!l_FoundIndex) {
        l_SlotIndex = 0;
        l_PageIndex = create_page();
      }
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      return l_Index;
    }

    u32 SkinningCommand::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for SkinningCommand.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, SkinningCommand::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool SkinningCommand::get_page_for_index(const u32 p_Index,
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
