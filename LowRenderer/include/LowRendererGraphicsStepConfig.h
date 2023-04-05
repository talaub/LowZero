#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererFrontendConfig.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct LOW_RENDERER_API GraphicsStep;

    struct GraphicsStepCallbacks
    {
      void (*setup_signature)(GraphicsStep, RenderFlow);
      void (*setup_renderpass)(GraphicsStep, RenderFlow);
      void (*setup_pipelines)(GraphicsStep, RenderFlow, bool);
      void (*execute)(GraphicsStep, RenderFlow, Math::Matrix4x4 &,
                      Math::Matrix4x4 &);
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API GraphicsStepConfigData
    {
      GraphicsStepCallbacks callbacks;
      Util::List<ResourceConfig> resources;
      DimensionsConfig dimensions_config;
      Util::List<GraphicsPipelineConfig> pipelines;
      Util::List<PipelineResourceBindingConfig> rendertargets;
      PipelineResourceBindingConfig depth_rendertarget;
      bool use_depth;
      bool depth_clear;
      bool depth_test;
      bool depth_write;
      uint8_t depth_compare_operation;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(GraphicsStepConfigData);
      }
    };

    struct LOW_RENDERER_API GraphicsStepConfig : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<GraphicsStepConfig> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      GraphicsStepConfig();
      GraphicsStepConfig(uint64_t p_Id);
      GraphicsStepConfig(GraphicsStepConfig &p_Copy);

    private:
      static GraphicsStepConfig make(Low::Util::Name p_Name);

    public:
      explicit GraphicsStepConfig(const GraphicsStepConfig &p_Copy)
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
      static GraphicsStepConfig *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      GraphicsStepCallbacks &get_callbacks() const;
      void set_callbacks(GraphicsStepCallbacks &p_Value);

      Util::List<ResourceConfig> &get_resources() const;

      DimensionsConfig &get_dimensions_config() const;

      Util::List<GraphicsPipelineConfig> &get_pipelines() const;

      Util::List<PipelineResourceBindingConfig> &get_rendertargets() const;

      PipelineResourceBindingConfig &get_depth_rendertarget() const;

      bool is_use_depth() const;

      bool is_depth_clear() const;

      bool is_depth_test() const;

      bool is_depth_write() const;

      uint8_t get_depth_compare_operation() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static GraphicsStepConfig make(Util::Name p_Name,
                                     Util::Yaml::Node &p_Node);

    private:
      void set_dimensions_config(DimensionsConfig &p_Value);
      void set_depth_rendertarget(PipelineResourceBindingConfig &p_Value);
      void set_use_depth(bool p_Value);
      void set_depth_clear(bool p_Value);
      void set_depth_test(bool p_Value);
      void set_depth_write(bool p_Value);
      void set_depth_compare_operation(uint8_t p_Value);
    };
  } // namespace Renderer
} // namespace Low
