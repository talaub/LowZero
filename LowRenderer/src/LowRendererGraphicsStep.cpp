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

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(GraphicsStep);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &GraphicsStep::is_alive;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resources);
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, resources);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&GraphicsStep::ms_Buffer
              [p_Handle.get_index() * GraphicsStepData::get_size() +
               offsetof(GraphicsStepData, resources)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(Util::Map<RenderFlow, ResourceRegistry> *)&GraphicsStep::ms_Buffer
               [p_Handle.get_index() * GraphicsStepData::get_size() +
                offsetof(GraphicsStepData, resources)]) =
              *(Util::Map<RenderFlow, ResourceRegistry> *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(config);
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, config);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&GraphicsStep::ms_Buffer
              [p_Handle.get_index() * GraphicsStepData::get_size() +
               offsetof(GraphicsStepData, config)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(GraphicsStepConfig *)&GraphicsStep::ms_Buffer
               [p_Handle.get_index() * GraphicsStepData::get_size() +
                offsetof(GraphicsStepData, config)]) =
              *(GraphicsStepConfig *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pipelines);
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, pipelines);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&GraphicsStep::ms_Buffer
              [p_Handle.get_index() * GraphicsStepData::get_size() +
               offsetof(GraphicsStepData, pipelines)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>>
                 *)&GraphicsStep::ms_Buffer[p_Handle.get_index() *
                                                GraphicsStepData::get_size() +
                                            offsetof(GraphicsStepData,
                                                     pipelines)]) =
              *(Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>>
                    *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(renderobjects);
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, renderobjects);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&GraphicsStep::ms_Buffer
              [p_Handle.get_index() * GraphicsStepData::get_size() +
               offsetof(GraphicsStepData, renderobjects)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(Util::Map<Util::Name, Util::Map<Mesh, Util::List<RenderObject>>>
                 *)&GraphicsStep::ms_Buffer[p_Handle.get_index() *
                                                GraphicsStepData::get_size() +
                                            offsetof(GraphicsStepData,
                                                     renderobjects)]) =
              *(Util::Map<Util::Name, Util::Map<Mesh, Util::List<RenderObject>>>
                    *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(renderpasses);
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, renderpasses);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&GraphicsStep::ms_Buffer
              [p_Handle.get_index() * GraphicsStepData::get_size() +
               offsetof(GraphicsStepData, renderpasses)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(Util::Map<RenderFlow, Interface::Renderpass> *)&GraphicsStep::
               ms_Buffer[p_Handle.get_index() * GraphicsStepData::get_size() +
                         offsetof(GraphicsStepData, renderpasses)]) =
              *(Util::Map<RenderFlow, Interface::Renderpass> *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, context);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&GraphicsStep::ms_Buffer
              [p_Handle.get_index() * GraphicsStepData::get_size() +
               offsetof(GraphicsStepData, context)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(Interface::Context *)&GraphicsStep::ms_Buffer
               [p_Handle.get_index() * GraphicsStepData::get_size() +
                offsetof(GraphicsStepData, context)]) =
              *(Interface::Context *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(signatures);
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, signatures);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&GraphicsStep::ms_Buffer
              [p_Handle.get_index() * GraphicsStepData::get_size() +
               offsetof(GraphicsStepData, signatures)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(Util::Map<RenderFlow, Interface::PipelineResourceSignature>
                 *)&GraphicsStep::ms_Buffer[p_Handle.get_index() *
                                                GraphicsStepData::get_size() +
                                            offsetof(GraphicsStepData,
                                                     signatures)]) =
              *(Util::Map<RenderFlow, Interface::PipelineResourceSignature> *)
                  p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(object_matrix_buffers);
        l_PropertyInfo.dataOffset =
            offsetof(GraphicsStepData, object_matrix_buffers);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&GraphicsStep::ms_Buffer
              [p_Handle.get_index() * GraphicsStepData::get_size() +
               offsetof(GraphicsStepData, object_matrix_buffers)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(Util::Map<RenderFlow, Resource::Buffer> *)&GraphicsStep::ms_Buffer
               [p_Handle.get_index() * GraphicsStepData::get_size() +
                offsetof(GraphicsStepData, object_matrix_buffers)]) =
              *(Util::Map<RenderFlow, Resource::Buffer> *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.dataOffset = offsetof(GraphicsStepData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (
              void *)&GraphicsStep::ms_Buffer[p_Handle.get_index() *
                                                  GraphicsStepData::get_size() +
                                              offsetof(GraphicsStepData, name)];
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          (*(Low::Util::Name *)&GraphicsStep::ms_Buffer
               [p_Handle.get_index() * GraphicsStepData::get_size() +
                offsetof(GraphicsStepData, name)]) = *(Low::Util::Name *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
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
        l_Params.bufferSize = sizeof(RenderObjectShaderInfo) * 32u;
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
          l_ResourceDescription.step = Backend::ResourcePipelineStep::GRAPHICS;
          l_ResourceDescription.type = Backend::ResourceType::BUFFER;

          l_ResourceDescriptions.push_back(l_ResourceDescription);
        }
        get_signatures()[p_RenderFlow] =
            Interface::PipelineResourceSignature::make(N(StepResourceSignature),
                                                       get_context(), 2,
                                                       l_ResourceDescriptions);

        get_signatures()[p_RenderFlow].set_buffer_resource(
            N(u_RenderObjects), 0, get_object_matrix_buffers()[p_RenderFlow]);
      }

      create_renderpass(p_RenderFlow);
      create_pipelines(p_RenderFlow, false);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare
    }

    void GraphicsStep::execute(RenderFlow p_RenderFlow,
                               Math::Matrix4x4 &p_ProjectionMatrix,
                               Math::Matrix4x4 &p_ViewMatrix)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
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

      get_renderpasses()[p_RenderFlow].begin();

      RenderObjectShaderInfo l_ObjectShaderInfos[32];
      uint32_t l_ObjectIndex = 0;

      for (auto pit = get_pipelines()[p_RenderFlow].begin();
           pit != get_pipelines()[p_RenderFlow].end(); ++pit) {
        Interface::GraphicsPipeline i_Pipeline = *pit;
        i_Pipeline.bind();

        auto &renderobjects = get_renderobjects()[pit->get_name()];

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

            l_ObjectShaderInfos[l_ObjectIndex].mvp = l_MVPMatrix;
            l_ObjectShaderInfos[l_ObjectIndex].model_matrix = l_ModelMatrix;

            l_ObjectShaderInfos[l_ObjectIndex].material_index =
                i_RenderObject.get_material().get_index();

            l_ObjectIndex++;

            ++it;
          }
        }
      }

      get_object_matrix_buffers()[p_RenderFlow].set(
          (void *)l_ObjectShaderInfos);

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

      if (get_context().is_debug_enabled()) {
        LOW_RENDERER_END_RENDERDOC_SECTION(get_context().get_context());
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

    void GraphicsStep::register_renderobject(RenderObject p_RenderObject)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_register_renderobject

      MaterialType l_MaterialType =
          p_RenderObject.get_material().get_material_type();

      for (uint32_t i = 0u; i < get_config().get_pipelines().size(); ++i) {
        GraphicsPipelineConfig &i_Config = get_config().get_pipelines()[i];
        if (i_Config.name == l_MaterialType.get_gbuffer_pipeline().name ||
            i_Config.name == l_MaterialType.get_depth_pipeline().name) {
          get_renderobjects()[i_Config.name][p_RenderObject.get_mesh()]
              .push_back(p_RenderObject);
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
      create_renderpass(p_RenderFlow);
      create_pipelines(p_RenderFlow, true);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_dimensions
    }

    void GraphicsStep::create_renderpass(RenderFlow p_RenderFlow)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_create_renderpass
      Interface::RenderpassCreateParams l_Params;
      l_Params.context = get_context();
      l_Params.dimensions = p_RenderFlow.get_dimensions();
      l_Params.useDepth = get_config().is_use_depth();
      if (get_config().is_depth_clear()) {
        l_Params.clearDepthColor = {1.0f, 1.0f};
      } else {
        l_Params.clearDepthColor = {1.0f, 0.0f};
      }

      for (uint8_t i = 0u; i < get_config().get_rendertargets().size(); ++i) {
        l_Params.clearColors.push_back({0.0f, 0.0f, 0.0f, 1.0f});

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

      if (get_config().is_use_depth()) {
        Resource::Image l_Image =
            p_RenderFlow.get_resources().get_image_resource(
                get_config().get_depth_rendertarget().resourceName);
        LOW_ASSERT(l_Image.is_alive(),
                   "Could not find rendertarget image resource");
        l_Params.depthRenderTarget = l_Image;
      }

      get_renderpasses()[p_RenderFlow] =
          Interface::Renderpass::make(get_name(), l_Params);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_create_renderpass
    }

    void GraphicsStep::create_pipelines(RenderFlow p_RenderFlow,
                                        bool p_UpdateExisting)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_create_pipelines
      for (uint32_t i = 0u; i < get_config().get_pipelines().size(); ++i) {
        GraphicsPipelineConfig &i_Config = get_config().get_pipelines()[i];
        Interface::PipelineGraphicsCreateParams i_Params;
        i_Params.context = get_context();
        i_Params.cullMode = i_Config.cullMode;
        i_Params.polygonMode = i_Config.polygonMode;
        i_Params.frontFace = i_Config.frontFace;
        i_Params.dimensions = p_RenderFlow.get_dimensions();
        i_Params.signatures = {get_context().get_global_signature(),
                               p_RenderFlow.get_resource_signature(),
                               get_signatures()[p_RenderFlow]};
        i_Params.vertexShaderPath = i_Config.vertexPath;
        i_Params.fragmentShaderPath = i_Config.fragmentPath;
        i_Params.renderpass = get_renderpasses()[p_RenderFlow];
        i_Params.depthTest = get_config().is_depth_test();
        i_Params.depthWrite = get_config().is_depth_write();
        i_Params.depthCompareOperation =
            get_config().get_depth_compare_operation();
        i_Params.vertexDataAttributeTypes = {
            Backend::VertexAttributeType::VECTOR3,
            Backend::VertexAttributeType::VECTOR2,
            Backend::VertexAttributeType::VECTOR3,
            Backend::VertexAttributeType::VECTOR3,
            Backend::VertexAttributeType::VECTOR3};
        for (uint8_t i = 0u; i < get_config().get_rendertargets().size(); ++i) {
          Backend::GraphicsPipelineColorTarget i_ColorTarget;

          if (get_config().get_rendertargets()[i].resourceScope ==
              ResourceBindScope::RENDERFLOW) {
            Resource::Image i_Image =
                p_RenderFlow.get_resources().get_image_resource(
                    get_config().get_rendertargets()[i].resourceName);
            LOW_ASSERT(i_Image.is_alive(),
                       "Could not find rendertarget image resource");

            i_ColorTarget.wirteMask =
                Backend::imageformat_get_pipeline_write_mask(
                    i_Image.get_image().format);
          } else {
            LOW_ASSERT(false, "Unsupported rendertarget resource scope");
          }

          i_ColorTarget.blendEnable = true;
          i_Params.colorTargets.push_back(i_ColorTarget);
        }

        if (p_UpdateExisting) {
          Interface::PipelineManager::register_graphics_pipeline(
              get_pipelines()[p_RenderFlow][i], i_Params);
        } else {
          get_pipelines()[p_RenderFlow].push_back(
              Interface::GraphicsPipeline::make(i_Config.name, i_Params));
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_create_pipelines
    }

  } // namespace Renderer
} // namespace Low
