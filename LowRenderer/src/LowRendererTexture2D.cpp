#include "LowRendererTexture2D.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    const uint16_t Texture2D::TYPE_ID = 1;
    uint8_t *Texture2D::ms_Buffer = 0;
    Low::Util::Instances::Slot *Texture2D::ms_Slots = 0;
    Low::Util::List<Texture2D> Texture2D::ms_LivingInstances =
        Low::Util::List<Texture2D>();

    Texture2D::Texture2D() : Low::Util::Handle(0ull)
    {
    }
    Texture2D::Texture2D(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Texture2D::Texture2D(Texture2D &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Texture2D Texture2D::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      Texture2D l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Texture2D::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, Texture2D, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void Texture2D::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Texture2D *l_Instances = living_instances();
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

    void Texture2D::initialize()
    {
      initialize_buffer(&ms_Buffer, Texture2DData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Texture2D);
      LOW_PROFILE_ALLOC(type_slots_Texture2D);
    }

    void Texture2D::cleanup()
    {
      Low::Util::List<Texture2D> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Texture2D);
      LOW_PROFILE_FREE(type_slots_Texture2D);
    }

    bool Texture2D::is_alive() const
    {
      return m_Data.m_Type == Texture2D::TYPE_ID &&
             check_alive(ms_Slots, Texture2D::get_capacity());
    }

    uint32_t Texture2D::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(Texture2D));
      }
      return l_Capacity;
    }

    Interface::Image2D &Texture2D::get_image2d() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Texture2D, image2d, Interface::Image2D);
    }
    void Texture2D::set_image2d(Interface::Image2D &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Texture2D, image2d, Interface::Image2D) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_image2d
      // LOW_CODEGEN::END::CUSTOM:SETTER_image2d
    }

    Low::Util::Name Texture2D::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Texture2D, name, Low::Util::Name);
    }
    void Texture2D::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Texture2D, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

  } // namespace Renderer
} // namespace Low
