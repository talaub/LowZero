#include "LowRendererFramebuffer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Framebuffer::TYPE_ID = 5;
      uint8_t *Framebuffer::ms_Buffer = 0;
      Low::Util::Instances::Slot *Framebuffer::ms_Slots = 0;
      Low::Util::List<Framebuffer> Framebuffer::ms_LivingInstances =
          Low::Util::List<Framebuffer>();

      Framebuffer::Framebuffer() : Low::Util::Handle(0ull)
      {
      }
      Framebuffer::Framebuffer(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Framebuffer::Framebuffer(Framebuffer &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Framebuffer Framebuffer::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        Framebuffer l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Framebuffer::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, Framebuffer, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void Framebuffer::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::framebuffer_cleanup(get_framebuffer());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Framebuffer *l_Instances = living_instances();
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

      void Framebuffer::initialize()
      {
        initialize_buffer(&ms_Buffer, FramebufferData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Framebuffer);
        LOW_PROFILE_ALLOC(type_slots_Framebuffer);
      }

      void Framebuffer::cleanup()
      {
        Low::Util::List<Framebuffer> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Framebuffer);
        LOW_PROFILE_FREE(type_slots_Framebuffer);
      }

      bool Framebuffer::is_alive() const
      {
        return m_Data.m_Type == Framebuffer::TYPE_ID &&
               check_alive(ms_Slots, Framebuffer::get_capacity());
      }

      uint32_t Framebuffer::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(Framebuffer));
        }
        return l_Capacity;
      }

      Low::Renderer::Backend::Framebuffer &Framebuffer::get_framebuffer() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Framebuffer, framebuffer,
                        Low::Renderer::Backend::Framebuffer);
      }
      void
      Framebuffer::set_framebuffer(Low::Renderer::Backend::Framebuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Framebuffer, framebuffer,
                 Low::Renderer::Backend::Framebuffer) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_framebuffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_framebuffer
      }

      Low::Util::Name Framebuffer::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Framebuffer, name, Low::Util::Name);
      }
      void Framebuffer::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Framebuffer, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Framebuffer Framebuffer::make(Util::Name p_Name,
                                    FramebufferCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Framebuffer l_Framebuffer = Framebuffer::make(p_Name);

        Backend::FramebufferCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.renderpass = &(p_Params.renderpass.get_renderpass());
        l_Params.dimensions = p_Params.dimensions;
        l_Params.framesInFlight = p_Params.framesInFlight;
        l_Params.renderTargetCount =
            static_cast<uint8_t>(p_Params.renderTargets.size());
        l_Params.renderTargets = nullptr;

        Util::List<Backend::Image2D> l_Targets;
        if (!p_Params.renderTargets.empty()) {
          for (uint32_t i = 0u; i < p_Params.renderTargets.size(); ++i) {
            l_Targets.push_back(p_Params.renderTargets[i].get_image2d());
          }
          l_Params.renderTargets = l_Targets.data();
        }

        Backend::framebuffer_create(l_Framebuffer.get_framebuffer(), l_Params);

        return l_Framebuffer;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void Framebuffer::get_dimensions(Math::UVector2 &p_Dimensions)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_dimensions
        Backend::framebuffer_get_dimensions(get_framebuffer(), p_Dimensions);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_dimensions
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
