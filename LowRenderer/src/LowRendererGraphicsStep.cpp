#include "LowRendererGraphicsStep.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowRenderer.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t GraphicsStep::TYPE_ID = 13;
    uint32_t GraphicsStep::ms_Capacity = 0u;
    uint8_t *GraphicsStep::ms_Buffer = 0;
    Low::Util::Instances::Slot *GraphicsStep::ms_Slots = 0;
    Low::Util::List<GraphicsStep> GraphicsStep::ms_LivingInstances =
        Low::Util::List<GraphicsStep>();

    GraphicsStep::GraphicsStep() : Low::Util::Handle(0ull)
    {
    }
    GraphicsStep::GraphicsStep(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    GraphicsStep::GraphicsStep(GraphicsStep &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle GraphicsStep::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    GraphicsStep GraphicsStep::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      GraphicsStep l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = GraphicsStep::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(
          l_Handle, GraphicsStep, resources,
          SINGLE_ARG(Util::Map<RenderFlow, ResourceRegistry>)))
          Util::Map<RenderFlow, ResourceRegistry>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStep, config,
                              GraphicsStepConfig))
          GraphicsStepConfig();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, GraphicsStep, pipelines,
          SINGLE_ARG(
              Util::Map<RenderFlow,
                        Util::List<Interface::GraphicsPipeline>>)))
          Util::Map<RenderFlow,
                    Util::List<Interface::GraphicsPipeline>>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, GraphicsStep, renderobjects,
          SINGLE_ARG(
              Util::Map<Util::Name,
                        Util::Map<Mesh, Util::List<RenderObject>>>)))
          Util::Map<Util::Name,
                    Util::Map<Mesh, Util::List<RenderObject>>>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, GraphicsStep, skinned_renderobjects,
          SINGLE_ARG(
              Util::Map<Util::Name, Util::List<RenderObject>>)))
          Util::Map<Util::Name, Util::List<RenderObject>>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, GraphicsStep, renderpasses,
          SINGLE_ARG(Util::Map<RenderFlow, Interface::Renderpass>)))
          Util::Map<RenderFlow, Interface::Renderpass>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStep, context,
                              Interface::Context))
          Interface::Context();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, GraphicsStep, pipeline_signatures,
          SINGLE_ARG(
              Util::Map<
                  RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>>)))
          Util::Map<
              RenderFlow,
              Util::List<Interface::PipelineResourceSignature>>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, GraphicsStep, signatures,
          SINGLE_ARG(
              Util::Map<RenderFlow,
                        Interface::PipelineResourceSignature>)))
          Util::Map<RenderFlow,
                    Interface::PipelineResourceSignature>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, GraphicsStep, output_image,
                              Resource::Image)) Resource::Image();
      ACCESSOR_TYPE_SOA(l_Handle, GraphicsStep, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void GraphicsStep::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const GraphicsStep *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void GraphicsStep::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(GraphicsStep));

      initialize_buffer(&ms_Buffer, GraphicsStepData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_GraphicsStep);
      LOW_PROFILE_ALLOC(type_slots_GraphicsStep);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GraphicsStep);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GraphicsStep::is_alive;
      l_TypeInfo.destroy = &GraphicsStep::destroy;
      l_TypeInfo.serialize = &GraphicsStep::serialize;
      l_TypeInfo.deserialize = &GraphicsStep::deserialize;
      l_TypeInfo.find_by_index = &GraphicsStep::_find_by_index;
      l_TypeInfo.find_by_name = &GraphicsStep::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &GraphicsStep::_make;
      l_TypeInfo.duplicate_default = &GraphicsStep::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &GraphicsStep::living_instances);
      l_TypeInfo.get_living_count = &GraphicsStep::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resources);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, resources);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_resources();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, resources,
              SINGLE_ARG(Util::Map<RenderFlow, ResourceRegistry>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(config);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, config);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = GraphicsStepConfig::TYPE_ID;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_config();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, config, GraphicsStepConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pipelines);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, pipelines);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_pipelines();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, pipelines,
              SINGLE_ARG(Util::Map<
                         RenderFlow,
                         Util::List<Interface::GraphicsPipeline>>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(renderobjects);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, renderobjects);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_renderobjects();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, renderobjects,
              SINGLE_ARG(Util::Map<
                         Util::Name,
                         Util::Map<Mesh, Util::List<RenderObject>>>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(skinned_renderobjects);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, skinned_renderobjects);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_skinned_renderobjects();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, skinned_renderobjects,
              SINGLE_ARG(
                  Util::Map<Util::Name, Util::List<RenderObject>>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(renderpasses);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, renderpasses);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_renderpasses();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, renderpasses,
              SINGLE_ARG(
                  Util::Map<RenderFlow, Interface::Renderpass>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, context);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Interface::Context::TYPE_ID;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_context();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, context, Interface::Context);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pipeline_signatures);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, pipeline_signatures);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_pipeline_signatures();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, pipeline_signatures,
              SINGLE_ARG(Util::Map<
                         RenderFlow,
                         Util::List<
                             Interface::PipelineResourceSignature>>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.set_pipeline_signatures(
              *(Util::Map<
                  RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>> *)
                  p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(signatures);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, signatures);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_signatures();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, signatures,
              SINGLE_ARG(
                  Util::Map<RenderFlow,
                            Interface::PipelineResourceSignature>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(output_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, output_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Resource::Image::TYPE_ID;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_output_image();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, GraphicsStep, output_image, Resource::Image);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.set_output_image(*(Resource::Image *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, GraphicsStep,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          GraphicsStep l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = GraphicsStep::TYPE_ID;
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
          l_ParameterInfo.handleType = GraphicsStepConfig::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(clear_renderobjects);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
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
      }
      {
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
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ProjectionMatrix);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ViewMatrix);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(register_renderobject);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderObject);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
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
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(fill_pipeline_signatures);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = GraphicsStep::TYPE_ID;
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
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(create_signature);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = GraphicsStep::TYPE_ID;
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
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(create_renderpass);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = GraphicsStep::TYPE_ID;
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
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(create_pipelines);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = GraphicsStep::TYPE_ID;
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
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_UpdateExisting);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(default_execute);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = GraphicsStep::TYPE_ID;
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
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ProjectionMatrix);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ViewMatrix);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(default_execute_fullscreen_triangle);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = GraphicsStep::TYPE_ID;
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
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ProjectionMatrix);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_ViewMatrix);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(draw_renderobjects);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = GraphicsStep::TYPE_ID;
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
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void GraphicsStep::cleanup()
    {
      Low::Util::List<GraphicsStep> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_GraphicsStep);
      LOW_PROFILE_FREE(type_slots_GraphicsStep);
    }

    Low::Util::Handle GraphicsStep::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    GraphicsStep GraphicsStep::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      GraphicsStep l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = GraphicsStep::TYPE_ID;

      return l_Handle;
    }

    bool GraphicsStep::is_alive() const
    {
      return m_Data.m_Type == GraphicsStep::TYPE_ID &&
             check_alive(ms_Slots, GraphicsStep::get_capacity());
    }

    uint32_t GraphicsStep::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    GraphicsStep::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    GraphicsStep GraphicsStep::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    GraphicsStep GraphicsStep::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      GraphicsStep l_Handle = make(p_Name);
      if (get_config().is_alive()) {
        l_Handle.set_config(get_config());
      }
      if (get_context().is_alive()) {
        l_Handle.set_context(get_context());
      }
      l_Handle.set_pipeline_signatures(get_pipeline_signatures());
      if (get_output_image().is_alive()) {
        l_Handle.set_output_image(get_output_image());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    GraphicsStep GraphicsStep::duplicate(GraphicsStep p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    GraphicsStep::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      GraphicsStep l_GraphicsStep = p_Handle.get_id();
      return l_GraphicsStep.duplicate(p_Name);
    }

    void GraphicsStep::serialize(Low::Util::Yaml::Node &p_Node) const
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

    void GraphicsStep::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      GraphicsStep l_GraphicsStep = p_Handle.get_id();
      l_GraphicsStep.serialize(p_Node);
    }

    Low::Util::Handle
    GraphicsStep::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {
      GraphicsStep l_Handle = GraphicsStep::make(N(GraphicsStep));

      if (p_Node["resources"]) {
      }
      if (p_Node["config"]) {
        l_Handle.set_config(GraphicsStepConfig::deserialize(
                                p_Node["config"], l_Handle.get_id())
                                .get_id());
      }
      if (p_Node["pipelines"]) {
      }
      if (p_Node["renderobjects"]) {
      }
      if (p_Node["skinned_renderobjects"]) {
      }
      if (p_Node["renderpasses"]) {
      }
      if (p_Node["context"]) {
        l_Handle.set_context(Interface::Context::deserialize(
                                 p_Node["context"], l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["pipeline_signatures"]) {
      }
      if (p_Node["signatures"]) {
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
    GraphicsStep::get_resources() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resources
      // LOW_CODEGEN::END::CUSTOM:GETTER_resources

      return TYPE_SOA(
          GraphicsStep, resources,
          SINGLE_ARG(Util::Map<RenderFlow, ResourceRegistry>));
    }

    GraphicsStepConfig GraphicsStep::get_config() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_config
      // LOW_CODEGEN::END::CUSTOM:GETTER_config

      return TYPE_SOA(GraphicsStep, config, GraphicsStepConfig);
    }
    void GraphicsStep::set_config(GraphicsStepConfig p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_config
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_config

      // Set new value
      TYPE_SOA(GraphicsStep, config, GraphicsStepConfig) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_config
      // LOW_CODEGEN::END::CUSTOM:SETTER_config
    }

    Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>> &
    GraphicsStep::get_pipelines() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pipelines
      // LOW_CODEGEN::END::CUSTOM:GETTER_pipelines

      return TYPE_SOA(
          GraphicsStep, pipelines,
          SINGLE_ARG(
              Util::Map<RenderFlow,
                        Util::List<Interface::GraphicsPipeline>>));
    }

    Util::Map<Util::Name, Util::Map<Mesh, Util::List<RenderObject>>> &
    GraphicsStep::get_renderobjects() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderobjects
      // LOW_CODEGEN::END::CUSTOM:GETTER_renderobjects

      return TYPE_SOA(
          GraphicsStep, renderobjects,
          SINGLE_ARG(
              Util::Map<Util::Name,
                        Util::Map<Mesh, Util::List<RenderObject>>>));
    }

    Util::Map<Util::Name, Util::List<RenderObject>> &
    GraphicsStep::get_skinned_renderobjects() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_skinned_renderobjects
      // LOW_CODEGEN::END::CUSTOM:GETTER_skinned_renderobjects

      return TYPE_SOA(
          GraphicsStep, skinned_renderobjects,
          SINGLE_ARG(
              Util::Map<Util::Name, Util::List<RenderObject>>));
    }

    Util::Map<RenderFlow, Interface::Renderpass> &
    GraphicsStep::get_renderpasses() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderpasses
      // LOW_CODEGEN::END::CUSTOM:GETTER_renderpasses

      return TYPE_SOA(
          GraphicsStep, renderpasses,
          SINGLE_ARG(Util::Map<RenderFlow, Interface::Renderpass>));
    }

    Interface::Context GraphicsStep::get_context() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context
      // LOW_CODEGEN::END::CUSTOM:GETTER_context

      return TYPE_SOA(GraphicsStep, context, Interface::Context);
    }
    void GraphicsStep::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_context
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_context

      // Set new value
      TYPE_SOA(GraphicsStep, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
      // LOW_CODEGEN::END::CUSTOM:SETTER_context
    }

    Util::Map<RenderFlow,
              Util::List<Interface::PipelineResourceSignature>> &
    GraphicsStep::get_pipeline_signatures() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pipeline_signatures
      // LOW_CODEGEN::END::CUSTOM:GETTER_pipeline_signatures

      return TYPE_SOA(
          GraphicsStep, pipeline_signatures,
          SINGLE_ARG(
              Util::Map<
                  RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>>));
    }
    void GraphicsStep::set_pipeline_signatures(
        Util::Map<RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>>
            &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pipeline_signatures
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_pipeline_signatures

      // Set new value
      TYPE_SOA(
          GraphicsStep, pipeline_signatures,
          SINGLE_ARG(
              Util::Map<RenderFlow,
                        Util::List<
                            Interface::PipelineResourceSignature>>)) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pipeline_signatures
      // LOW_CODEGEN::END::CUSTOM:SETTER_pipeline_signatures
    }

    Util::Map<RenderFlow, Interface::PipelineResourceSignature> &
    GraphicsStep::get_signatures() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_signatures
      // LOW_CODEGEN::END::CUSTOM:GETTER_signatures

      return TYPE_SOA(
          GraphicsStep, signatures,
          SINGLE_ARG(
              Util::Map<RenderFlow,
                        Interface::PipelineResourceSignature>));
    }

    Resource::Image GraphicsStep::get_output_image() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_output_image
      // LOW_CODEGEN::END::CUSTOM:GETTER_output_image

      return TYPE_SOA(GraphicsStep, output_image, Resource::Image);
    }
    void GraphicsStep::set_output_image(Resource::Image p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_output_image
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_output_image

      // Set new value
      TYPE_SOA(GraphicsStep, output_image, Resource::Image) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_output_image
      // LOW_CODEGEN::END::CUSTOM:SETTER_output_image
    }

    Low::Util::Name GraphicsStep::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(GraphicsStep, name, Low::Util::Name);
    }
    void GraphicsStep::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(GraphicsStep, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    GraphicsStep GraphicsStep::make(Util::Name p_Name,
                                    Interface::Context p_Context,
                                    GraphicsStepConfig p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      GraphicsStep l_Step = GraphicsStep::make(p_Name);
      l_Step.set_config(p_Config);
      l_Step.set_context(p_Context);

      return l_Step;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void GraphicsStep::clear_renderobjects()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_clear_renderobjects
      get_renderobjects().clear();
      get_skinned_renderobjects().clear();

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_clear_renderobjects
    }

    void GraphicsStep::prepare(RenderFlow p_RenderFlow)
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

      get_config().get_callbacks().setup_signature(*this,
                                                   p_RenderFlow);
      get_config().get_callbacks().setup_renderpass(*this,
                                                    p_RenderFlow);
      get_config().get_callbacks().setup_pipelines(
          *this, p_RenderFlow, false);

      fill_pipeline_signatures(*this, p_RenderFlow);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare
    }

    void GraphicsStep::execute(RenderFlow p_RenderFlow,
                               Math::Matrix4x4 &p_ProjectionMatrix,
                               Math::Matrix4x4 &p_ViewMatrix)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
      Util::String l_ProfileString = get_name().c_str();
      l_ProfileString += " (GraphicsStep execute)";
      LOW_PROFILE_CPU("Renderer", l_ProfileString.c_str());
      if (get_context().is_debug_enabled()) {
        Util::String l_RenderDocLabel =
            Util::String("GraphicsStep - ") + get_name().c_str();
        LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
            get_context().get_context(), l_RenderDocLabel,
            Math::Color(0.234f, 0.341f, 0.4249f, 1.0f));
      }

      get_context().get_global_signature().commit();
      p_RenderFlow.get_resource_signature().commit();
      get_signatures()[p_RenderFlow].commit();

      get_config().get_callbacks().execute(
          *this, p_RenderFlow, p_ProjectionMatrix, p_ViewMatrix);

      clear_renderobjects();

      if (get_context().is_debug_enabled()) {
        LOW_RENDERER_END_RENDERDOC_SECTION(
            get_context().get_context());
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

    void
    GraphicsStep::register_renderobject(RenderObject &p_RenderObject)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_register_renderobject
      MaterialType l_MaterialType =
          p_RenderObject.material.get_material_type();

      if (p_RenderObject.useSkinningBuffer) {
        for (uint32_t i = 0u; i < get_config().get_pipelines().size();
             ++i) {
          GraphicsPipelineConfig &i_Config =
              get_config().get_pipelines()[i];
          if (i_Config.name ==
                  l_MaterialType.get_gbuffer_pipeline().name ||
              i_Config.name ==
                  l_MaterialType.get_depth_pipeline().name) {
            get_skinned_renderobjects()[i_Config.name].push_back(
                p_RenderObject);
          }
        }
      } else {
        for (uint32_t i = 0u; i < get_config().get_pipelines().size();
             ++i) {
          GraphicsPipelineConfig &i_Config =
              get_config().get_pipelines()[i];
          if (i_Config.name ==
                  l_MaterialType.get_gbuffer_pipeline().name ||
              i_Config.name ==
                  l_MaterialType.get_depth_pipeline().name) {
            get_renderobjects()[i_Config.name][p_RenderObject.mesh]
                .push_back(p_RenderObject);
          }
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_register_renderobject
    }

    void GraphicsStep::update_dimensions(RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_dimensions
      get_resources()[p_RenderFlow].update_dimensions(p_RenderFlow);

      // Recreate renderpass
      get_renderpasses()[p_RenderFlow].destroy();
      get_config().get_callbacks().setup_renderpass(*this,
                                                    p_RenderFlow);
      get_config().get_callbacks().setup_pipelines(
          *this, p_RenderFlow, true);

      fill_pipeline_signatures(*this, p_RenderFlow);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_dimensions
    }

    void
    GraphicsStep::fill_pipeline_signatures(GraphicsStep p_Step,
                                           RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_fill_pipeline_signatures
      Util::List<GraphicsPipelineConfig> &l_Configs =
          p_Step.get_config().get_pipelines();
      for (uint32_t i = 0; i < l_Configs.size(); ++i) {
        GraphicsPipelineConfig &i_Config = l_Configs[i];

        if (p_Step.get_pipeline_signatures()[p_RenderFlow].size() <=
            i) {
          continue;
        }

        Interface::PipelineResourceSignature i_Signature =
            p_Step.get_pipeline_signatures()[p_RenderFlow][i];
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
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_fill_pipeline_signatures
    }

    void GraphicsStep::create_signature(GraphicsStep p_Step,
                                        RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_create_signature
      Util::List<Backend::PipelineResourceDescription>
          l_ResourceDescriptions;

      {
        Backend::PipelineResourceDescription l_ResourceDescription;
        l_ResourceDescription.arraySize = 1;
        l_ResourceDescription.name = N(u_RenderObjects);
        l_ResourceDescription.step =
            Backend::ResourcePipelineStep::GRAPHICS;
        l_ResourceDescription.type = Backend::ResourceType::BUFFER;

        l_ResourceDescriptions.push_back(l_ResourceDescription);
      }
      {
        Backend::PipelineResourceDescription l_ResourceDescription;
        l_ResourceDescription.arraySize = 1;
        l_ResourceDescription.name = N(u_Colors);
        l_ResourceDescription.step =
            Backend::ResourcePipelineStep::GRAPHICS;
        l_ResourceDescription.type = Backend::ResourceType::BUFFER;

        l_ResourceDescriptions.push_back(l_ResourceDescription);
      }
      p_Step.get_signatures()[p_RenderFlow] =
          Interface::PipelineResourceSignature::make(
              N(StepResourceSignature), p_Step.get_context(), 2,
              l_ResourceDescriptions);

      p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
          N(u_RenderObjects), 0,
          p_Step.get_resources()[p_RenderFlow].get_buffer_resource(
              N(_renderobject_buffer)));

      p_Step.get_signatures()[p_RenderFlow].set_buffer_resource(
          N(u_Colors), 0,
          p_Step.get_resources()[p_RenderFlow].get_buffer_resource(
              N(_color_buffer)));

      for (uint32_t i = 0;
           i < p_Step.get_config().get_pipelines().size(); ++i) {
        GraphicsPipelineConfig &i_Config =
            p_Step.get_config().get_pipelines()[i];

        Util::List<Backend::PipelineResourceDescription>
            i_ResourceDescriptions;
        for (auto it = i_Config.resourceBinding.begin();
             it != i_Config.resourceBinding.end(); ++it) {
          Backend::PipelineResourceDescription i_Resource;
          i_Resource.name = it->resourceName;
          i_Resource.step = Backend::ResourcePipelineStep::GRAPHICS;

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

        p_Step.get_pipeline_signatures()[p_RenderFlow].push_back(
            Interface::PipelineResourceSignature::make(
                p_Step.get_name(), p_Step.get_context(), 3,
                i_ResourceDescriptions));
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_create_signature
    }

    void GraphicsStep::create_renderpass(GraphicsStep p_Step,
                                         RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_create_renderpass
      Interface::RenderpassCreateParams l_Params;
      l_Params.context = p_Step.get_context();
      apply_dimensions_config(
          p_Step.get_context(), p_RenderFlow,
          p_Step.get_config().get_dimensions_config(),
          l_Params.dimensions);
      l_Params.useDepth = p_Step.get_config().is_use_depth();
      if (p_Step.get_config().is_depth_clear()) {
        l_Params.clearDepthColor = {1.0f, 1.0f};
      } else {
        l_Params.clearDepthColor = {1.0f, 0.0f};
      }

      for (uint8_t i = 0u;
           i < p_Step.get_config().get_rendertargets().size(); ++i) {

        l_Params.clearColors.push_back(
            p_Step.get_config().get_rendertargets_clearcolor());

        if (p_Step.get_config()
                .get_rendertargets()[i]
                .resourceScope == ResourceBindScope::LOCAL) {
          Resource::Image i_Image =
              p_Step.get_resources()[p_RenderFlow].get_image_resource(
                  p_Step.get_config()
                      .get_rendertargets()[i]
                      .resourceName);
          LOW_ASSERT(i_Image.is_alive(),
                     "Could not find rendertarget image resource");

          l_Params.renderTargets.push_back(i_Image);
        } else if (p_Step.get_config()
                       .get_rendertargets()[i]
                       .resourceScope ==
                   ResourceBindScope::RENDERFLOW) {
          Resource::Image i_Image =
              p_RenderFlow.get_resources().get_image_resource(
                  p_Step.get_config()
                      .get_rendertargets()[i]
                      .resourceName);
          LOW_ASSERT(i_Image.is_alive(),
                     "Could not find rendertarget image resource");

          l_Params.renderTargets.push_back(i_Image);
        } else {
          LOW_ASSERT(false,
                     "Unsupported rendertarget resource scope");
        }
      }

      if (p_Step.get_config().is_use_depth()) {
        Resource::Image l_Image =
            p_RenderFlow.get_resources().get_image_resource(
                p_Step.get_config()
                    .get_depth_rendertarget()
                    .resourceName);
        LOW_ASSERT(l_Image.is_alive(),
                   "Could not find rendertarget image resource");
        l_Params.depthRenderTarget = l_Image;
      }

      p_Step.get_renderpasses()[p_RenderFlow] =
          Interface::Renderpass::make(p_Step.get_name(), l_Params);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_create_renderpass
    }

    void GraphicsStep::create_pipelines(GraphicsStep p_Step,
                                        RenderFlow p_RenderFlow,
                                        bool p_UpdateExisting)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_create_pipelines
      Math::UVector2 l_Dimensions;
      apply_dimensions_config(
          p_Step.get_context(), p_RenderFlow,
          p_Step.get_config().get_dimensions_config(), l_Dimensions);

      for (uint32_t i = 0u;
           i < p_Step.get_config().get_pipelines().size(); ++i) {
        GraphicsPipelineConfig &i_Config =
            p_Step.get_config().get_pipelines()[i];
        Interface::PipelineGraphicsCreateParams i_Params;
        i_Params.context = p_Step.get_context();
        i_Params.cullMode = i_Config.cullMode;
        i_Params.polygonMode = i_Config.polygonMode;
        i_Params.frontFace = i_Config.frontFace;
        i_Params.dimensions = l_Dimensions;
        i_Params.signatures = {
            p_Step.get_context().get_global_signature(),
            p_RenderFlow.get_resource_signature(),
            p_Step.get_signatures()[p_RenderFlow]};
        if (p_Step.get_pipeline_signatures()[p_RenderFlow].size() >
            i) {
          i_Params.signatures = {
              p_Step.get_context().get_global_signature(),
              p_RenderFlow.get_resource_signature(),
              p_Step.get_signatures()[p_RenderFlow],
              p_Step.get_pipeline_signatures()[p_RenderFlow][i]};
        }
        i_Params.vertexShaderPath = i_Config.vertexPath;
        i_Params.fragmentShaderPath = i_Config.fragmentPath;
        i_Params.renderpass = p_Step.get_renderpasses()[p_RenderFlow];
        i_Params.depthTest = p_Step.get_config().is_depth_test();
        i_Params.depthWrite = p_Step.get_config().is_depth_write();
        i_Params.depthCompareOperation =
            p_Step.get_config().get_depth_compare_operation();
        i_Params.vertexDataAttributeTypes = {
            Backend::VertexAttributeType::VECTOR3,
            Backend::VertexAttributeType::VECTOR2,
            Backend::VertexAttributeType::VECTOR3,
            Backend::VertexAttributeType::VECTOR3,
            Backend::VertexAttributeType::VECTOR3};
        for (uint8_t i = 0u;
             i < p_Step.get_config().get_rendertargets().size();
             ++i) {
          Backend::GraphicsPipelineColorTarget i_ColorTarget;

          Resource::Image i_Image = 0;
          if (p_Step.get_config()
                  .get_rendertargets()[i]
                  .resourceScope == ResourceBindScope::LOCAL) {
            i_Image =
                p_Step.get_resources()[p_RenderFlow]
                    .get_image_resource(p_Step.get_config()
                                            .get_rendertargets()[i]
                                            .resourceName);
            LOW_ASSERT(i_Image.is_alive(),
                       "Could not find rendertarget image resource");

            i_ColorTarget.wirteMask =
                Backend::imageformat_get_pipeline_write_mask(
                    i_Image.get_image().format);
          } else if (p_Step.get_config()
                         .get_rendertargets()[i]
                         .resourceScope ==
                     ResourceBindScope::RENDERFLOW) {
            i_Image = p_RenderFlow.get_resources().get_image_resource(
                p_Step.get_config()
                    .get_rendertargets()[i]
                    .resourceName);
            LOW_ASSERT(i_Image.is_alive(),
                       "Could not find rendertarget image resource");

            i_ColorTarget.wirteMask =
                Backend::imageformat_get_pipeline_write_mask(
                    i_Image.get_image().format);
          } else {
            LOW_ASSERT(false,
                       "Unsupported rendertarget resource scope");
          }

          i_ColorTarget.blendEnable = false;

          if (i_Image.get_image().format ==
                  Backend::ImageFormat::RGBA16_SFLOAT ||
              i_Image.get_image().format ==
                  Backend::ImageFormat::RGBA32_SFLOAT) {
            i_ColorTarget.blendEnable = i_Config.translucency;
          }
          i_Params.colorTargets.push_back(i_ColorTarget);
        }

        if (p_UpdateExisting) {
          Interface::PipelineManager::register_graphics_pipeline(
              p_Step.get_pipelines()[p_RenderFlow][i], i_Params);
        } else {
          p_Step.get_pipelines()[p_RenderFlow].push_back(
              Interface::GraphicsPipeline::make(i_Config.name,
                                                i_Params));
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_create_pipelines
    }

    void
    GraphicsStep::default_execute(GraphicsStep p_Step,
                                  RenderFlow p_RenderFlow,
                                  Math::Matrix4x4 &p_ProjectionMatrix,
                                  Math::Matrix4x4 &p_ViewMatrix)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_default_execute
      RenderObjectShaderInfo
          l_ObjectShaderInfos[LOW_RENDERER_RENDEROBJECT_COUNT];
      Math::Vector4 l_Colors[LOW_RENDERER_RENDEROBJECT_COUNT];
      uint32_t l_ObjectIndex = 0;

      for (auto pit = p_Step.get_pipelines()[p_RenderFlow].begin();
           pit != p_Step.get_pipelines()[p_RenderFlow].end(); ++pit) {
        Interface::GraphicsPipeline i_GraphicsPipeline = *pit;
        if (!i_GraphicsPipeline.is_alive()) {
          continue;
        }

        {
          LOW_PROFILE_CPU("Renderer", "Collect renderobjects");
          for (auto mit =
                   p_Step
                       .get_skinned_renderobjects()[i_GraphicsPipeline
                                                        .get_name()]
                       .begin();
               mit !=
               p_Step
                   .get_skinned_renderobjects()[i_GraphicsPipeline

                                                    .get_name()]
                   .end();
               ++mit) {
            RenderObject &i_RenderObject = *mit;

            Math::Matrix4x4 l_MVPMatrix = p_ProjectionMatrix *
                                          p_ViewMatrix *
                                          i_RenderObject.transform;

            l_Colors[l_ObjectIndex] = i_RenderObject.color;

            l_ObjectShaderInfos[l_ObjectIndex].mvp = l_MVPMatrix;
            l_ObjectShaderInfos[l_ObjectIndex].model_matrix =
                i_RenderObject.transform;

            l_ObjectShaderInfos[l_ObjectIndex].material_index =
                i_RenderObject.material.get_index();
            l_ObjectShaderInfos[l_ObjectIndex].entity_id =
                i_RenderObject.entity_id;
            l_ObjectShaderInfos[l_ObjectIndex].texture_index =
                i_RenderObject.texture.get_index();

            l_ObjectIndex++;
          }

          for (auto mit = p_Step
                              .get_renderobjects()[i_GraphicsPipeline
                                                       .get_name()]
                              .begin();
               mit !=
               p_Step
                   .get_renderobjects()[i_GraphicsPipeline.get_name()]
                   .end();
               ++mit) {
            for (auto it = mit->second.begin();
                 it != mit->second.end();) {
              RenderObject &i_RenderObject = *it;

              Math::Matrix4x4 l_MVPMatrix = p_ProjectionMatrix *
                                            p_ViewMatrix *
                                            i_RenderObject.transform;
              l_Colors[l_ObjectIndex] = i_RenderObject.color;

              l_ObjectShaderInfos[l_ObjectIndex].mvp = l_MVPMatrix;
              l_ObjectShaderInfos[l_ObjectIndex].model_matrix =
                  i_RenderObject.transform;

              l_ObjectShaderInfos[l_ObjectIndex].material_index =
                  i_RenderObject.material.get_index();
              l_ObjectShaderInfos[l_ObjectIndex].entity_id =
                  i_RenderObject.entity_id;

              l_ObjectIndex++;

              ++it;
            }
          }
        }
      }

      {
        LOW_PROFILE_CPU("Renderer", "Write buffers");
        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(_renderobject_buffer))
            .set((void *)l_ObjectShaderInfos);

        p_Step.get_resources()[p_RenderFlow]
            .get_buffer_resource(N(_color_buffer))
            .set((void *)l_Colors);
      }

      {
        LOW_PROFILE_CPU("Renderer", "Renderpass");
        p_Step.get_renderpasses()[p_RenderFlow].begin();
        GraphicsStep::draw_renderobjects(p_Step, p_RenderFlow);
        p_Step.get_renderpasses()[p_RenderFlow].end();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_default_execute
    }

    void GraphicsStep::default_execute_fullscreen_triangle(
        GraphicsStep p_Step, RenderFlow p_RenderFlow,
        Math::Matrix4x4 &p_ProjectionMatrix,
        Math::Matrix4x4 &p_ViewMatrix)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_default_execute_fullscreen_triangle
      uint32_t l_PipelineIndex = 0;

      for (auto pit = p_Step.get_pipelines()[p_RenderFlow].begin();
           pit != p_Step.get_pipelines()[p_RenderFlow].end(); ++pit) {
        Interface::GraphicsPipeline i_Pipeline = *pit;

        Interface::PipelineResourceSignature i_Signature =
            p_Step.get_pipeline_signatures()[p_RenderFlow]
                                            [l_PipelineIndex];
        i_Signature.commit();
        p_Step.get_renderpasses()[p_RenderFlow].begin();

        i_Pipeline.bind();

        {
          Backend::DrawParams i_Params;
          i_Params.context = &p_Step.get_context().get_context();
          i_Params.firstVertex = 0;
          i_Params.vertexCount = 3;
          Backend::callbacks().draw(i_Params);
        }

        l_PipelineIndex++;
        p_Step.get_renderpasses()[p_RenderFlow].end();
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_default_execute_fullscreen_triangle
    }

    void GraphicsStep::draw_renderobjects(GraphicsStep p_Step,
                                          RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_draw_renderobjects
      uint32_t l_InstanceId = 0;

      LOW_PROFILE_CPU("Renderer", "Draw renderobjects");

      for (auto pit = p_Step.get_pipelines()[p_RenderFlow].begin();
           pit != p_Step.get_pipelines()[p_RenderFlow].end(); ++pit) {
        Interface::GraphicsPipeline i_Pipeline = *pit;
        i_Pipeline.bind();

        bool i_SkinnedBound = false;
        bool i_VertexBound = false;

        for (auto it =
                 p_Step.get_skinned_renderobjects()[pit->get_name()]
                     .begin();
             it != p_Step.get_skinned_renderobjects()[pit->get_name()]
                       .end();
             ++it) {
          if (!i_SkinnedBound) {
            get_skinning_buffer().bind_vertex();
            i_SkinnedBound = true;
          }

          Backend::DrawIndexedParams i_Params;
          i_Params.context = &p_Step.get_context().get_context();
          i_Params.firstIndex = it->mesh.get_index_buffer_start();
          i_Params.indexCount = it->mesh.get_index_count();
          i_Params.instanceCount = 1;
          i_Params.vertexOffset = it->vertexBufferStartOverride;
          i_Params.firstInstance = l_InstanceId;

          l_InstanceId++;

          Backend::callbacks().draw_indexed(i_Params);
        }

        for (auto it =
                 p_Step.get_renderobjects()[pit->get_name()].begin();
             it != p_Step.get_renderobjects()[pit->get_name()].end();
             ++it) {
          if (!i_VertexBound) {
            get_vertex_buffer().bind_vertex();
            i_VertexBound = true;
          }

          Backend::DrawIndexedParams i_Params;
          i_Params.context = &p_Step.get_context().get_context();
          i_Params.firstIndex = it->first.get_index_buffer_start();
          i_Params.indexCount = it->first.get_index_count();
          i_Params.instanceCount = it->second.size();
          i_Params.vertexOffset = it->first.get_vertex_buffer_start();
          i_Params.firstInstance = l_InstanceId;

          l_InstanceId += it->second.size();

          Backend::callbacks().draw_indexed(i_Params);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_draw_renderobjects
    }

    uint32_t GraphicsStep::create_instance()
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

    void GraphicsStep::increase_budget()
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
                            sizeof(GraphicsStepData));
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
              &l_NewBuffer[offsetof(GraphicsStepData, resources) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::Map<RenderFlow,
                                             ResourceRegistry>))])
              Util::Map<RenderFlow, ResourceRegistry>();
          *i_ValPtr = it->get_resources();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepData, config) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepData, config) *
                          (l_Capacity)],
               l_Capacity * sizeof(GraphicsStepConfig));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer
                  [offsetof(GraphicsStepData, pipelines) *
                       (l_Capacity + l_CapacityIncrease) +
                   (it->get_index() *
                    sizeof(Util::Map<
                           RenderFlow,
                           Util::List<
                               Interface::GraphicsPipeline>>))])
              Util::Map<RenderFlow,
                        Util::List<Interface::GraphicsPipeline>>();
          *i_ValPtr = it->get_pipelines();
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer
                  [offsetof(GraphicsStepData, renderobjects) *
                       (l_Capacity + l_CapacityIncrease) +
                   (it->get_index() *
                    sizeof(Util::Map<
                           Util::Name,
                           Util::Map<Mesh,
                                     Util::List<RenderObject>>>))])
              Util::Map<Util::Name,
                        Util::Map<Mesh, Util::List<RenderObject>>>();
          *i_ValPtr = it->get_renderobjects();
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer
                  [offsetof(GraphicsStepData, skinned_renderobjects) *
                       (l_Capacity + l_CapacityIncrease) +
                   (it->get_index() *
                    sizeof(Util::Map<Util::Name,
                                     Util::List<RenderObject>>))])
              Util::Map<Util::Name, Util::List<RenderObject>>();
          *i_ValPtr = it->get_skinned_renderobjects();
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(GraphicsStepData, renderpasses) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(
                                Util::Map<RenderFlow,
                                          Interface::Renderpass>))])
              Util::Map<RenderFlow, Interface::Renderpass>();
          *i_ValPtr = it->get_renderpasses();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepData, context) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepData, context) *
                          (l_Capacity)],
               l_Capacity * sizeof(Interface::Context));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer
                  [offsetof(GraphicsStepData, pipeline_signatures) *
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
          *i_ValPtr = it->get_pipeline_signatures();
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer
                  [offsetof(GraphicsStepData, signatures) *
                       (l_Capacity + l_CapacityIncrease) +
                   (it->get_index() *
                    sizeof(Util::Map<
                           RenderFlow,
                           Interface::PipelineResourceSignature>))])
              Util::Map<RenderFlow,
                        Interface::PipelineResourceSignature>();
          *i_ValPtr = it->get_signatures();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepData, output_image) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepData, output_image) *
                          (l_Capacity)],
               l_Capacity * sizeof(Resource::Image));
      }
      {
        memcpy(&l_NewBuffer[offsetof(GraphicsStepData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(GraphicsStepData, name) *
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

      LOW_LOG_DEBUG << "Auto-increased budget for GraphicsStep from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
