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
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API MaterialTypeData
    {
      GraphicsPipelineConfig gbuffer_pipeline;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MaterialTypeData);
      }
    };

    struct LOW_RENDERER_API MaterialType : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<MaterialType> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      MaterialType();
      MaterialType(uint64_t p_Id);
      MaterialType(MaterialType &p_Copy);

      static MaterialType make(Low::Util::Name p_Name);
      explicit MaterialType(const MaterialType &p_Copy)
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
      static MaterialType *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      GraphicsPipelineConfig &get_gbuffer_pipeline() const;
      void set_gbuffer_pipeline(GraphicsPipelineConfig &p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);
    };
  } // namespace Renderer
} // namespace Low
