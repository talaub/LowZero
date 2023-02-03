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
      struct FramebufferCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT FramebufferData
      {
        Low::Renderer::Backend::Framebuffer framebuffer;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(FramebufferData);
        }
      };

      struct LOW_EXPORT Framebuffer : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Framebuffer> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Framebuffer();
        Framebuffer(uint64_t p_Id);
        Framebuffer(Framebuffer &p_Copy);

        static Framebuffer make(Low::Util::Name p_Name);
        explicit Framebuffer(const Framebuffer &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Framebuffer *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Low::Renderer::Backend::Framebuffer &get_framebuffer() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Framebuffer make(Util::Name p_Name,
                                FramebufferCreateParams &p_Params);
        void get_dimensions(Math::UVector2 &p_Dimensions);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
