#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererInterface.h"
#include "LowRendererResourceRegistry.h"
#include "LowRendererGraphicsStepConfig.h"
#include "LowRendererGraphicsPipeline.h"
#include "LowRendererRenderFlow.h"
#include "LowRendererBuffer.h"
#include "LowRendererExposedObjects.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    struct RenderObjectShaderInfo
    {
      alignas(16) Math::Matrix4x4 mvp;
      alignas(16) Math::Matrix4x4 model_matrix;
      u32 material_index;
      u32 entity_id;
      u32 texture_index;
      u32 click_passthrough;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API GraphicsStep : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Util::Map<RenderFlow, ResourceRegistry> resources;
        GraphicsStepConfig config;
        Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>>
            pipelines;
        Util::Map<Util::Name,
                  Util::Map<Mesh, Util::List<RenderObject>>>
            renderobjects;
        Util::Map<Util::Name, Util::List<RenderObject>>
            skinned_renderobjects;
        Util::Map<RenderFlow, Interface::Renderpass> renderpasses;
        Interface::Context context;
        Util::Map<RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>>
            pipeline_signatures;
        Util::Map<RenderFlow, Interface::PipelineResourceSignature>
            signatures;
        Resource::Image output_image;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(Data);
        }
      };

    public:
      static Low::Util::UniqueLock<Low::Util::SharedMutex>
          ms_PagesLock;
      static Low::Util::SharedMutex ms_PagesMutex;
      static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

      static Low::Util::List<GraphicsStep> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      GraphicsStep();
      GraphicsStep(uint64_t p_Id);
      GraphicsStep(GraphicsStep &p_Copy);

    private:
      static GraphicsStep make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit GraphicsStep(const GraphicsStep &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static GraphicsStep *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static GraphicsStep create_handle_by_index(u32 p_Index);

      static GraphicsStep find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Function<void(Low::Util::Handle,
                                           Low::Util::Name)>
                      p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      GraphicsStep duplicate(Low::Util::Name p_Name) const;
      static GraphicsStep duplicate(GraphicsStep p_Handle,
                                    Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static GraphicsStep find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        GraphicsStep l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        GraphicsStep l_GraphicsStep = p_Handle.get_id();
        l_GraphicsStep.destroy();
      }

      Util::Map<RenderFlow, ResourceRegistry> &get_resources() const;

      GraphicsStepConfig get_config() const;

      Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>> &
      get_pipelines() const;

      Util::Map<Util::Name,
                Util::Map<Mesh, Util::List<RenderObject>>> &
      get_renderobjects() const;

      Util::Map<Util::Name, Util::List<RenderObject>> &
      get_skinned_renderobjects() const;

      Util::Map<RenderFlow, Interface::Renderpass> &
      get_renderpasses() const;

      Interface::Context get_context() const;

      Util::Map<RenderFlow,
                Util::List<Interface::PipelineResourceSignature>> &
      get_pipeline_signatures() const;
      void set_pipeline_signatures(
          Util::Map<RenderFlow,
                    Util::List<Interface::PipelineResourceSignature>>
              &p_Value);

      Util::Map<RenderFlow, Interface::PipelineResourceSignature> &
      get_signatures() const;

      Resource::Image get_output_image() const;
      void set_output_image(Resource::Image p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static GraphicsStep make(Util::Name p_Name,
                               Interface::Context p_Context,
                               GraphicsStepConfig p_Config);
      void clear_renderobjects();
      void prepare(RenderFlow p_RenderFlow);
      void execute(RenderFlow p_RenderFlow,
                   Math::Matrix4x4 &p_ProjectionMatrix,
                   Math::Matrix4x4 &p_ViewMatrix);
      void register_renderobject(RenderObject &p_RenderObject);
      void update_dimensions(RenderFlow p_RenderFlow);
      static void create_signature(GraphicsStep p_Step,
                                   RenderFlow p_RenderFlow);
      static void create_renderpass(GraphicsStep p_Step,
                                    RenderFlow p_RenderFlow);
      static void create_pipelines(GraphicsStep p_Step,
                                   RenderFlow p_RenderFlow,
                                   bool p_UpdateExisting);
      static void default_execute(GraphicsStep p_Step,
                                  RenderFlow p_RenderFlow,
                                  Math::Matrix4x4 &p_ProjectionMatrix,
                                  Math::Matrix4x4 &p_ViewMatrix);
      static void default_execute_fullscreen_triangle(
          GraphicsStep p_Step, RenderFlow p_RenderFlow,
          Math::Matrix4x4 &p_ProjectionMatrix,
          Math::Matrix4x4 &p_ViewMatrix);
      static void draw_renderobjects(GraphicsStep p_Step,
                                     RenderFlow p_RenderFlow);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);
      static u32 create_page();
      void set_config(GraphicsStepConfig p_Value);
      void set_context(Interface::Context p_Value);
      static void fill_pipeline_signatures(GraphicsStep p_Step,
                                           RenderFlow p_RenderFlow);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
