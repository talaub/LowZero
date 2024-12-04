#include "LowRendererComputeStep.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t ComputeStep::TYPE_ID = 10;
    uint32_t ComputeStep::ms_Capacity = 0u;
    uint8_t *ComputeStep::ms_Buffer = 0;
    Low::Util::Instances::Slot *ComputeStep::ms_Slots = 0;
    Low::Util::List<ComputeStep> ComputeStep::ms_LivingInstances =
        Low::Util::List<ComputeStep>();

    ComputeStep::ComputeStep() : Low::Util::Handle(0ull)
    {
    }
    ComputeStep::ComputeStep(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    ComputeStep::ComputeStep(ComputeStep &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle ComputeStep::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    ComputeStep ComputeStep::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      ComputeStep l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = ComputeStep::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(
          l_Handle, ComputeStep, resources,
          SINGLE_ARG(Util::Map<RenderFlow, ResourceRegistry>)))
          Util::Map<RenderFlow, ResourceRegistry>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, ComputeStep, config,
                              ComputeStepConfig)) ComputeStepConfig();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, ComputeStep, pipelines,
          SINGLE_ARG(
              Util::Map<RenderFlow,
                        Util::List<Interface::ComputePipeline>>)))
          Util::Map<RenderFlow,
                    Util::List<Interface::ComputePipeline>>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, ComputeStep, signatures,
          SINGLE_ARG(
              Util::Map<
                  RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>>)))
          Util::Map<
              RenderFlow,
              Util::List<Interface::PipelineResourceSignature>>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, ComputeStep, context,
                              Interface::Context))
          Interface::Context();
      new (&ACCESSOR_TYPE_SOA(l_Handle, ComputeStep, output_image,
                              Resource::Image)) Resource::Image();
      ACCESSOR_TYPE_SOA(l_Handle, ComputeStep, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void ComputeStep::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const ComputeStep *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void ComputeStep::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(ComputeStep));

      initialize_buffer(&ms_Buffer, ComputeStepData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_ComputeStep);
      LOW_PROFILE_ALLOC(type_slots_ComputeStep);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(ComputeStep);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &ComputeStep::is_alive;
      l_TypeInfo.destroy = &ComputeStep::destroy;
      l_TypeInfo.serialize = &ComputeStep::serialize;
      l_TypeInfo.deserialize = &ComputeStep::deserialize;
      l_TypeInfo.find_by_index = &ComputeStep::_find_by_index;
      l_TypeInfo.find_by_name = &ComputeStep::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &ComputeStep::_make;
      l_TypeInfo.duplicate_default = &ComputeStep::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &ComputeStep::living_instances);
      l_TypeInfo.get_living_count = &ComputeStep::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: resources
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resources);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepData, resources);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.get_resources();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStep, resources,
              SINGLE_ARG(Util::Map<RenderFlow, ResourceRegistry>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStep l_Handle = p_Handle.get_id();
          *((Util::Map<RenderFlow, ResourceRegistry> *)p_Data) =
              l_Handle.get_resources();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resources
      }
      {
        // Property: config
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(config);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(ComputeStepData, config);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = ComputeStepConfig::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.get_config();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStep, config, ComputeStepConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStep l_Handle = p_Handle.get_id();
          *((ComputeStepConfig *)p_Data) = l_Handle.get_config();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: config
      }
      {
        // Property: pipelines
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pipelines);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepData, pipelines);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.get_pipelines();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStep, pipelines,
              SINGLE_ARG(
                  Util::Map<RenderFlow,
                            Util::List<Interface::ComputePipeline>>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStep l_Handle = p_Handle.get_id();
          *((Util::Map<RenderFlow,
                       Util::List<Interface::ComputePipeline>> *)
                p_Data) = l_Handle.get_pipelines();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: pipelines
      }
      {
        // Property: signatures
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(signatures);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepData, signatures);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.get_signatures();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStep, signatures,
              SINGLE_ARG(Util::Map<
                         RenderFlow,
                         Util::List<
                             Interface::PipelineResourceSignature>>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStep l_Handle = p_Handle.get_id();
          *((Util::Map<
              RenderFlow,
              Util::List<Interface::PipelineResourceSignature>> *)
                p_Data) = l_Handle.get_signatures();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: signatures
      }
      {
        // Property: context
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepData, context);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Interface::Context::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.get_context();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStep, context, Interface::Context);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStep l_Handle = p_Handle.get_id();
          *((Interface::Context *)p_Data) = l_Handle.get_context();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: context
      }
      {
        // Property: output_image
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(output_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ComputeStepData, output_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Resource::Image::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.get_output_image();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ComputeStep, output_image, Resource::Image);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.set_output_image(*(Resource::Image *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStep l_Handle = p_Handle.get_id();
          *((Resource::Image *)p_Data) = l_Handle.get_output_image();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: output_image
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(ComputeStepData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ComputeStep,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ComputeStep l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ComputeStep l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = ComputeStep::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Context);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Interface::Context::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Config);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = ComputeStepConfig::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: prepare
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(prepare);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderFlow);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderFlow::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: prepare
      }
      {
        // Function: execute
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(execute);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderFlow);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderFlow::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: execute
      }
      {
        // Function: update_dimensions
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(update_dimensions);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderFlow);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderFlow::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: update_dimensions
      }
      {
        // Function: create_pipelines
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(create_pipelines);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = ComputeStep::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderFlow);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderFlow::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: create_pipelines
      }
      {
        // Function: create_signatures
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(create_signatures);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = ComputeStep::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderFlow);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderFlow::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: create_signatures
      }
      {
        // Function: prepare_signatures
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(prepare_signatures);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = ComputeStep::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderFlow);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderFlow::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: prepare_signatures
      }
      {
        // Function: default_execute
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(default_execute);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = ComputeStep::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderFlow);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderFlow::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: default_execute
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void ComputeStep::cleanup()
    {
      Low::Util::List<ComputeStep> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_ComputeStep);
      LOW_PROFILE_FREE(type_slots_ComputeStep);
    }

    Low::Util::Handle ComputeStep::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    ComputeStep ComputeStep::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      ComputeStep l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = ComputeStep::TYPE_ID;

      return l_Handle;
    }

    bool ComputeStep::is_alive() const
    {
      return m_Data.m_Type == ComputeStep::TYPE_ID &&
             check_alive(ms_Slots, ComputeStep::get_capacity());
    }

    uint32_t ComputeStep::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    ComputeStep::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    ComputeStep ComputeStep::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    ComputeStep ComputeStep::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      ComputeStep l_Handle = make(p_Name);
      if (get_config().is_alive()) {
        l_Handle.set_config(get_config());
      }
      if (get_context().is_alive()) {
        l_Handle.set_context(get_context());
      }
      if (get_output_image().is_alive()) {
        l_Handle.set_output_image(get_output_image());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    ComputeStep ComputeStep::duplicate(ComputeStep p_Handle,
                                       Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    ComputeStep::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Name p_Name)
    {
      ComputeStep l_ComputeStep = p_Handle.get_id();
      return l_ComputeStep.duplicate(p_Name);
    }

    void ComputeStep::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      if (get_config().is_alive()) {
        get_config().serialize(p_Node["config"]);
      }
      if (get_context().is_alive()) {
        get_context().serialize(p_Node["context"]);
      }
      if (get_output_image().is_alive()) {
        get_output_image().serialize(p_Node["output_image"]);
      }
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void ComputeStep::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
    {
      ComputeStep l_ComputeStep = p_Handle.get_id();
      l_ComputeStep.serialize(p_Node);
    }

    Low::Util::Handle
    ComputeStep::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
    {
      ComputeStep l_Handle = ComputeStep::make(N(ComputeStep));

      if (p_Node["resources"]) {
      }
      if (p_Node["config"]) {
        l_Handle.set_config(ComputeStepConfig::deserialize(
                                p_Node["config"], l_Handle.get_id())
                                .get_id());
      }
      if (p_Node["pipelines"]) {
      }
      if (p_Node["signatures"]) {
      }
      if (p_Node["context"]) {
        l_Handle.set_context(Interface::Context::deserialize(
                                 p_Node["context"], l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["output_image"]) {
        l_Handle.set_output_image(
            Resource::Image::deserialize(p_Node["output_image"],
                                         l_Handle.get_id())
                .get_id());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    Util::Map<RenderFlow, ResourceRegistry> &
    ComputeStep::get_resources() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resources

      // LOW_CODEGEN::END::CUSTOM:GETTER_resources

      return TYPE_SOA(
          ComputeStep, resources,
          SINGLE_ARG(Util::Map<RenderFlow, ResourceRegistry>));
    }

    ComputeStepConfig ComputeStep::get_config() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_config

      // LOW_CODEGEN::END::CUSTOM:GETTER_config

      return TYPE_SOA(ComputeStep, config, ComputeStepConfig);
    }
    void ComputeStep::set_config(ComputeStepConfig p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_config

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_config

      // Set new value
      TYPE_SOA(ComputeStep, config, ComputeStepConfig) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_config

      // LOW_CODEGEN::END::CUSTOM:SETTER_config
    }

    Util::Map<RenderFlow, Util::List<Interface::ComputePipeline>> &
    ComputeStep::get_pipelines() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pipelines

      // LOW_CODEGEN::END::CUSTOM:GETTER_pipelines

      return TYPE_SOA(
          ComputeStep, pipelines,
          SINGLE_ARG(
              Util::Map<RenderFlow,
                        Util::List<Interface::ComputePipeline>>));
    }

    Util::Map<RenderFlow,
              Util::List<Interface::PipelineResourceSignature>> &
    ComputeStep::get_signatures() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_signatures

      // LOW_CODEGEN::END::CUSTOM:GETTER_signatures

      return TYPE_SOA(
          ComputeStep, signatures,
          SINGLE_ARG(
              Util::Map<
                  RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>>));
    }

    Interface::Context ComputeStep::get_context() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context

      // LOW_CODEGEN::END::CUSTOM:GETTER_context

      return TYPE_SOA(ComputeStep, context, Interface::Context);
    }
    void ComputeStep::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_context

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_context

      // Set new value
      TYPE_SOA(ComputeStep, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context

      // LOW_CODEGEN::END::CUSTOM:SETTER_context
    }

    Resource::Image ComputeStep::get_output_image() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:GETTER_output_image

      return TYPE_SOA(ComputeStep, output_image, Resource::Image);
    }
    void ComputeStep::set_output_image(Resource::Image p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_output_image

      // Set new value
      TYPE_SOA(ComputeStep, output_image, Resource::Image) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:SETTER_output_image
    }

    Low::Util::Name ComputeStep::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(ComputeStep, name, Low::Util::Name);
    }
    void ComputeStep::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(ComputeStep, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    ComputeStep ComputeStep::make(Util::Name p_Name,
                                  Interface::Context p_Context,
                                  ComputeStepConfig p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

      ComputeStep l_Step = ComputeStep::make(p_Name);

      l_Step.set_config(p_Config);
      l_Step.set_context(p_Context);

      return l_Step;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void ComputeStep::prepare(RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_prepare

      Util::Map<RenderFlow, ResourceRegistry> &l_Resources =
          get_resources();
      l_Resources[p_RenderFlow].initialize(
          get_config().get_resources(), get_context(), p_RenderFlow);

      // Sets the output image depending on what is configured
      // in the config
      if (step_has_resource_from_binding(
              get_config().get_output_image(), p_RenderFlow, *this)) {
        set_output_image(
            (Resource::Image)step_get_resource_from_binding(
                get_config().get_output_image(), p_RenderFlow, *this)
                .get_id());
      }

      get_config().get_callbacks().setup_signatures(*this,
                                                    p_RenderFlow);
      get_config().get_callbacks().setup_pipelines(*this,
                                                   p_RenderFlow);

      get_config().get_callbacks().populate_signatures(*this,
                                                       p_RenderFlow);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare
    }

    void ComputeStep::execute(RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute

      LOW_ASSERT(get_resources().find(p_RenderFlow) !=
                     get_resources().end(),
                 "Step not prepared for renderflow");

      Util::String l_ProfileString = get_name().c_str();
      l_ProfileString += " (ComputeStep execute)";
      LOW_PROFILE_CPU("Renderer", l_ProfileString.c_str());

      if (get_context().is_debug_enabled()) {
        Util::String l_RenderDocLabel =
            Util::String("ComputeStep - ") + get_name().c_str();
        LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
            get_context().get_context(), l_RenderDocLabel,
            Math::Color(0.4249f, 0.2341f, 0.341f, 1.0f));
      }

      get_config().get_callbacks().execute(*this, p_RenderFlow);

      if (get_context().is_debug_enabled()) {
        LOW_RENDERER_END_RENDERDOC_SECTION(
            get_context().get_context());
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

    void ComputeStep::update_dimensions(RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_dimensions

      get_resources()[p_RenderFlow].update_dimensions(p_RenderFlow);

      get_config().get_callbacks().populate_signatures(*this,
                                                       p_RenderFlow);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_dimensions
    }

    void ComputeStep::create_pipelines(ComputeStep p_Step,
                                       RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_create_pipelines

      for (uint32_t i = 0;
           i < p_Step.get_config().get_pipelines().size(); ++i) {
        ComputePipelineConfig &i_Config =
            p_Step.get_config().get_pipelines()[i];

        Interface::PipelineComputeCreateParams l_Params;
        l_Params.context = p_Step.get_context();
        l_Params.shaderPath = i_Config.shader;
        l_Params.signatures = {
            p_Step.get_context().get_global_signature(),
            p_RenderFlow.get_resource_signature(),
            p_Step.get_signatures()[p_RenderFlow][i]};
        p_Step.get_pipelines()[p_RenderFlow].push_back(
            Interface::ComputePipeline::make(p_Step.get_name(),
                                             l_Params));
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_create_pipelines
    }

    void ComputeStep::create_signatures(ComputeStep p_Step,
                                        RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_create_signatures

      for (uint32_t i = 0;
           i < p_Step.get_config().get_pipelines().size(); ++i) {
        ComputePipelineConfig &i_Config =
            p_Step.get_config().get_pipelines()[i];

        Util::List<Backend::PipelineResourceDescription>
            i_ResourceDescriptions;
        for (auto it = i_Config.resourceBinding.begin();
             it != i_Config.resourceBinding.end(); ++it) {
          Backend::PipelineResourceDescription i_Resource;
          i_Resource.name = it->resourceName;
          i_Resource.step = Backend::ResourcePipelineStep::COMPUTE;

          if (it->bindType == ResourceBindType::IMAGE) {
            i_Resource.type = Backend::ResourceType::IMAGE;
          } else if (it->bindType == ResourceBindType::SAMPLER) {
            i_Resource.type = Backend::ResourceType::SAMPLER;
          } else if (it->bindType == ResourceBindType::BUFFER) {
            i_Resource.type = Backend::ResourceType::BUFFER;
          } else if (it->bindType ==
                     ResourceBindType::UNBOUND_SAMPLER) {
            i_Resource.type = Backend::ResourceType::UNBOUND_SAMPLER;
          } else if (it->bindType == ResourceBindType::TEXTURE2D) {
            i_Resource.type = Backend::ResourceType::TEXTURE2D;
          } else {
            LOW_ASSERT(false, "Unknown resource bind type");
          }

          if (it->resourceScope == ResourceBindScope::LOCAL) {
            bool i_Found = false;

            if (it->resourceName == N(INPUT_IMAGE)) {
              i_Found = true;
              i_Resource.arraySize = 1;
            }

            for (auto rit =
                     p_Step.get_config().get_resources().begin();
                 rit != p_Step.get_config().get_resources().end() &&
                 !i_Found;
                 ++rit) {
              if (rit->name == it->resourceName) {
                i_Found = true;
                i_Resource.arraySize = rit->arraySize;
                break;
              }
            }
            LOW_ASSERT(
                i_Found,
                "Cannot bind resource not found in renderstep");
          } else if (it->resourceScope ==
                     ResourceBindScope::RENDERFLOW) {
            i_Resource.arraySize = 1;
          } else {
            LOW_ASSERT(false, "Resource bind scope not supported");
          }

          i_ResourceDescriptions.push_back(i_Resource);
        }

        p_Step.get_signatures()[p_RenderFlow].push_back(
            Interface::PipelineResourceSignature::make(
                p_Step.get_name(), p_Step.get_context(), 2,
                i_ResourceDescriptions));
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_create_signatures
    }

    void ComputeStep::prepare_signatures(ComputeStep p_Step,
                                         RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_prepare_signatures

      Util::List<ComputePipelineConfig> &l_Configs =
          p_Step.get_config().get_pipelines();
      for (uint32_t i = 0; i < l_Configs.size(); ++i) {
        ComputePipelineConfig &i_Config = l_Configs[i];

        Interface::PipelineResourceSignature i_Signature =
            p_Step.get_signatures()[p_RenderFlow][i];
        for (auto it = i_Config.resourceBinding.begin();
             it != i_Config.resourceBinding.end(); ++it) {
          if (it->bindType == ResourceBindType::IMAGE) {
            Resource::Image i_Image;

            if (it->resourceScope == ResourceBindScope::LOCAL) {
              if (it->resourceName == N(INPUT_IMAGE)) {
                i_Image =
                    p_RenderFlow.get_previous_output_image(p_Step);
              } else {
                i_Image = p_Step.get_resources()[p_RenderFlow]
                              .get_image_resource(it->resourceName);
              }
            } else if (it->resourceScope ==
                       ResourceBindScope::RENDERFLOW) {
              i_Image =
                  p_RenderFlow.get_resources().get_image_resource(
                      it->resourceName);
            } else {
              LOW_ASSERT(false, "Resource bind scope not supported");
            }
            i_Signature.set_image_resource(it->resourceName, 0,
                                           i_Image);
          } else if (it->bindType == ResourceBindType::SAMPLER) {
            Resource::Image i_Image;

            if (it->resourceScope == ResourceBindScope::LOCAL) {
              if (it->resourceName == N(INPUT_IMAGE)) {
                i_Image =
                    p_RenderFlow.get_previous_output_image(p_Step);
              } else {
                i_Image = p_Step.get_resources()[p_RenderFlow]
                              .get_image_resource(it->resourceName);
              }
            } else if (it->resourceScope ==
                       ResourceBindScope::RENDERFLOW) {
              i_Image =
                  p_RenderFlow.get_resources().get_image_resource(
                      it->resourceName);
            } else {
              LOW_ASSERT(false, "Resource bind scope not supported");
            }
            i_Signature.set_sampler_resource(it->resourceName, 0,
                                             i_Image);
          } else if (it->bindType == ResourceBindType::TEXTURE2D) {
            Resource::Image i_Image;

            if (it->resourceScope == ResourceBindScope::LOCAL) {
              i_Image = p_Step.get_resources()[p_RenderFlow]
                            .get_image_resource(it->resourceName);
            } else if (it->resourceScope ==
                       ResourceBindScope::RENDERFLOW) {
              i_Image =
                  p_RenderFlow.get_resources().get_image_resource(
                      it->resourceName);
            } else {
              LOW_ASSERT(false, "Resource bind scope not supported");
            }
            i_Signature.set_texture2d_resource(it->resourceName, 0,
                                               i_Image);
          } else if (it->bindType ==
                     ResourceBindType::UNBOUND_SAMPLER) {
            Resource::Image i_Image;

            if (it->resourceScope == ResourceBindScope::LOCAL) {
              i_Image = p_Step.get_resources()[p_RenderFlow]
                            .get_image_resource(it->resourceName);
            } else if (it->resourceScope ==
                       ResourceBindScope::RENDERFLOW) {
              i_Image =
                  p_RenderFlow.get_resources().get_image_resource(
                      it->resourceName);
            } else {
              LOW_ASSERT(false, "Resource bind scope not supported");
            }
            i_Signature.set_unbound_sampler_resource(it->resourceName,
                                                     0, i_Image);
          } else if (it->bindType == ResourceBindType::BUFFER) {
            Resource::Buffer i_Buffer;

            if (it->resourceScope == ResourceBindScope::LOCAL) {
              i_Buffer = p_Step.get_resources()[p_RenderFlow]
                             .get_buffer_resource(it->resourceName);
            } else if (it->resourceScope ==
                       ResourceBindScope::RENDERFLOW) {
              i_Buffer =
                  p_RenderFlow.get_resources().get_buffer_resource(
                      it->resourceName);
            } else {
              LOW_ASSERT(false, "Resource bind scope not supported");
            }
            i_Signature.set_buffer_resource(it->resourceName, 0,
                                            i_Buffer);
          } else {
            LOW_ASSERT(false, "Unsupported resource bind type");
          }
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare_signatures
    }

    void ComputeStep::default_execute(ComputeStep p_Step,
                                      RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_default_execute

      Util::List<ComputePipelineConfig> &l_Configs =
          p_Step.get_config().get_pipelines();
      for (uint32_t i = 0; i < l_Configs.size(); ++i) {
        ComputePipelineConfig &i_Config = l_Configs[i];

        Interface::PipelineResourceSignature i_Signature =
            p_Step.get_signatures()[p_RenderFlow][i];
        i_Signature.commit();

        p_Step.get_pipelines()[p_RenderFlow][i].bind();

        Math::UVector3 i_DispatchDimensions;
        if (i_Config.dispatchConfig.dimensionType ==
            ComputeDispatchDimensionType::ABSOLUTE) {
          i_DispatchDimensions = i_Config.dispatchConfig.absolute;
        } else if (i_Config.dispatchConfig.dimensionType ==
                   ComputeDispatchDimensionType::RELATIVE) {
          Math::UVector2 i_Dimensions;
          if (i_Config.dispatchConfig.relative.target ==
              ComputeDispatchRelativeTarget::RENDERFLOW) {
            i_Dimensions = p_RenderFlow.get_dimensions();
          } else if (i_Config.dispatchConfig.relative.target ==
                     ComputeDispatchRelativeTarget::CONTEXT) {
            i_Dimensions = p_Step.get_context().get_dimensions();
          } else {
            LOW_ASSERT(false,
                       "Unknown dispatch dimensions relative target");
          }
          Math::Vector2 i_FloatDimensions = i_Dimensions;
          i_FloatDimensions *=
              i_Config.dispatchConfig.relative.multiplier;
          i_DispatchDimensions.x = i_FloatDimensions.x;
          i_DispatchDimensions.y = i_FloatDimensions.y;
          i_DispatchDimensions.z = 1;
        } else {
          LOW_ASSERT(false, "Unknown dispatch dimensions type");
        }

        Backend::callbacks().compute_dispatch(
            p_Step.get_context().get_context(), i_DispatchDimensions);
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_default_execute
    }

    uint32_t ComputeStep::create_instance()
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

    void ComputeStep::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer =
          (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                            sizeof(ComputeStepData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(ComputeStepData, resources) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::Map<RenderFlow,
                                             ResourceRegistry>))])
              Util::Map<RenderFlow, ResourceRegistry>();
          *i_ValPtr = it->get_resources();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(ComputeStepData, config) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ComputeStepData, config) *
                          (l_Capacity)],
               l_Capacity * sizeof(ComputeStepConfig));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer
                  [offsetof(ComputeStepData, pipelines) *
                       (l_Capacity + l_CapacityIncrease) +
                   (it->get_index() *
                    sizeof(Util::Map<
                           RenderFlow,
                           Util::List<Interface::ComputePipeline>>))])
              Util::Map<RenderFlow,
                        Util::List<Interface::ComputePipeline>>();
          *i_ValPtr = it->get_pipelines();
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer
                  [offsetof(ComputeStepData, signatures) *
                       (l_Capacity + l_CapacityIncrease) +
                   (it->get_index() *
                    sizeof(Util::Map<
                           RenderFlow,
                           Util::List<
                               Interface::
                                   PipelineResourceSignature>>))])
              Util::Map<
                  RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>>();
          *i_ValPtr = it->get_signatures();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(ComputeStepData, context) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ComputeStepData, context) *
                          (l_Capacity)],
               l_Capacity * sizeof(Interface::Context));
      }
      {
        memcpy(&l_NewBuffer[offsetof(ComputeStepData, output_image) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ComputeStepData, output_image) *
                          (l_Capacity)],
               l_Capacity * sizeof(Resource::Image));
      }
      {
        memcpy(&l_NewBuffer[offsetof(ComputeStepData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ComputeStepData, name) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity;
           i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for ComputeStep from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
