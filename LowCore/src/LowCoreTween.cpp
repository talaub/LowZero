#include "LowCoreTween.h"

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
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 Tween::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        Tween::IDENTIFIER(LOW_NAME(1181529166), LOW_NAME(29744197));
    uint32_t Tween::ms_Capacity = 0u;
    uint32_t Tween::ms_PageSize = 0u;
    Low::Util::List<Tween> Tween::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Tween::ms_Pages;

    Low::Util::Handle Tween::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Tween Tween::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      Tween l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Tween::ms_TypeId;

      ACCESSOR_TYPE_SOA(l_Handle, Tween, current_duration, float) =
          0.0f;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Tween, ease, TweenEase))
          TweenEase();
      ACCESSOR_TYPE_SOA(l_Handle, Tween, full_duration, float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, Tween, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Tween::destroy()
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

    void Tween::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                        N(Tween));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(Tween));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Tween::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Tween);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Tween::is_alive;
      l_TypeInfo.destroy = &Tween::destroy;
      l_TypeInfo.serialize = &Tween::serialize;
      l_TypeInfo.deserialize = &Tween::deserialize;
      l_TypeInfo.find_by_index = &Tween::_find_by_index;
      l_TypeInfo.notify = &Tween::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &Tween::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Tween::_make;
      l_TypeInfo.duplicate_default = &Tween::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Tween::living_instances);
      l_TypeInfo.get_living_count = &Tween::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: current_duration
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(current_duration);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Tween::Data, current_duration);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Tween l_Handle = p_Handle.get_id();
          l_Handle.get_current_duration();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Tween,
                                            current_duration, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Tween l_Handle = p_Handle.get_id();
          l_Handle.set_current_duration(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Tween l_Handle = p_Handle.get_id();
          *((float *)p_Data) = l_Handle.get_current_duration();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: current_duration
      }
      {
        // Property: ease
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(ease);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Tween::Data, ease);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            TweenEaseEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Tween l_Handle = p_Handle.get_id();
          l_Handle.get_ease();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Tween, ease,
                                            TweenEase);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Tween l_Handle = p_Handle.get_id();
          *((TweenEase *)p_Data) = l_Handle.get_ease();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: ease
      }
      {
        // Property: full_duration
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(full_duration);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Tween::Data, full_duration);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Tween l_Handle = p_Handle.get_id();
          l_Handle.get_full_duration();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Tween,
                                            full_duration, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Tween l_Handle = p_Handle.get_id();
          *((float *)p_Data) = l_Handle.get_full_duration();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: full_duration
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Tween::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Tween l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Tween, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Tween l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Tween l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: start
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(start);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Tween::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Time);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Ease);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::ENUM;
          l_ParameterInfo.handleType =
              TweenEaseEnumHelper::get_enum_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: start
      }
      {
        // Function: is_finished
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_finished);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: is_finished
      }
      {
        // Function: get_progress
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_progress);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_progress
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void Tween::cleanup()
    {
      Low::Util::List<Tween> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Tween::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Tween Tween::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Tween l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Tween::ms_TypeId;

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

    Tween Tween::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Tween l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Tween::ms_TypeId;

      return l_Handle;
    }

    bool Tween::is_alive() const
    {
      if (m_Data.m_Type != Tween::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == Tween::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t Tween::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Tween::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Tween Tween::find_by_name(Low::Util::Name p_Name)
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

    Tween Tween::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Tween l_Handle = make(p_Name);
      l_Handle.set_current_duration(get_current_duration());
      l_Handle.set_ease(get_ease());
      l_Handle.set_full_duration(get_full_duration());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Tween Tween::duplicate(Tween p_Handle, Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Tween::_duplicate(Low::Util::Handle p_Handle,
                                        Low::Util::Name p_Name)
    {
      Tween l_Tween = p_Handle.get_id();
      return l_Tween.duplicate(p_Name);
    }

    void Tween::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["current_duration"] = get_current_duration();
      Low::Util::Serial::serialize_enum(
          p_Node["ease"], TweenEaseEnumHelper::get_enum_id(),
          static_cast<uint8_t>(get_ease()));
      p_Node["full_duration"] = get_full_duration();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Tween::serialize(Low::Util::Handle p_Handle,
                          Low::Util::Serial::Node &p_Node)
    {
      Tween l_Tween = p_Handle.get_id();
      l_Tween.serialize(p_Node);
    }

    Low::Util::Handle
    Tween::deserialize(Low::Util::Serial::Node &p_Node,
                       Low::Util::Handle p_Creator)
    {
      Tween l_Handle = Tween::make(N(Tween));

      if (p_Node["current_duration"]) {
        l_Handle.set_current_duration(
            p_Node["current_duration"].as<float>());
      }
      if (p_Node["ease"]) {
        l_Handle.set_ease(static_cast<TweenEase>(
            Low::Util::Serial::deserialize_enum(p_Node["ease"])));
      }
      if (p_Node["full_duration"]) {
        l_Handle.set_full_duration(
            p_Node["full_duration"].as<float>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void
    Tween::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Tween::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Tween::observe(Low::Util::Name p_Observable,
                       Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Tween::notify(Low::Util::Handle p_Observed,
                       Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Tween::_notify(Low::Util::Handle p_Observer,
                        Low::Util::Handle p_Observed,
                        Low::Util::Name p_Observable)
    {
      Tween l_Tween = p_Observer.get_id();
      l_Tween.notify(p_Observed, p_Observable);
    }

    float Tween::get_current_duration() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_current_duration
      // LOW_CODEGEN::END::CUSTOM:GETTER_current_duration

      return TYPE_SOA(Tween, current_duration, float);
    }
    void Tween::set_current_duration(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_current_duration
      const float l_FullDuration = get_full_duration();
      if (l_FullDuration <= 0.0f) {
        p_Value = 0.0f;
      } else {
        p_Value =
            Low::Math::Util::clamp(p_Value, 0.0f, l_FullDuration);
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_current_duration

      // Set new value
      TYPE_SOA(Tween, current_duration, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_current_duration
      // LOW_CODEGEN::END::CUSTOM:SETTER_current_duration

      broadcast_observable(N(current_duration));
    }

    TweenEase Tween::get_ease() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ease
      // LOW_CODEGEN::END::CUSTOM:GETTER_ease

      return TYPE_SOA(Tween, ease, TweenEase);
    }
    void Tween::set_ease(TweenEase p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_ease
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_ease

      // Set new value
      TYPE_SOA(Tween, ease, TweenEase) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_ease
      // LOW_CODEGEN::END::CUSTOM:SETTER_ease

      broadcast_observable(N(ease));
    }

    float Tween::get_full_duration() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_full_duration
      // LOW_CODEGEN::END::CUSTOM:GETTER_full_duration

      return TYPE_SOA(Tween, full_duration, float);
    }
    void Tween::set_full_duration(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_full_duration
      p_Value = LOW_MATH_MAX(p_Value, 0.0f);
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_full_duration

      // Set new value
      TYPE_SOA(Tween, full_duration, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_full_duration
      if (get_current_duration() > p_Value) {
        set_current_duration(p_Value);
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_full_duration

      broadcast_observable(N(full_duration));
    }

    Low::Util::Name Tween::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Tween, name, Low::Util::Name);
    }
    void Tween::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Tween, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Tween Tween::start(const float p_Time, const TweenEase p_Ease)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_start
      Tween l_Tween = make(N(tween));

      l_Tween.set_ease(p_Ease);
      l_Tween.set_full_duration(p_Time);
      l_Tween.set_current_duration(0);

      return l_Tween;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_start
    }

    bool Tween::is_finished() const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_finished
      if (get_full_duration() <= 0.0f) {
        return true;
      }
      return get_current_duration() >= get_full_duration();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_finished
    }

    float Tween::get_progress() const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_progress
      if (get_full_duration() <= 0.0f) {
        return 1.0f;
      }
      return LOW_MATH_MIN(
          get_current_duration() / get_full_duration(), 1.0f);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_progress
    }

    uint32_t Tween::create_instance(u32 &p_PageIndex,
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

    u32 Tween::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Tween.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Tween::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Tween::get_page_for_index(const u32 p_Index,
                                   u32 &p_PageIndex, u32 &p_SlotIndex)
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

  } // namespace Core
} // namespace Low
