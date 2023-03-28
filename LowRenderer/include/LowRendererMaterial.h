#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererMaterialType.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API MaterialData
    {
      MaterialType material_type;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MaterialData);
      }
    };

    struct LOW_RENDERER_API Material : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Material> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Material();
      Material(uint64_t p_Id);
      Material(Material &p_Copy);

      static Material make(Low::Util::Name p_Name);
      explicit Material(const Material &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static Material *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      MaterialType get_material_type() const;
      void set_material_type(MaterialType p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);
    };
  } // namespace Renderer
} // namespace Low
