#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererImage.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_EXPORT Texture2DData
    {
      Resource::Image image;
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

    private:
      static Texture2D make(Low::Util::Name p_Name);

    public:
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

      Resource::Image &get_image() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Texture2D make(Util::Name p_Name,
                            Backend::ImageResourceCreateParams &p_Params);

    private:
      void set_image(Resource::Image &p_Value);
    };
  } // namespace Renderer
} // namespace Low