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
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      struct CommandBuffer;
      struct Image2DCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT Image2DData
      {
        Low::Renderer::Backend::Image2D image2d;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(Image2DData);
        }
      };

      struct LOW_EXPORT Image2D : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Image2D> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Image2D();
        Image2D(uint64_t p_Id);
        Image2D(Image2D &p_Copy);

        static Image2D make(Low::Util::Name p_Name);
        explicit Image2D(const Image2D &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Image2D *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Low::Renderer::Backend::Image2D &get_image2d() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Image2D make(Util::Name p_Name, Image2DCreateParams &p_Params);
        void transition_state(CommandBuffer p_CommandBuffer,
                              uint8_t p_DstState);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
