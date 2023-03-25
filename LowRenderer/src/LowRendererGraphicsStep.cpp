#include "LowRendererGraphicsStep.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t GraphicsStep::TYPE_ID = 5;
    uint8_t *GraphicsStep::ms_Buffer = 0;
    Low::Util::Instances::Slot *GraphicsStep::ms_Slots = 0;
    Low::Util::List<GraphicsStep> GraphicsStep::ms_LivingInstances =
        Low::Util::List<GraphicsStep>();

    GraphicsStep::GraphicsStep() : Low::Util::Handle(0ull)
    {
    }
    GraphicsStep::GraphicsStep(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    GraphicsStep::GraphicsStep(GraphicsStep &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    GraphicsStep GraphicsStep::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      GraphicsStepData *l_DataPtr =
          (GraphicsStepData *)&ms_Buffer[l_Index * sizeof(GraphicsStepData)];
      new (l_DataPtr) GraphicsStepData();

      GraphicsStep l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = GraphicsStep::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, GraphicsStep, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

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
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void GraphicsStep::initialize()
    {
      initialize_buffer(&ms_Buffer, GraphicsStepData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_GraphicsStep);
      LOW_PROFILE_ALLOC(type_slots_GraphicsStep);
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

    bool GraphicsStep::is_alive() const
    {
      return m_Data.m_Type == GraphicsStep::TYPE_ID &&
             check_alive(ms_Slots, GraphicsStep::get_capacity());
    }

    uint32_t GraphicsStep::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(GraphicsStep));
      }
      return l_Capacity;
    }

    Util::Map<RenderFlow, ResourceRegistry> &GraphicsStep::get_resources() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStep, resources,
                      SINGLE_ARG(Util::Map<RenderFlow, ResourceRegistry>));
    }

    GraphicsStepConfig GraphicsStep::get_config() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStep, config, GraphicsStepConfig);
    }
    void GraphicsStep::set_config(GraphicsStepConfig p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(GraphicsStep, config, GraphicsStepConfig) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_config
      // LOW_CODEGEN::END::CUSTOM:SETTER_config
    }

    Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>> &
    GraphicsStep::get_pipelines() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(
          GraphicsStep, pipelines,
          SINGLE_ARG(
              Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>>));
    }

    Util::Map<Util::Name, Util::Map<Mesh, Util::List<RenderObject>>> &
    GraphicsStep::get_renderobjects() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(
          GraphicsStep, renderobjects,
          SINGLE_ARG(Util::Map<Util::Name,
                               Util::Map<Mesh, Util::List<RenderObject>>>));
    }

    Util::Map<RenderFlow, Interface::Renderpass> &
    GraphicsStep::get_renderpasses() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStep, renderpasses,
                      SINGLE_ARG(Util::Map<RenderFlow, Interface::Renderpass>));
    }

    Interface::Context GraphicsStep::get_context() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStep, context, Interface::Context);
    }
    void GraphicsStep::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(GraphicsStep, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
      // LOW_CODEGEN::END::CUSTOM:SETTER_context
    }

    Util::Map<RenderFlow, Interface::PipelineResourceSignature> &
    GraphicsStep::get_signatures() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(
          GraphicsStep, signatures,
          SINGLE_ARG(
              Util::Map<RenderFlow, Interface::PipelineResourceSignature>));
    }

    Util::Map<RenderFlow, Resource::Buffer> &
    GraphicsStep::get_object_matrix_buffers() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStep, object_matrix_buffers,
                      SINGLE_ARG(Util::Map<RenderFlow, Resource::Buffer>));
    }

    Low::Util::Name GraphicsStep::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(GraphicsStep, name, Low::Util::Name);
    }
    void GraphicsStep::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

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

    void GraphicsStep::prepare(RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_prepare
      {
        Backend::BufferCreateParams l_Params;
        l_Params.context = &get_context().get_context();
        l_Params.bufferSize = sizeof(Math::Matrix4x4) * 32u;
        l_Params.data = nullptr;
        l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER;

        get_object_matrix_buffers()[p_RenderFlow] =
            Resource::Buffer::make(N(ObjectMatrixBuffer), l_Params);
      }

      {
        Util::List<Backend::PipelineResourceDescription> l_ResourceDescriptions;

        {
          Backend::PipelineResourceDescription l_ResourceDescription;
          l_ResourceDescription.arraySize = 1;
          l_ResourceDescription.name = N(u_RenderObjects);
          l_ResourceDescription.step = Backend::ResourcePipelineStep::VERTEX;
          l_ResourceDescription.type = Backend::ResourceType::BUFFER;

          l_ResourceDescriptions.push_back(l_ResourceDescription);
        }
        get_signatures()[p_RenderFlow] =
            Interface::PipelineResourceSignature::make(N(StepResourceSignature),
                                                       get_context(), 1,
                                                       l_ResourceDescriptions);

        get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_RenderObjects), 0, get_object_matrix_buffers()[p_RenderFlow]);
      }

      {
        Interface::RenderpassCreateParams l_Params;
        l_Params.context = get_context();
        l_Params.dimensions = p_RenderFlow.get_dimensions();
        l_Params.useDepth = false;
        l_Params.clearDepthColor = {1.0f, 0.0f};

        for (uint8_t i = 0u; i < get_config().get_rendertargets().size(); ++i) {
          l_Params.clearColors.push_back({0.0f, 0.5f, 1.0f, 1.0f});

          if (get_config().get_rendertargets()[i].resourceScope ==
              ResourceBindScope::RENDERFLOW) {
            Resource::Image i_Image =
                p_RenderFlow.get_resources().get_image_resource(
                    get_config().get_rendertargets()[i].resourceName);
            LOW_ASSERT(i_Image.is_alive(),
                       "Could not find rendertarget image resource");

            l_Params.renderTargets.push_back(i_Image);
          } else {
            LOW_ASSERT(false, "Unsupported rendertarget resource scope");
          }
        }

        Interface::Renderpass l_Renderpass =
            Interface::Renderpass::make(get_name(), l_Params);
        get_renderpasses()[p_RenderFlow] = l_Renderpass;

        {
          for (uint32_t i = 0u; i < get_config().get_pipelines().size(); ++i) {
            GraphicsPipelineConfig &i_Config = get_config().get_pipelines()[i];
            Interface::PipelineGraphicsCreateParams i_Params;
            i_Params.context = get_context();
            i_Params.cullMode = i_Config.cullMode;
            i_Params.polygonMode = i_Config.polygonMode;
            i_Params.frontFace = i_Config.frontFace;
            i_Params.dimensions = p_RenderFlow.get_dimensions();
            i_Params.signatures = {get_context().get_global_signature(),
                                   get_signatures()[p_RenderFlow]};
            i_Params.vertexShaderPath = i_Config.vertexPath;
            i_Params.fragmentShaderPath = i_Config.fragmentPath;
            i_Params.renderpass = l_Renderpass;
            i_Params.vertexDataAttributeTypes = {
                Backend::VertexAttributeType::VECTOR3};
            for (uint8_t i = 0u; i < get_config().get_rendertargets().size();
                 ++i) {
              // TODO: Temp
              Backend::GraphicsPipelineColorTarget i_ColorTarget;
              i_ColorTarget.blendEnable = false;
              i_ColorTarget.wirteMask = LOW_RENDERER_COLOR_WRITE_BIT_RED |
                                        LOW_RENDERER_COLOR_WRITE_BIT_GREEN |
                                        LOW_RENDERER_COLOR_WRITE_BIT_BLUE |
                                        LOW_RENDERER_COLOR_WRITE_BIT_ALPHA;
              i_Params.colorTargets.push_back(i_ColorTarget);
            }
            get_pipelines()[p_RenderFlow].push_back(
                Interface::GraphicsPipeline::make(i_Config.name, i_Params));
          }
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare
    }

    void GraphicsStep::execute(RenderFlow p_RenderFlow,
                               Math::Matrix4x4 &p_ProjectionMatrix,
                               Math::Matrix4x4 &p_ViewMatrix)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
      if (get_context().get_debug_enabled()) {
        Util::String l_RenderDocLabel =
            Util::String("GraphicsStep - ") + get_name().c_str();
        LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
            get_context().get_context(), l_RenderDocLabel,
            Math::Color(0.234f, 0.341f, 0.4249f, 1.0f));
      }

      get_context().get_global_signature().commit();
      get_signatures()[p_RenderFlow].commit();

      get_renderpasses()[p_RenderFlow].begin();

      Math::Matrix4x4 l_ObjectMatrices[32];
      uint32_t l_ObjectIndex = 0;

      for (auto pit = get_pipelines()[p_RenderFlow].begin();
           pit != get_pipelines()[p_RenderFlow].end(); ++pit) {
        Interface::GraphicsPipeline i_Pipeline = *pit;
        i_Pipeline.bind();

        for (auto mit = get_renderobjects()[pit->get_name()].begin();
             mit != get_renderobjects()[pit->get_name()].end(); ++mit) {
          for (auto it = mit->second.begin(); it != mit->second.end();) {
            RenderObject i_RenderObject = *it;

            if (!i_RenderObject.is_alive()) {
              it = mit->second.erase(it);
              continue;
            }

            Math::Matrix4x4 l_ModelMatrix =
                glm::translate(glm::mat4(1.0f),
                               i_RenderObject.get_world_position()) *
                glm::toMat4(i_RenderObject.get_world_rotation()) *
                glm::scale(glm::mat4(1.0f), i_RenderObject.get_world_scale());

            Math::Matrix4x4 l_MVPMatrix =
                p_ProjectionMatrix * p_ViewMatrix * l_ModelMatrix;

            l_ObjectMatrices[l_ObjectIndex] = l_MVPMatrix;

            l_ObjectIndex++;

            ++it;
          }
        }
      }

      get_object_matrix_buffers()[p_RenderFlow].set(l_ObjectMatrices);

      uint32_t l_InstanceId = 0;

      for (auto pit = get_pipelines()[p_RenderFlow].begin();
           pit != get_pipelines()[p_RenderFlow].end(); ++pit) {
        Interface::GraphicsPipeline i_Pipeline = *pit;
        i_Pipeline.bind();

        for (auto it = get_renderobjects()[pit->get_name()].begin();
             it != get_renderobjects()[pit->get_name()].end(); ++it) {
          Backend::DrawIndexedParams i_Params;
          i_Params.context = &get_context().get_context();
          i_Params.firstIndex = it->first.get_index_buffer_start();
          i_Params.indexCount = it->first.get_index_count();
          i_Params.instanceCount = it->second.size();
          i_Params.vertexOffset = it->first.get_vertex_buffer_start();
          i_Params.firstInstance = l_InstanceId;

          l_InstanceId += it->second.size();

          Backend::callbacks().draw_indexed(i_Params);
        }
      }
      get_renderpasses()[p_RenderFlow].end();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

    void GraphicsStep::register_renderobject(RenderObject p_RenderObject)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_register_renderobject
      for (uint32_t i = 0u; i < get_config().get_pipelines().size(); ++i) {
        GraphicsPipelineConfig &i_Config = get_config().get_pipelines()[i];
        if (i_Config.name == p_RenderObject.get_material()
                                 .get_material_type()
                                 .get_gbuffer_pipeline()
                                 .name) {
          get_renderobjects()[i_Config.name][p_RenderObject.get_mesh()]
              .push_back(p_RenderObject);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_register_renderobject
    }

  } // namespace Renderer
} // namespace Low
