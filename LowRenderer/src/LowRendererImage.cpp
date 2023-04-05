#include "LowRendererImage.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    namespace Resource {
      const uint16_t Image::TYPE_ID = 18;
      uint8_t *Image::ms_Buffer = 0;
      Low::Util::Instances::Slot *Image::ms_Slots = 0;
      Low::Util::List<Image> Image::ms_LivingInstances =
          Low::Util::List<Image>();

      Image::Image() : Low::Util::Handle(0ull)
      {
      }
      Image::Image(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Image::Image(Image &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Image Image::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        Image l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Image::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, Image, image, Backend::ImageResource))
            Backend::ImageResource();
        ACCESSOR_TYPE_SOA(l_Handle, Image, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void Image::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::callbacks().imageresource_cleanup(get_image());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Image *l_Instances = living_instances();
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

      void Image::initialize()
      {
        initialize_buffer(&ms_Buffer, ImageData::get_size(), get_capacity(),
                          &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Image);
        LOW_PROFILE_ALLOC(type_slots_Image);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Image);
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Image::is_alive;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(image);
          l_PropertyInfo.dataOffset = offsetof(ImageData, image);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Image, image,
                                              Backend::ImageResource);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Image, image, Backend::ImageResource) =
                *(Backend::ImageResource *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.dataOffset = offsetof(ImageData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Image, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ACCESSOR_TYPE_SOA(p_Handle, Image, name, Low::Util::Name) =
                *(Low::Util::Name *)p_Data;
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Image::cleanup()
      {
        Low::Util::List<Image> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Image);
        LOW_PROFILE_FREE(type_slots_Image);
      }

      bool Image::is_alive() const
      {
        return m_Data.m_Type == Image::TYPE_ID &&
               check_alive(ms_Slots, Image::get_capacity());
      }

      uint32_t Image::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(Image));
        }
        return l_Capacity;
      }

      Backend::ImageResource &Image::get_image() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Image, image, Backend::ImageResource);
      }
      void Image::set_image(Backend::ImageResource &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Image, image, Backend::ImageResource) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_image
        // LOW_CODEGEN::END::CUSTOM:SETTER_image
      }

      Low::Util::Name Image::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(Image, name, Low::Util::Name);
      }
      void Image::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(Image, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      Image Image::make(Util::Name p_Name,
                        Backend::ImageResourceCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        Image l_Image = Image::make(p_Name);

        Backend::callbacks().imageresource_create(l_Image.get_image(),
                                                  p_Params);

        l_Image.get_image().handleId = l_Image.get_id();

        return l_Image;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void Image::reinitialize(Backend::ImageResourceCreateParams &p_Params)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_reinitialize
        Backend::callbacks().imageresource_cleanup(get_image());

        Backend::callbacks().imageresource_create(get_image(), p_Params);

        get_image().handleId = get_id();

        // LOW_CODEGEN::END::CUSTOM:FUNCTION_reinitialize
      }

    } // namespace Resource
  }   // namespace Renderer
} // namespace Low
