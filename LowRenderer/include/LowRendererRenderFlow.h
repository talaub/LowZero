#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererInterface.h"
#include "LowRendererResourceRegistry.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_EXPORT RenderFlowData
    {
      Math::UVector2 dimensions;
      Util::List<Util::Handle> steps;
      ResourceRegistry resources;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(RenderFlowData);
      }
    };

    struct LOW_EXPORT RenderFlow : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<RenderFlow> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      RenderFlow();
      RenderFlow(uint64_t p_Id);
      RenderFlow(RenderFlow &p_Copy);

    private:
      static RenderFlow make(Low::Util::Name p_Name);

    public:
      explicit RenderFlow(const RenderFlow &p_Copy)
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
      static RenderFlow *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      Math::UVector2 &get_dimensions() const;

      Util::List<Util::Handle> &get_steps() const;
      void set_steps(Util::List<Util::Handle> &p_Value);

      ResourceRegistry &get_resources() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static RenderFlow make(Util::Name p_Name, Interface::Context p_Context,
                             Util::Yaml::Node &p_Config);
      void execute();
    };
  } // namespace Renderer
} // namespace Low