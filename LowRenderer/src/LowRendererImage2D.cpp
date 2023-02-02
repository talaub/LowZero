#include "LowRendererImage2D.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

#include "LowRendererInterface.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t Image2D::TYPE_ID = 7;
      uint8_t *Image2D::ms_Buffer = 0;
      Low::Util::Instances::Slot *Image2D::ms_Slots = 0;
      Low::Util::List<Image2D> Image2D::ms_LivingInstances =
          Low::Util::List<Image2D>();

      Image2D::Image2D() : Low::Util::Handle(0ull)
      {
      }
      Image2D::Image2D(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Image2D::Image2D(Image2D &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Image2D Image2D::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        Image2D l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Image2D::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, Image2D, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        return l_Handle;
      }

      void Image2D::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        LOW_ASSERT_WARN(false, "Now backend logic to clean up image2d");
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Image2D *l_Instances = living_instances();
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

      void Image2D::cleanup()
      {
        Image2D *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);
      }

      bool Image2D::is_alive() const
      {
        return m_Data.m_Type == Image2D::TYPE_ID &&
               check_alive(ms_Slots, Image2D::get_capacity());
      }

      uint32_t Image2D::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity = Low::Util::Config::get_capacity(N(Image2D));
        }
        return l_Capacity;
      }

      Low::Renderer::Backend::Image2D &Image2D::get_image2d() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Image2D, image2d, Low::Renderer::Backend::Image2D);
      }

      Low::Util::Name Image2D::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Image2D, name, Low::Util::Name);
      }
      void Image2D::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Image2D, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Image2D Image2D::make(Util::Name p_Name, Image2DCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Image2D l_Image = Image2D::make(p_Name);

        Backend::Image2DCreateParams l_Params;
        l_Params.context = &(p_Params.context.get_context());
        l_Params.format = &(p_Params.format);
        l_Params.writable = p_Params.writeable;
        l_Params.create_image = p_Params.create_image;
        l_Params.depth = p_Params.depth;

        LOW_ASSERT(false, "Currently not supported");

        return l_Image;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
