#include "LowRendererGraphicsStepConfig.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowUtilString.h"
#include "LowRendererBackend.h"
#include "LowRendererGraphicsStep.h"

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t GraphicsStepConfig::TYPE_ID = 12;
    uint32_t GraphicsStepConfig::ms_Capacity = 0u;
    uint8_t *GraphicsStepConfig::ms_Buffer = 0;
    Low::Util::Instances::Slot *GraphicsStepConfig::ms_Slots = 0;
    Low::Util::List<GraphicsStepConfig> GraphicsStepConfig::ms_LivingInstances =
        Low::Util::List<GraphicsStepConfig>();

    GraphicsStepConfig::GraphicsStepConfig() : Low::Util::Handle(0ull)
    {
    }
    GraphicsStepConfig::GraphicsStepConfig(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    GraphicsStepConfig::GraphicsStepConfig(GraphicsStepConfig &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle GraphicsStepConfig::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GraphicsStepConfig GraphicsStepConfig::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      GraphicsStepConfig l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = GraphicsStepConfig::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, callbacks,
                              GraphicsStepCallbacks)) GraphicsStepCallbacks();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, resources,
                              Util::List<ResourceConfig>))
          Util::List<ResourceConfig>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, dimensions_config,
                              DimensionsConfig)) DimensionsConfig();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, pipelines,
                              Util::List<GraphicsPipelineConfig>))
          Util::List<GraphicsPipelineConfig>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, rendertargets,
                              Util::List<PipelineResourceBindingConfig>))
          Util::List<PipelineResourceBindingConfig>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig,
                              rendertargets_clearcolor, Math::Color))
          Math::Color();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, depth_rendertarget,
                              PipelineResourceBindingConfig))
          PipelineResourceBindingConfig();
      ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, use_depth, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, depth_clear, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, depth_test, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, depth_write, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, GraphicsStepConfig, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GraphicsStepConfig::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const GraphicsStepConfig *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void GraphicsStepConfig::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(GraphicsStepConfig));

      initialize_buffer(&ms_Buffer, GraphicsStepConfigData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_GraphicsStepConfig);
      LOW_PROFILE_ALLOC(type_slots_GraphicsStepConfig);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GraphicsStepConfig);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GraphicsStepConfig::is_alive;
      l_TypeInfo.destroy = &GraphicsStepConfig::destroy;
      l_TypeInfo.serialize = &GraphicsStepConfig::serialize;
      l_TypeInfo.deserialize = &GraphicsStepConfig::deserialize;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GraphicsStepConfig::_make;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GraphicsStepConfig::living_instances);
      l_TypeInfo.get_living_count = &GraphicsStepConfig::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(callbacks);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepConfigData, callbacks);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_callbacks();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            callbacks, GraphicsStepCallbacks);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_callbacks(*(GraphicsStepCallbacks *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resources);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepConfigData, resources);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_resources();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            resources,
                                            Util::List<ResourceConfig>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions_config);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepConfigData, dimensions_config);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_dimensions_config();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            dimensions_config,
                                            DimensionsConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pipelines);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepConfigData, pipelines);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_pipelines();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            pipelines,
                                            Util::List<GraphicsPipelineConfig>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(rendertargets);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepConfigData, rendertargets);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_rendertargets();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStepConfig, rendertargets,
              Util::List<PipelineResourceBindingConfig>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(rendertargets_clearcolor);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepConfigData, rendertargets_clearcolor);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_rendertargets_clearcolor();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            rendertargets_clearcolor,
                                            Math::Color);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_rendertargets_clearcolor(*(Math::Color *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_rendertarget);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepConfigData, depth_rendertarget);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_depth_rendertarget();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            depth_rendertarget,
                                            PipelineResourceBindingConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_depth_rendertarget(
              *(PipelineResourceBindingConfig *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(use_depth);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepConfigData, use_depth);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.is_use_depth();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            use_depth, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_use_depth(*(bool *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_clear);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepConfigData, depth_clear);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.is_depth_clear();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            depth_clear, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_depth_clear(*(bool *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_test);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepConfigData, depth_test);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.is_depth_test();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            depth_test, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_depth_test(*(bool *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_write);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepConfigData, depth_write);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.is_depth_write();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            depth_write, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_depth_write(*(bool *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_compare_operation);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepConfigData, depth_compare_operation);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_depth_compare_operation();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig,
                                            depth_compare_operation, uint8_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_depth_compare_operation(*(uint8_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepConfigData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStepConfig, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStepConfig l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void GraphicsStepConfig::cleanup()
    {
      Low::Util::List<GraphicsStepConfig> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_GraphicsStepConfig);
      LOW_PROFILE_FREE(type_slots_GraphicsStepConfig);
    }

    GraphicsStepConfig GraphicsStepConfig::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GraphicsStepConfig l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = GraphicsStepConfig::TYPE_ID;

      return l_Handle;
    }

    bool GraphicsStepConfig::is_alive() const
    {
      return m_Data.m_Type == GraphicsStepConfig::TYPE_ID &&
             check_alive(ms_Slots, GraphicsStepConfig::get_capacity());
    }

    uint32_t GraphicsStepConfig::get_capacity()
    {
      return ms_Capacity;
    }

    GraphicsStepConfig GraphicsStepConfig::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    void GraphicsStepConfig::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      Low::Util::Serialization::serialize(p_Node["rendertargets_clearcolor"],
                                          get_rendertargets_clearcolor());
      p_Node["use_depth"] = is_use_depth();
      p_Node["depth_clear"] = is_depth_clear();
      p_Node["depth_test"] = is_depth_test();
      p_Node["depth_write"] = is_depth_write();
      p_Node["depth_compare_operation"] = get_depth_compare_operation();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void GraphicsStepConfig::serialize(Low::Util::Handle p_Handle,
                                       Low::Util::Yaml::Node &p_Node)
    {
      GraphicsStepConfig l_GraphicsStepConfig = p_Handle.get_id();
      l_GraphicsStepConfig.serialize(p_Node);
    }

    Low::Util::Handle
    GraphicsStepConfig::deserialize(Low::Util::Yaml::Node &p_Node,
                                    Low::Util::Handle p_Creator)
    {
      GraphicsStepConfig l_Handle =
          GraphicsStepConfig::make(N(GraphicsStepConfig));

      if (p_Node["callbacks"]) {
      }
      if (p_Node["resources"]) {
      }
      if (p_Node["dimensions_config"]) {
      }
      if (p_Node["pipelines"]) {
      }
      if (p_Node["rendertargets"]) {
      }
      if (p_Node["rendertargets_clearcolor"]) {
        l_Handle.set_rendertargets_clearcolor(
            Low::Util::Serialization::deserialize_vector4(
                p_Node["rendertargets_clearcolor"]));
      }
      if (p_Node["depth_rendertarget"]) {
      }
      if (p_Node["use_depth"]) {
        l_Handle.set_use_depth(p_Node["use_depth"].as<bool>());
      }
      if (p_Node["depth_clear"]) {
        l_Handle.set_depth_clear(p_Node["depth_clear"].as<bool>());
      }
      if (p_Node["depth_test"]) {
        l_Handle.set_depth_test(p_Node["depth_test"].as<bool>());
      }
      if (p_Node["depth_write"]) {
        l_Handle.set_depth_write(p_Node["depth_write"].as<bool>());
      }
      if (p_Node["depth_compare_operation"]) {
        l_Handle.set_depth_compare_operation(
            p_Node["depth_compare_operation"].as<uint8_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    GraphicsStepCallbacks &GraphicsStepConfig::get_callbacks() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_callbacks
      // LOW_CODEGEN::END::CUSTOM:GETTER_callbacks

      return TYPE_SOA(GraphicsStepConfig, callbacks, GraphicsStepCallbacks);
    }
    void GraphicsStepConfig::set_callbacks(GraphicsStepCallbacks &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_callbacks
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_callbacks

      // Set new value
      TYPE_SOA(GraphicsStepConfig, callbacks, GraphicsStepCallbacks) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_callbacks
      // LOW_CODEGEN::END::CUSTOM:SETTER_callbacks
    }

    Util::List<ResourceConfig> &GraphicsStepConfig::get_resources() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resources
      // LOW_CODEGEN::END::CUSTOM:GETTER_resources

      return TYPE_SOA(GraphicsStepConfig, resources,
                      Util::List<ResourceConfig>);
    }

    DimensionsConfig &GraphicsStepConfig::get_dimensions_config() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions_config
      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions_config

      return TYPE_SOA(GraphicsStepConfig, dimensions_config, DimensionsConfig);
    }
    void GraphicsStepConfig::set_dimensions_config(DimensionsConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions_config
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions_config

      // Set new value
      TYPE_SOA(GraphicsStepConfig, dimensions_config, DimensionsConfig) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions_config
      // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions_config
    }

    Util::List<GraphicsPipelineConfig> &
    GraphicsStepConfig::get_pipelines() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pipelines
      // LOW_CODEGEN::END::CUSTOM:GETTER_pipelines

      return TYPE_SOA(GraphicsStepConfig, pipelines,
                      Util::List<GraphicsPipelineConfig>);
    }

    Util::List<PipelineResourceBindingConfig> &
    GraphicsStepConfig::get_rendertargets() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rendertargets
      // LOW_CODEGEN::END::CUSTOM:GETTER_rendertargets

      return TYPE_SOA(GraphicsStepConfig, rendertargets,
                      Util::List<PipelineResourceBindingConfig>);
    }

    Math::Color &GraphicsStepConfig::get_rendertargets_clearcolor() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rendertargets_clearcolor
      // LOW_CODEGEN::END::CUSTOM:GETTER_rendertargets_clearcolor

      return TYPE_SOA(GraphicsStepConfig, rendertargets_clearcolor,
                      Math::Color);
    }
    void GraphicsStepConfig::set_rendertargets_clearcolor(Math::Color &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rendertargets_clearcolor
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_rendertargets_clearcolor

      // Set new value
      TYPE_SOA(GraphicsStepConfig, rendertargets_clearcolor, Math::Color) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rendertargets_clearcolor
      // LOW_CODEGEN::END::CUSTOM:SETTER_rendertargets_clearcolor
    }

    PipelineResourceBindingConfig &
    GraphicsStepConfig::get_depth_rendertarget() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_rendertarget
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_rendertarget

      return TYPE_SOA(GraphicsStepConfig, depth_rendertarget,
                      PipelineResourceBindingConfig);
    }
    void GraphicsStepConfig::set_depth_rendertarget(
        PipelineResourceBindingConfig &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_rendertarget
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_rendertarget

      // Set new value
      TYPE_SOA(GraphicsStepConfig, depth_rendertarget,
               PipelineResourceBindingConfig) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_rendertarget
      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_rendertarget
    }

    bool GraphicsStepConfig::is_use_depth() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_use_depth
      // LOW_CODEGEN::END::CUSTOM:GETTER_use_depth

      return TYPE_SOA(GraphicsStepConfig, use_depth, bool);
    }
    void GraphicsStepConfig::set_use_depth(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_use_depth
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_use_depth

      // Set new value
      TYPE_SOA(GraphicsStepConfig, use_depth, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_use_depth
      // LOW_CODEGEN::END::CUSTOM:SETTER_use_depth
    }

    bool GraphicsStepConfig::is_depth_clear() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_clear
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_clear

      return TYPE_SOA(GraphicsStepConfig, depth_clear, bool);
    }
    void GraphicsStepConfig::set_depth_clear(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_clear
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_clear

      // Set new value
      TYPE_SOA(GraphicsStepConfig, depth_clear, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_clear
      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_clear
    }

    bool GraphicsStepConfig::is_depth_test() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_test
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_test

      return TYPE_SOA(GraphicsStepConfig, depth_test, bool);
    }
    void GraphicsStepConfig::set_depth_test(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_test
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_test

      // Set new value
      TYPE_SOA(GraphicsStepConfig, depth_test, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_test
      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_test
    }

    bool GraphicsStepConfig::is_depth_write() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_write
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_write

      return TYPE_SOA(GraphicsStepConfig, depth_write, bool);
    }
    void GraphicsStepConfig::set_depth_write(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_write
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_write

      // Set new value
      TYPE_SOA(GraphicsStepConfig, depth_write, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_write
      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_write
    }

    uint8_t GraphicsStepConfig::get_depth_compare_operation() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_compare_operation
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_compare_operation

      return TYPE_SOA(GraphicsStepConfig, depth_compare_operation, uint8_t);
    }
    void GraphicsStepConfig::set_depth_compare_operation(uint8_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_compare_operation
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_compare_operation

      // Set new value
      TYPE_SOA(GraphicsStepConfig, depth_compare_operation, uint8_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_compare_operation
      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_compare_operation
    }

    Low::Util::Name GraphicsStepConfig::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GraphicsStepConfig, name, Low::Util::Name);
    }
    void GraphicsStepConfig::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GraphicsStepConfig, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    GraphicsStepConfig GraphicsStepConfig::make(Util::Name p_Name,
                                                Util::Yaml::Node &p_Node)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      GraphicsStepConfig l_Config = GraphicsStepConfig::make(p_Name);

      l_Config.get_callbacks().setup_signature =
          &GraphicsStep::create_signature;
      l_Config.get_callbacks().setup_pipelines =
          &GraphicsStep::create_pipelines;
      l_Config.get_callbacks().setup_renderpass =
          &GraphicsStep::create_renderpass;
      l_Config.get_callbacks().execute = &GraphicsStep::default_execute;

      DimensionsConfig l_DimensionsConfig;
      l_DimensionsConfig.type = ImageResourceDimensionType::RELATIVE;
      l_DimensionsConfig.relative.multiplier = 1.0f;
      l_DimensionsConfig.relative.target =
          ImageResourceDimensionRelativeOptions::RENDERFLOW;

      if (p_Node["dimensions"]) {
        parse_dimensions_config(p_Node["dimensions"], l_DimensionsConfig);
      }
      l_Config.set_dimensions_config(l_DimensionsConfig);

      if (p_Node["resources"]) {
        parse_resource_configs(p_Node["resources"], l_Config.get_resources());
      }

      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.name = N(_renderobject_buffer);
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.buffer.size =
            sizeof(RenderObjectShaderInfo) * LOW_RENDERER_RENDEROBJECT_COUNT;
        l_Config.get_resources().push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.name = N(_color_buffer);
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.buffer.size = sizeof(Math::Vector4) * 32u;
        l_Config.get_resources().push_back(l_ResourceConfig);
      }

      Util::Yaml::Node &l_PipelinesNode = p_Node["pipelines"];
      for (auto it = l_PipelinesNode.begin(); it != l_PipelinesNode.end();
           ++it) {
        Util::Name i_PipelineName = LOW_YAML_AS_NAME((*it)["name"]);
        l_Config.get_pipelines().push_back(
            get_graphics_pipeline_config(i_PipelineName));
      }

      l_Config.set_rendertargets_clearcolor(
          Math::Color(0.0f, 0.0f, 0.0f, 1.0f));

      if (p_Node["clear_color"]) {
        Math::Color l_ClearColor;
        l_ClearColor.r = p_Node["clear_color"]["r"].as<float>();
        l_ClearColor.g = p_Node["clear_color"]["g"].as<float>();
        l_ClearColor.b = p_Node["clear_color"]["b"].as<float>();
        l_ClearColor.a = p_Node["clear_color"]["a"].as<float>();

        l_Config.set_rendertargets_clearcolor(l_ClearColor);
      }

      Util::String l_ContextPrefix = "context:";
      Util::String l_RenderFlowPrefix = "renderflow:";

      Util::Yaml::Node &l_RenderTargetsNode = p_Node["rendertargets"];
      for (auto it = l_RenderTargetsNode.begin();
           it != l_RenderTargetsNode.end(); ++it) {
        Util::String i_TargetString = LOW_YAML_AS_STRING((*it));
        PipelineResourceBindingConfig i_ResourceBinding;
        parse_pipeline_resource_binding(i_ResourceBinding, i_TargetString,
                                        Util::String("sampler"));

        l_Config.get_rendertargets().push_back(i_ResourceBinding);
      }

      l_Config.set_use_depth(false);
      l_Config.set_depth_write(false);
      l_Config.set_depth_test(false);
      l_Config.set_depth_clear(false);
      l_Config.set_depth_compare_operation(Backend::CompareOperation::EQUAL);

      if (p_Node["depth_rendertarget"]) {
        Util::String l_TargetString =
            LOW_YAML_AS_STRING(p_Node["depth_rendertarget"]);
        PipelineResourceBindingConfig l_BindConfig;

        parse_pipeline_resource_binding(l_BindConfig, l_TargetString,
                                        Util::String("sampler"));

        l_Config.set_depth_rendertarget(l_BindConfig);
        l_Config.set_use_depth(true);

        l_Config.set_depth_write(p_Node["depth_write"].as<bool>());
        l_Config.set_depth_test(p_Node["depth_test"].as<bool>());
        l_Config.set_depth_clear(p_Node["depth_clear"].as<bool>());

        Util::String l_CompareOperationString =
            LOW_YAML_AS_STRING(p_Node["depth_compare_operation"]);
        if (l_CompareOperationString == "LESS") {
          l_Config.set_depth_compare_operation(Backend::CompareOperation::LESS);
        } else if (l_CompareOperationString == "EQUAL" ||
                   l_CompareOperationString == "EQUALS") {
          l_Config.set_depth_compare_operation(
              Backend::CompareOperation::EQUAL);
        } else {
          LOW_ASSERT(false, "Unknown compare operation in configuration");
        }
      }

      return l_Config;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t GraphicsStepConfig::create_instance()
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

    void GraphicsStepConfig::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(GraphicsStepConfigData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepConfigData, callbacks) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepConfigData, callbacks) *
                          (l_Capacity)],
               l_Capacity * sizeof(GraphicsStepCallbacks));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr =
              new (&l_NewBuffer[offsetof(GraphicsStepConfigData, resources) *
                                    (l_Capacity + l_CapacityIncrease) +
                                (it->get_index() *
                                 sizeof(Util::List<ResourceConfig>))])
                  Util::List<ResourceConfig>();
          *i_ValPtr = it->get_resources();
        }
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(GraphicsStepConfigData, dimensions_config) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GraphicsStepConfigData, dimensions_config) *
                       (l_Capacity)],
            l_Capacity * sizeof(DimensionsConfig));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr =
              new (&l_NewBuffer[offsetof(GraphicsStepConfigData, pipelines) *
                                    (l_Capacity + l_CapacityIncrease) +
                                (it->get_index() *
                                 sizeof(Util::List<GraphicsPipelineConfig>))])
                  Util::List<GraphicsPipelineConfig>();
          *i_ValPtr = it->get_pipelines();
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(GraphicsStepConfigData, rendertargets) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::List<PipelineResourceBindingConfig>))])
              Util::List<PipelineResourceBindingConfig>();
          *i_ValPtr = it->get_rendertargets();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepConfigData,
                                     rendertargets_clearcolor) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepConfigData,
                                   rendertargets_clearcolor) *
                          (l_Capacity)],
               l_Capacity * sizeof(Math::Color));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(GraphicsStepConfigData, depth_rendertarget) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GraphicsStepConfigData, depth_rendertarget) *
                       (l_Capacity)],
            l_Capacity * sizeof(PipelineResourceBindingConfig));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepConfigData, use_depth) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepConfigData, use_depth) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepConfigData, depth_clear) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepConfigData, depth_clear) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepConfigData, depth_test) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepConfigData, depth_test) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepConfigData, depth_write) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepConfigData, depth_write) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepConfigData,
                                     depth_compare_operation) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepConfigData,
                                   depth_compare_operation) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint8_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(GraphicsStepConfigData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(GraphicsStepConfigData, name) * (l_Capacity)],
            l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for GraphicsStepConfig from "
                    << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Renderer
} // namespace Low
