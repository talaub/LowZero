#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererImage2D.h"
#include "LowRendererUniform.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_EXPORT Texture2DData
    {
      Interface::Image2D image2d;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(Texture2DData);
      }
    };

    struct LOW_EXPORT Texture2D : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Texture2D> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Texture2D();
      Texture2D(uint64_t p_Id);
      Texture2D(Texture2D &p_Copy);

      static Texture2D make(Low::Util::Name p_Name);
      explicit Texture2D(const Texture2D &p_Copy)
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
      static Texture2D *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      Interface::Image2D &get_image2d() const;
      void set_image2d(Interface::Image2D &p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);
    };
  } // namespace Renderer
} // namespace Low
