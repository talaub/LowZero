#include "LowRendererComputeStepConfig.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowRendererComputeStep.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t ComputeStepConfig::TYPE_ID = 11;
    uint32_t ComputeStepConfig::ms_Capacity = 0u;
    uint32_t ComputeStepConfig::ms_PageSize = 0u;
    Low::Util::SharedMutex ComputeStepConfig::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        ComputeStepConfig::ms_PagesLock(
            ComputeStepConfig::ms_PagesMutex, std::defer_lock);
    Low::Util::List<ComputeStepConfig>
        ComputeStepConfig::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        ComputeStepConfig::ms_Pages;

    ComputeStepConfig::ComputeStepConfig() : Low::Util::Handle(0ull)
    {
    }
    ComputeStepConfig::ComputeStepConfig(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    ComputeStepConfig::ComputeStepConfig(ComputeStepConfig &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle ComputeStepConfig::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    ComputeStepConfig ComputeStepConfig::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      ComputeStepConfig l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = ComputeStepConfig::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ComputeStepConfig,
                                 callbacks, ComputeStepCallbacks))
          ComputeStepCallbacks();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, ComputeStepConfig, resources,
          Util::List<ResourceConfig>)) Util::List<ResourceConfig>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ComputeStepConfig,
                                 pipelines,
                                 Util::List<ComputePipelineConfig>))
          Util::List<ComputePipelineConfig>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, ComputeStepConfig,
                                 output_image,
                                 PipelineResourceBindingConfig))
          PipelineResourceBindingConfig();
      ACCESSOR_TYPE_SOA(l_Handle, ComputeStepConfig, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void ComputeStepConfig::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());
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

    void ComputeStepConfig::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer), N(ComputeStepConfig));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, ComputeStepConfig::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(ComputeStepConfig);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &ComputeStepConfig::is_alive;
      l_TypeInfo.destroy = &ComputeStepConfig::destroy;
      l_TypeInfo.serialize = &ComputeStepConfig::serialize;
      l_TypeInfo.deserialize = &ComputeStepConfig::deserialize;
      l_TypeInfo.find_by_index = &ComputeStepConfig::_find_by_index;
      l_TypeInfo.notify = &ComputeStepConfig::_notify;
      l_TypeInfo.find_by_name = &ComputeStepConfig::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &ComputeStepConfig::_make;
      l_TypeInfo.duplicate_default = &ComputeStepConfig::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &ComputeStepConfig::living_instances);
      l_TypeInfo.get_living_count = &ComputeStepConfig::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: callbacks
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(callbacks);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepConfig::Data, callbacks);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          l_Handle.get_callbacks();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStepConfig, callbacks,
              ComputeStepCallbacks);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_callbacks(*(ComputeStepCallbacks *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          *((ComputeStepCallbacks *)p_Data) =
              l_Handle.get_callbacks();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: callbacks
      }
      {
        // Property: resources
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resources);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepConfig::Data, resources);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          l_Handle.get_resources();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStepConfig, resources,
              Util::List<ResourceConfig>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          *((Util::List<ResourceConfig> *)p_Data) =
              l_Handle.get_resources();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resources
      }
      {
        // Property: pipelines
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pipelines);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepConfig::Data, pipelines);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          l_Handle.get_pipelines();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStepConfig, pipelines,
              Util::List<ComputePipelineConfig>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          *((Util::List<ComputePipelineConfig> *)p_Data) =
              l_Handle.get_pipelines();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: pipelines
      }
      {
        // Property: output_image
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(output_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepConfig::Data, output_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          l_Handle.get_output_image();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStepConfig, output_image,
              PipelineResourceBindingConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_output_image(
              *(PipelineResourceBindingConfig *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          *((PipelineResourceBindingConfig *)p_Data) =
              l_Handle.get_output_image();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: output_image
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepConfig::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
              l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStepConfig, name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStepConfig l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<ComputeStepConfig> l_HandleLock(
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
        l_FunctionInfo.handleType = ComputeStepConfig::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Node);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void ComputeStepConfig::cleanup()
    {
      Low::Util::List<ComputeStepConfig> l_Instances =
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
    ComputeStepConfig::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    ComputeStepConfig
    ComputeStepConfig::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      ComputeStepConfig l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = ComputeStepConfig::TYPE_ID;

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

    ComputeStepConfig
    ComputeStepConfig::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      ComputeStepConfig l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = ComputeStepConfig::TYPE_ID;

      return l_Handle;
    }

    bool ComputeStepConfig::is_alive() const
    {
      if (m_Data.m_Type != ComputeStepConfig::TYPE_ID) {
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
      return m_Data.m_Type == ComputeStepConfig::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t ComputeStepConfig::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    ComputeStepConfig::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    ComputeStepConfig
    ComputeStepConfig::find_by_name(Low::Util::Name p_Name)
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

    ComputeStepConfig
    ComputeStepConfig::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      ComputeStepConfig l_Handle = make(p_Name);
      l_Handle.set_callbacks(get_callbacks());
      l_Handle.set_output_image(get_output_image());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    ComputeStepConfig
    ComputeStepConfig::duplicate(ComputeStepConfig p_Handle,
                                 Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    ComputeStepConfig::_duplicate(Low::Util::Handle p_Handle,
                                  Low::Util::Name p_Name)
    {
      ComputeStepConfig l_ComputeStepConfig = p_Handle.get_id();
      return l_ComputeStepConfig.duplicate(p_Name);
    }

    void
    ComputeStepConfig::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void ComputeStepConfig::serialize(Low::Util::Handle p_Handle,
                                      Low::Util::Yaml::Node &p_Node)
    {
      ComputeStepConfig l_ComputeStepConfig = p_Handle.get_id();
      l_ComputeStepConfig.serialize(p_Node);
    }

    Low::Util::Handle
    ComputeStepConfig::deserialize(Low::Util::Yaml::Node &p_Node,
                                   Low::Util::Handle p_Creator)
    {
      ComputeStepConfig l_Handle =
          ComputeStepConfig::make(N(ComputeStepConfig));

      if (p_Node["callbacks"]) {
      }
      if (p_Node["resources"]) {
      }
      if (p_Node["pipelines"]) {
      }
      if (p_Node["output_image"]) {
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void ComputeStepConfig::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 ComputeStepConfig::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 ComputeStepConfig::observe(Low::Util::Name p_Observable,
                                   Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void ComputeStepConfig::notify(Low::Util::Handle p_Observed,
                                   Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void ComputeStepConfig::_notify(Low::Util::Handle p_Observer,
                                    Low::Util::Handle p_Observed,
                                    Low::Util::Name p_Observable)
    {
      ComputeStepConfig l_ComputeStepConfig = p_Observer.get_id();
      l_ComputeStepConfig.notify(p_Observed, p_Observable);
    }

    ComputeStepCallbacks &ComputeStepConfig::get_callbacks() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_callbacks

      // LOW_CODEGEN::END::CUSTOM:GETTER_callbacks

      return TYPE_SOA(ComputeStepConfig, callbacks,
                      ComputeStepCallbacks);
    }
    void
    ComputeStepConfig::set_callbacks(ComputeStepCallbacks &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_callbacks

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_callbacks

      // Set new value
      TYPE_SOA(ComputeStepConfig, callbacks, ComputeStepCallbacks) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_callbacks

      // LOW_CODEGEN::END::CUSTOM:SETTER_callbacks

      broadcast_observable(N(callbacks));
    }

    Util::List<ResourceConfig> &
    ComputeStepConfig::get_resources() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resources

      // LOW_CODEGEN::END::CUSTOM:GETTER_resources

      return TYPE_SOA(ComputeStepConfig, resources,
                      Util::List<ResourceConfig>);
    }

    Util::List<ComputePipelineConfig> &
    ComputeStepConfig::get_pipelines() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pipelines

      // LOW_CODEGEN::END::CUSTOM:GETTER_pipelines

      return TYPE_SOA(ComputeStepConfig, pipelines,
                      Util::List<ComputePipelineConfig>);
    }

    PipelineResourceBindingConfig &
    ComputeStepConfig::get_output_image() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:GETTER_output_image

      return TYPE_SOA(ComputeStepConfig, output_image,
                      PipelineResourceBindingConfig);
    }
    void ComputeStepConfig::set_output_image(
        PipelineResourceBindingConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_output_image

      // Set new value
      TYPE_SOA(ComputeStepConfig, output_image,
               PipelineResourceBindingConfig) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:SETTER_output_image

      broadcast_observable(N(output_image));
    }

    Low::Util::Name ComputeStepConfig::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(ComputeStepConfig, name, Low::Util::Name);
    }
    void ComputeStepConfig::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<ComputeStepConfig> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(ComputeStepConfig, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    ComputeStepConfig
    ComputeStepConfig::make(Util::Name p_Name,
                            Util::Yaml::Node &p_Node)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

      ComputeStepConfig l_Config = ComputeStepConfig::make(p_Name);

      l_Config.get_callbacks().setup_pipelines =
          &ComputeStep::create_pipelines;
      l_Config.get_callbacks().setup_signatures =
          &ComputeStep::create_signatures;
      l_Config.get_callbacks().populate_signatures =
          &ComputeStep::prepare_signatures;
      l_Config.get_callbacks().execute =
          &ComputeStep::default_execute;

      if (p_Node["output_image"]) {
        PipelineResourceBindingConfig l_Binding;
        parse_pipeline_resource_binding(
            l_Binding, LOW_YAML_AS_STRING(p_Node["output_image"]),
            Util::String("image"));
        l_Config.set_output_image(l_Binding);
      }

      if (p_Node["resources"]) {
        parse_resource_configs(p_Node["resources"],
                               l_Config.get_resources());
      }
      parse_compute_pipeline_configs(p_Node["pipelines"],
                                     l_Config.get_pipelines());

      return l_Config;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t ComputeStepConfig::create_instance(
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

    u32 ComputeStepConfig::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT(
          (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
          "Could not increase capacity for ComputeStepConfig.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, ComputeStepConfig::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool ComputeStepConfig::get_page_for_index(const u32 p_Index,
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
