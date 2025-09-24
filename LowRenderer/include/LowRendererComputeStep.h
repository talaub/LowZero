#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererInterface.h"
#include "LowRendererResourceRegistry.h"
#include "LowRendererComputeStepConfig.h"
#include "LowRendererComputePipeline.h"
#include "LowRendererRenderFlow.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API ComputeStep : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Util::Map<RenderFlow, ResourceRegistry> resources;
        ComputeStepConfig config;
        Util::Map<RenderFlow, Util::List<Interface::ComputePipeline>>
            pipelines;
        Util::Map<RenderFlow,
                  Util::List<Interface::PipelineResourceSignature>>
            signatures;
        Interface::Context context;
        Resource::Image output_image;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(Data);
        }
      };

    public:
      static Low::Util::SharedMutex ms_LivingMutex;
      static Low::Util::UniqueLock<Low::Util::SharedMutex>
          ms_PagesLock;
      static Low::Util::SharedMutex ms_PagesMutex;
      static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

      static Low::Util::List<ComputeStep> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      ComputeStep();
      ComputeStep(uint64_t p_Id);
      ComputeStep(ComputeStep &p_Copy);

    private:
      static ComputeStep make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit ComputeStep(const ComputeStep &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static ComputeStep *living_instances()
      {
        Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        return ms_LivingInstances.data();
      }

      static ComputeStep create_handle_by_index(u32 p_Index);

      static ComputeStep find_by_index(uint32_t p_Index);
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

      ComputeStep duplicate(Low::Util::Name p_Name) const;
      static ComputeStep duplicate(ComputeStep p_Handle,
                                   Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static ComputeStep find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        ComputeStep l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        ComputeStep l_ComputeStep = p_Handle.get_id();
        l_ComputeStep.destroy();
      }

      Util::Map<RenderFlow, ResourceRegistry> &get_resources() const;

      ComputeStepConfig get_config() const;

      Util::Map<RenderFlow, Util::List<Interface::ComputePipeline>> &
      get_pipelines() const;

      Util::Map<RenderFlow,
                Util::List<Interface::PipelineResourceSignature>> &
      get_signatures() const;

      Interface::Context get_context() const;

      Resource::Image get_output_image() const;
      void set_output_image(Resource::Image p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static ComputeStep make(Util::Name p_Name,
                              Interface::Context p_Context,
                              ComputeStepConfig p_Config);
      void prepare(RenderFlow p_RenderFlow);
      void execute(RenderFlow p_RenderFlow);
      void update_dimensions(RenderFlow p_RenderFlow);
      static void create_pipelines(ComputeStep p_Step,
                                   RenderFlow p_RenderFlow);
      static void create_signatures(ComputeStep p_Step,
                                    RenderFlow p_RenderFlow);
      static void prepare_signatures(ComputeStep p_Step,
                                     RenderFlow p_RenderFlow);
      static void default_execute(ComputeStep p_Step,
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
      void set_config(ComputeStepConfig p_Value);
      void set_context(Interface::Context p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
