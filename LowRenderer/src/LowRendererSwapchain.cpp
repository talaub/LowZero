#include "LowRendererSwapchain.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Swapchain::TYPE_ID = 2;
      uint8_t *Swapchain::ms_Buffer = 0;
      Low::Util::Instances::Slot *Swapchain::ms_Slots = 0;
      Low::Util::List<Swapchain> Swapchain::ms_LivingInstances =
          Low::Util::List<Swapchain>();

      Swapchain::Swapchain() : Low::Util::Handle(0ull)
      {
      }
      Swapchain::Swapchain(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Swapchain::Swapchain(Swapchain &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Swapchain Swapchain::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        Swapchain l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Swapchain::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, Swapchain, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void Swapchain::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::swapchain_cleanup(get_swapchain());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Swapchain *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
        _LOW_ASSERT(l_LivingInstanceFound);
      }

      void Swapchain::initialize()
      {
        initialize_buffer(&ms_Buffer, SwapchainData::get_size(), get_capacity(),
                          &ms_Slots);
      }

      void Swapchain::cleanup()
      {
        Low::Util::List<Swapchain> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool Swapchain::is_alive() const
      {
        return m_Data.m_Type == Swapchain::TYPE_ID &&
               check_alive(ms_Slots, Swapchain::get_capacity());
      }

      uint32_t Swapchain::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(N(Swapchain));
        }
        return l_Capacity;
      }

      Low::Renderer::Backend::Swapchain &Swapchain::get_swapchain() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Swapchain, swapchain,
                        Low::Renderer::Backend::Swapchain);
      }

      Util::List<CommandBuffer> &Swapchain::get_commandbuffers() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Swapchain, commandbuffers, Util::List<CommandBuffer>);
      }

      Util::List<Framebuffer> &Swapchain::get_framebuffers() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Swapchain, framebuffers, Util::List<Framebuffer>);
      }

      Renderpass &Swapchain::get_renderpass() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Swapchain, renderpass, Renderpass);
      }
      void Swapchain::set_renderpass(Renderpass &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Swapchain, renderpass, Renderpass) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderpass
        // LOW_CODEGEN::END::CUSTOM:SETTER_renderpass
      }

      Low::Util::Name Swapchain::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Swapchain, name, Low::Util::Name);
      }
      void Swapchain::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Swapchain, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Swapchain Swapchain::make(Util::Name p_Name,
                                SwapchainCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Swapchain l_Swapchain = Swapchain::make(p_Name);

        Backend::SwapchainCreateParams l_Params;
        l_Params.commandPool = &(p_Params.commandPool.get_commandpool());
        l_Params.context = &(p_Params.context.get_context());

        Backend::swapchain_create(l_Swapchain.get_swapchain(), l_Params);

        uint8_t l_FramesInFlight = Backend::swapchain_get_frames_in_flight(
            l_Swapchain.get_swapchain());
        uint8_t l_ImageCount =
            Backend::swapchain_get_image_count(l_Swapchain.get_swapchain());

        for (uint8_t i = 0; i < l_FramesInFlight; ++i) {
          Backend::CommandBuffer l_Cb = Backend::swapchain_get_commandbuffer(
              l_Swapchain.get_swapchain(), i);

          CommandBuffer l_CommandBuffer = CommandBuffer::make(p_Name);
          l_CommandBuffer.set_commandbuffer(l_Cb);

          l_Swapchain.get_commandbuffers().push_back(l_CommandBuffer);
        }

        for (uint8_t i = 0; i < l_ImageCount; ++i) {
          Backend::Framebuffer l_Fb = Backend::swapchain_get_framebuffer(
              l_Swapchain.get_swapchain(), i);

          Framebuffer l_Framebuffer = Framebuffer::make(p_Name);
          l_Framebuffer.set_framebuffer(l_Fb);

          l_Swapchain.get_framebuffers().push_back(l_Framebuffer);
        }

        l_Swapchain.set_renderpass(Renderpass::make(p_Name));
        l_Swapchain.get_renderpass().set_renderpass(
            l_Swapchain.get_swapchain().renderpass);

        return l_Swapchain;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      uint8_t Swapchain::prepare()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_prepare
        return Backend::swapchain_prepare(get_swapchain());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_prepare
      }

      void Swapchain::swap()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_swap
        Backend::swapchain_swap(get_swapchain());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_swap
      }

      CommandBuffer Swapchain::get_current_commandbuffer()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_current_commandbuffer
        return get_commandbuffers()[Backend::swapchain_get_current_frame_index(
            get_swapchain())];
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_current_commandbuffer
      }

      Framebuffer Swapchain::get_current_framebuffer()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_current_framebuffer
        return get_framebuffers()[Backend::swapchain_get_current_image_index(
            get_swapchain())];
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_current_framebuffer
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
