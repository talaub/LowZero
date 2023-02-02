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

        return l_Handle;
      }

      void Swapchain::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
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

      void Swapchain::cleanup()
      {
        Swapchain *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
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

      Low::Renderer::Backend::Swapchain &Swapchain::get_pipeline() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Swapchain, pipeline, Low::Renderer::Backend::Swapchain);
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

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
