#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererInterface.h"
#include "LowRendererResourceRegistry.h"
#include "LowRendererGraphicsStepConfig.h"
#include "LowRendererGraphicsPipeline.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderFlow.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_EXPORT GraphicsStepData
    {
      Util::Map<RenderFlow, ResourceRegistry> resources;
      GraphicsStepConfig config;
      Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>> pipelines;
      Util::Map<Util::Name, Util::List<RenderObject>> renderobjects;
      Util::Map<RenderFlow, Interface::Renderpass> renderpasses;
      Interface::Context context;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(GraphicsStepData);
      }
    };

    struct LOW_EXPORT GraphicsStep : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<GraphicsStep> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      GraphicsStep();
      GraphicsStep(uint64_t p_Id);
      GraphicsStep(GraphicsStep &p_Copy);

    private:
      static GraphicsStep make(Low::Util::Name p_Name);

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

      bool is_alive() const;

      static uint32_t get_capacity();

      Util::Map<RenderFlow, ResourceRegistry> &get_resources() const;

      GraphicsStepConfig get_config() const;

      Util::Map<RenderFlow, Util::List<Interface::GraphicsPipeline>> &
      get_pipelines() const;

      Util::Map<Util::Name, Util::List<RenderObject>> &
      get_renderobjects() const;

      Util::Map<RenderFlow, Interface::Renderpass> &get_renderpasses() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static GraphicsStep make(Util::Name p_Name, Interface::Context p_Context,
                               GraphicsStepConfig p_Config);
      void prepare(RenderFlow p_RenderFlow);
      void execute(RenderFlow p_RenderFlow);
      void register_renderobject(RenderObject p_RenderObject);

    private:
      void set_config(GraphicsStepConfig p_Value);
      Interface::Context get_context() const;
      void set_context(Interface::Context p_Value);
    };
  } // namespace Renderer
} // namespace Low
