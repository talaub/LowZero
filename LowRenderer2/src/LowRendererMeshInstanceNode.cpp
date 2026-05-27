#include "LowRendererMeshInstanceNode.h"

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

    u16 MeshInstanceNode::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        MeshInstanceNode::IDENTIFIER(LOW_NAME(509652687),
                                     LOW_NAME(2078444829));
    uint32_t MeshInstanceNode::ms_Capacity = 0u;
    uint32_t MeshInstanceNode::ms_PageSize = 0u;
    Low::Util::List<MeshInstanceNode>
        MeshInstanceNode::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        MeshInstanceNode::ms_Pages;

    Low::Util::Handle MeshInstanceNode::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    MeshInstanceNode MeshInstanceNode::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      MeshInstanceNode l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = MeshInstanceNode::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, MeshInstanceNode, world_transform,
          Low::Math::Matrix4x4)) Low::Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MeshInstanceNode,
                                 draw_command, DrawCommand))
          DrawCommand();
      ACCESSOR_TYPE_SOA(l_Handle, MeshInstanceNode, dirty, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, MeshInstanceNode, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MeshInstanceNode::destroy()
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

    void MeshInstanceNode::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(MeshInstanceNode));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(MeshInstanceNode));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, MeshInstanceNode::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MeshInstanceNode);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MeshInstanceNode::is_alive;
      l_TypeInfo.destroy = &MeshInstanceNode::destroy;
      l_TypeInfo.serialize = &MeshInstanceNode::serialize;
      l_TypeInfo.deserialize = &MeshInstanceNode::deserialize;
      l_TypeInfo.find_by_index = &MeshInstanceNode::_find_by_index;
      l_TypeInfo.notify = &MeshInstanceNode::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &MeshInstanceNode::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &MeshInstanceNode::_make;
      l_TypeInfo.duplicate_default = &MeshInstanceNode::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MeshInstanceNode::living_instances);
      l_TypeInfo.get_living_count = &MeshInstanceNode::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: world_transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshInstanceNode::Data, world_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.get_world_transform();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshInstanceNode, world_transform,
              Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.set_world_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_world_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: world_transform
      }
      {
        // Property: parent_index
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(parent_index);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshInstanceNode::Data, parent_index);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.get_parent_index();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshInstanceNode, parent_index, int32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          *((int32_t *)p_Data) = l_Handle.get_parent_index();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: parent_index
      }
      {
        // Property: bone_index
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(bone_index);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshInstanceNode::Data, bone_index);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.get_bone_index();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshInstanceNode, bone_index, int32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          *((int32_t *)p_Data) = l_Handle.get_bone_index();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: bone_index
      }
      {
        // Property: draw_command
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_command);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshInstanceNode::Data, draw_command);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = DrawCommand::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.get_draw_command();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshInstanceNode, draw_command, DrawCommand);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.set_draw_command(*(DrawCommand *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          *((DrawCommand *)p_Data) = l_Handle.get_draw_command();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_command
      }
      {
        // Property: dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshInstanceNode::Data, dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.is_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshInstanceNode, dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.set_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInstanceNode l_Handle = p_Handle.get_id();
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
            offsetof(MeshInstanceNode::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshInstanceNode, name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshInstanceNode l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshInstanceNode l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = MeshInstanceNode::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ParentIndex);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_BoneIndex);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
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

    void MeshInstanceNode::cleanup()
    {
      Low::Util::List<MeshInstanceNode> l_Instances =
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
    MeshInstanceNode::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    MeshInstanceNode MeshInstanceNode::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      MeshInstanceNode l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = MeshInstanceNode::ms_TypeId;

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

    MeshInstanceNode
    MeshInstanceNode::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      MeshInstanceNode l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = MeshInstanceNode::ms_TypeId;

      return l_Handle;
    }

    bool MeshInstanceNode::is_alive() const
    {
      if (m_Data.m_Type != MeshInstanceNode::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == MeshInstanceNode::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t MeshInstanceNode::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    MeshInstanceNode::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    MeshInstanceNode
    MeshInstanceNode::find_by_name(Low::Util::Name p_Name)
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

    MeshInstanceNode
    MeshInstanceNode::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      MeshInstanceNode l_Handle = make(p_Name);
      l_Handle.set_world_transform(get_world_transform());
      l_Handle.set_parent_index(get_parent_index());
      l_Handle.set_bone_index(get_bone_index());
      if (get_draw_command().is_alive()) {
        l_Handle.set_draw_command(get_draw_command());
      }
      l_Handle.set_dirty(is_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    MeshInstanceNode
    MeshInstanceNode::duplicate(MeshInstanceNode p_Handle,
                                Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    MeshInstanceNode::_duplicate(Low::Util::Handle p_Handle,
                                 Low::Util::Name p_Name)
    {
      MeshInstanceNode l_MeshInstanceNode = p_Handle.get_id();
      return l_MeshInstanceNode.duplicate(p_Name);
    }

    void
    MeshInstanceNode::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void MeshInstanceNode::serialize(Low::Util::Handle p_Handle,
                                     Low::Util::Serial::Node &p_Node)
    {
      MeshInstanceNode l_MeshInstanceNode = p_Handle.get_id();
      l_MeshInstanceNode.serialize(p_Node);
    }

    Low::Util::Handle
    MeshInstanceNode::deserialize(Low::Util::Serial::Node &p_Node,
                                  Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void MeshInstanceNode::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 MeshInstanceNode::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 MeshInstanceNode::observe(Low::Util::Name p_Observable,
                                  Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void MeshInstanceNode::notify(Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void MeshInstanceNode::_notify(Low::Util::Handle p_Observer,
                                   Low::Util::Handle p_Observed,
                                   Low::Util::Name p_Observable)
    {
      MeshInstanceNode l_MeshInstanceNode = p_Observer.get_id();
      l_MeshInstanceNode.notify(p_Observed, p_Observable);
    }

    Low::Math::Matrix4x4 &
    MeshInstanceNode::get_world_transform() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_world_transform

      return TYPE_SOA(MeshInstanceNode, world_transform,
                      Low::Math::Matrix4x4);
    }
    void MeshInstanceNode::set_world_transform(
        Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_transform

      if (get_world_transform() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(MeshInstanceNode, world_transform,
                 Low::Math::Matrix4x4) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_transform
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_transform

        broadcast_observable(N(world_transform));
      }
    }

    int32_t MeshInstanceNode::get_parent_index() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent_index
      // LOW_CODEGEN::END::CUSTOM:GETTER_parent_index

      return TYPE_SOA(MeshInstanceNode, parent_index, int32_t);
    }
    void MeshInstanceNode::set_parent_index(int32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent_index
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent_index

      // Set new value
      TYPE_SOA(MeshInstanceNode, parent_index, int32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent_index
      // LOW_CODEGEN::END::CUSTOM:SETTER_parent_index

      broadcast_observable(N(parent_index));
    }

    int32_t MeshInstanceNode::get_bone_index() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_bone_index
      // LOW_CODEGEN::END::CUSTOM:GETTER_bone_index

      return TYPE_SOA(MeshInstanceNode, bone_index, int32_t);
    }
    void MeshInstanceNode::set_bone_index(int32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_bone_index
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_bone_index

      // Set new value
      TYPE_SOA(MeshInstanceNode, bone_index, int32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_bone_index
      // LOW_CODEGEN::END::CUSTOM:SETTER_bone_index

      broadcast_observable(N(bone_index));
    }

    DrawCommand MeshInstanceNode::get_draw_command() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_command
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_command

      return TYPE_SOA(MeshInstanceNode, draw_command, DrawCommand);
    }
    void MeshInstanceNode::set_draw_command(DrawCommand p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_draw_command
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_draw_command

      // Set new value
      TYPE_SOA(MeshInstanceNode, draw_command, DrawCommand) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_draw_command
      // LOW_CODEGEN::END::CUSTOM:SETTER_draw_command

      broadcast_observable(N(draw_command));
    }

    bool MeshInstanceNode::is_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

      return TYPE_SOA(MeshInstanceNode, dirty, bool);
    }
    void MeshInstanceNode::toggle_dirty()
    {
      set_dirty(!is_dirty());
    }

    void MeshInstanceNode::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

      // Set new value
      TYPE_SOA(MeshInstanceNode, dirty, bool) = p_Value;

      if (p_Value) {
        mark_dirty();
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

      broadcast_observable(N(dirty));
    }

    void MeshInstanceNode::mark_dirty()
    {
      if (!is_dirty()) {
        TYPE_SOA(MeshInstanceNode, dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_dirty
      }
    }

    Low::Util::Name MeshInstanceNode::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(MeshInstanceNode, name, Low::Util::Name);
    }
    void MeshInstanceNode::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(MeshInstanceNode, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    MeshInstanceNode MeshInstanceNode::make(Util::Name p_Name,
                                            int32_t p_ParentIndex,
                                            int32_t p_BoneIndex)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      MeshInstanceNode l_Node = make(p_Name);
      l_Node.set_parent_index(p_ParentIndex);
      l_Node.set_bone_index(p_BoneIndex);

      return l_Node;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t MeshInstanceNode::create_instance(u32 &p_PageIndex,
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

    u32 MeshInstanceNode::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for MeshInstanceNode.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, MeshInstanceNode::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool MeshInstanceNode::get_page_for_index(const u32 p_Index,
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
