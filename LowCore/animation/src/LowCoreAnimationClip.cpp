#include "LowCoreAnimationClip.h"

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
#include "LowRendererAnimationClipState.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Animation {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Clip::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Clip::IDENTIFIER(LOW_NAME(1181529166), LOW_NAME(219331417));
      uint32_t Clip::ms_Capacity = 0u;
      uint32_t Clip::ms_PageSize = 0u;
      Low::Util::List<Clip> Clip::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Clip::ms_Pages;

      Low::Util::Handle Clip::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Clip Clip::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Clip l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Clip::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Clip, renderer_clip,
                                   Renderer::AnimationClip))
            Renderer::AnimationClip();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Clip, references,
                                   Low::Util::Set<u64>))
            Low::Util::Set<u64>();
        ACCESSOR_TYPE_SOA(l_Handle, Clip, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Clip::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
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

      void Clip::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                          N(Clip));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Clip));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Clip::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Clip);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Clip::is_alive;
        l_TypeInfo.destroy = &Clip::destroy;
        l_TypeInfo.serialize = &Clip::serialize;
        l_TypeInfo.deserialize = &Clip::deserialize;
        l_TypeInfo.find_by_index = &Clip::_find_by_index;
        l_TypeInfo.notify = &Clip::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Clip::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Clip::_make;
        l_TypeInfo.duplicate_default = &Clip::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Clip::living_instances);
        l_TypeInfo.get_living_count = &Clip::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: renderer_clip
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(renderer_clip);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Clip::Data, renderer_clip);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Renderer::AnimationClip::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Clip l_Handle = p_Handle.get_id();
            l_Handle.get_renderer_clip();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Clip, renderer_clip,
                Renderer::AnimationClip);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Clip l_Handle = p_Handle.get_id();
            *((Renderer::AnimationClip *)p_Data) =
                l_Handle.get_renderer_clip();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: renderer_clip
        }
        {
          // Property: references
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(references);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Clip::Data, references);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
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
          // End property: references
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Clip::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Clip l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Clip, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Clip l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Clip l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: make_from_renderer_clip
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make_from_renderer_clip);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Clip::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_RendererClip);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Renderer::AnimationClip::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make_from_renderer_clip
        }
        {
          // Function: find
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(find);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Clip::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Skeleton);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Renderer::Skeleton::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: find
        }
        {
          // Function: is_loaded
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(is_loaded);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: is_loaded
        }
        {
          // Function: get_duration
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_duration);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_duration
        }
        {
          // Function: get_ticks_per_second
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_ticks_per_second);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_ticks_per_second
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        for (u32 i = 0; i < Renderer::AnimationClip::living_count();
             ++i) {
          Renderer::AnimationClip i_Clip =
              Renderer::AnimationClip::living_instances()[i];

          make_from_renderer_clip(i_Clip);
        }
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Clip::cleanup()
      {
        Low::Util::List<Clip> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Clip::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Clip Clip::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Clip l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Clip::ms_TypeId;

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

      Clip Clip::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Clip l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Clip::ms_TypeId;

        return l_Handle;
      }

      bool Clip::is_alive() const
      {
        if (m_Data.m_Type != Clip::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Clip::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Clip::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Clip::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Clip Clip::find_by_name(Low::Util::Name p_Name)
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

      Clip Clip::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Clip l_Handle = make(p_Name);
        if (get_renderer_clip().is_alive()) {
          l_Handle.set_renderer_clip(get_renderer_clip());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Clip Clip::duplicate(Clip p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Clip::_duplicate(Low::Util::Handle p_Handle,
                                         Low::Util::Name p_Name)
      {
        Clip l_Clip = p_Handle.get_id();
        return l_Clip.duplicate(p_Name);
      }

      void Clip::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        if (get_renderer_clip().is_alive()) {
          get_renderer_clip().serialize(p_Node["renderer_clip"]);
        }
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Clip::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Serial::Node &p_Node)
      {
        Clip l_Clip = p_Handle.get_id();
        l_Clip.serialize(p_Node);
      }

      Low::Util::Handle
      Clip::deserialize(Low::Util::Serial::Node &p_Node,
                        Low::Util::Handle p_Creator)
      {
        Clip l_Handle = Clip::make(N(Clip));

        if (p_Node["renderer_clip"]) {
          l_Handle.set_renderer_clip(
              Renderer::AnimationClip::deserialize(
                  p_Node["renderer_clip"], l_Handle.get_id())
                  .get_id());
        }
        if (p_Node["references"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void
      Clip::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Clip::observe(Low::Util::Name p_Observable,
                        Low::Util::Function<void(Low::Util::Handle,
                                                 Low::Util::Name)>
                            p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Clip::observe(Low::Util::Name p_Observable,
                        Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Clip::notify(Low::Util::Handle p_Observed,
                        Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Clip::_notify(Low::Util::Handle p_Observer,
                         Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        Clip l_Clip = p_Observer.get_id();
        l_Clip.notify(p_Observed, p_Observable);
      }

      void Clip::reference(const u64 p_Id)
      {
        _LOW_ASSERT(is_alive());

        const u32 l_OldReferences =
            (TYPE_SOA(Clip, references, Low::Util::Set<u64>)).size();

        (TYPE_SOA(Clip, references, Low::Util::Set<u64>))
            .insert(p_Id);

        const u32 l_References =
            (TYPE_SOA(Clip, references, Low::Util::Set<u64>)).size();

        if (l_OldReferences != l_References) {
          // LOW_CODEGEN:BEGIN:CUSTOM:NEW_REFERENCE
          get_renderer_clip().reference(p_Id);
          // LOW_CODEGEN::END::CUSTOM:NEW_REFERENCE
        }
      }

      void Clip::dereference(const u64 p_Id)
      {
        _LOW_ASSERT(is_alive());

        const u32 l_OldReferences =
            (TYPE_SOA(Clip, references, Low::Util::Set<u64>)).size();

        (TYPE_SOA(Clip, references, Low::Util::Set<u64>)).erase(p_Id);

        const u32 l_References =
            (TYPE_SOA(Clip, references, Low::Util::Set<u64>)).size();

        if (l_OldReferences != l_References) {
          // LOW_CODEGEN:BEGIN:CUSTOM:REFERENCE_REMOVED
          get_renderer_clip().dereference(p_Id);
          // LOW_CODEGEN::END::CUSTOM:REFERENCE_REMOVED
        }
      }

      u32 Clip::references() const
      {
        return get_references().size();
      }

      bool Clip::is_referenced() const
      {
        return !get_references().empty();
      }

      Renderer::AnimationClip Clip::get_renderer_clip() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderer_clip
        // LOW_CODEGEN::END::CUSTOM:GETTER_renderer_clip

        return TYPE_SOA(Clip, renderer_clip, Renderer::AnimationClip);
      }
      void Clip::set_renderer_clip(Renderer::AnimationClip p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_renderer_clip
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_renderer_clip

        // Set new value
        TYPE_SOA(Clip, renderer_clip, Renderer::AnimationClip) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderer_clip
        // LOW_CODEGEN::END::CUSTOM:SETTER_renderer_clip

        broadcast_observable(N(renderer_clip));
      }

      Low::Util::Set<u64> &Clip::get_references() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
        // LOW_CODEGEN::END::CUSTOM:GETTER_references

        return TYPE_SOA(Clip, references, Low::Util::Set<u64>);
      }

      Low::Util::Name Clip::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Clip, name, Low::Util::Name);
      }
      void Clip::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Clip, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      Clip Clip::make_from_renderer_clip(
          Renderer::AnimationClip p_RendererClip)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_renderer_clip
        if (!p_RendererClip.is_alive()) {
          return Low::Util::Handle::DEAD;
        }

        for (u32 i = 0u; i < living_count(); ++i) {
          Clip i_Clip = living_instances()[i];
          if (i_Clip.get_renderer_clip() == p_RendererClip) {
            return i_Clip;
          }
        }

        Clip l_Clip = make(p_RendererClip.get_name());
        l_Clip.set_renderer_clip(p_RendererClip);
        return l_Clip;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_renderer_clip
      }

      Clip Clip::find(Renderer::Skeleton p_Skeleton,
                      Util::Name p_Name)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find
        if (!p_Skeleton.is_alive()) {
          return Util::Handle::DEAD;
        }

        for (u32 i = 0; i < living_count(); ++i) {
          Clip i_Clip = living_instances()[i];

          Renderer::AnimationClip i_RAC = i_Clip.get_renderer_clip();

          if (!i_RAC.is_alive()) {
            continue;
          }

          if (i_RAC.get_skeleton() == p_Skeleton &&
              i_RAC.get_name() == p_Name) {
            return i_Clip;
          }
        }

        for (u32 i = 0; i < Renderer::AnimationClip::living_count();
             ++i) {
          Renderer::AnimationClip i_RendererClip =
              Renderer::AnimationClip::living_instances()[i];

          if (!i_RendererClip.is_alive()) {
            continue;
          }

          if (i_RendererClip.get_skeleton() == p_Skeleton &&
              i_RendererClip.get_name() == p_Name) {
            return make_from_renderer_clip(i_RendererClip);
          }
        }

        return Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_find
      }

      bool Clip::is_loaded() const
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded
        if (!is_alive() || !get_renderer_clip().is_alive()) {
          return false;
        }

        return get_renderer_clip().get_state() ==
               Renderer::AnimationClipState::LOADED;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
      }

      float Clip::get_duration() const
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_duration
        if (!is_alive() || !get_renderer_clip().is_alive()) {
          return 0.0f;
        }

        return get_renderer_clip().get_duration();
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_duration
      }

      float Clip::get_ticks_per_second() const
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_ticks_per_second
        if (!is_alive() || !get_renderer_clip().is_alive()) {
          return 0.0f;
        }

        return get_renderer_clip().get_ticks_per_second();
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_ticks_per_second
      }

      uint32_t Clip::create_instance(u32 &p_PageIndex,
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
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
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

      u32 Clip::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Clip.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Clip::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Clip::get_page_for_index(const u32 p_Index,
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

    } // namespace Animation
  } // namespace Core
} // namespace Low
