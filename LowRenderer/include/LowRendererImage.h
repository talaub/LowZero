#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Resource {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT ImageData
      {
        Backend::ImageResource image;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(ImageData);
        }
      };

      struct LOW_EXPORT Image : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Image> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Image();
        Image(uint64_t p_Id);
        Image(Image &p_Copy);

      private:
        static Image make(Low::Util::Name p_Name);

      public:
        explicit Image(const Image &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Image *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Backend::ImageResource &get_image() const;
        void set_image(Backend::ImageResource &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Image make(Util::Name p_Name,
                          Backend::ImageResourceCreateParams &p_Params);
      };
    } // namespace Resource
  }   // namespace Renderer
} // namespace Low
