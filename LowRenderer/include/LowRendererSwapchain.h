#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"
#include "LowRendererCommandBuffer.h"
#include "LowRendererFramebuffer.h"
#include "LowRendererRenderpass.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      struct SwapchainCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT SwapchainData
      {
        Low::Renderer::Backend::Swapchain swapchain;
        Util::List<CommandBuffer> commandbuffers;
        Util::List<Framebuffer> framebuffers;
        Renderpass renderpass;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(SwapchainData);
        }
      };

      struct LOW_EXPORT Swapchain : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Swapchain> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Swapchain();
        Swapchain(uint64_t p_Id);
        Swapchain(Swapchain &p_Copy);

      private:
        static Swapchain make(Low::Util::Name p_Name);

      public:
        explicit Swapchain(const Swapchain &p_Copy)
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
        static Swapchain *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Low::Renderer::Backend::Swapchain &get_swapchain() const;

        Util::List<CommandBuffer> &get_commandbuffers() const;

        Util::List<Framebuffer> &get_framebuffers() const;

        Renderpass &get_renderpass() const;
        void set_renderpass(Renderpass &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Swapchain make(Util::Name p_Name,
                              SwapchainCreateParams &p_Params);
        uint8_t prepare();
        void swap();
        CommandBuffer get_current_commandbuffer();
        Framebuffer get_current_framebuffer();
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low