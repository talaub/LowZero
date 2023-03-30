#include "LowRendererImGuiImage.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      const uint16_t ImGuiImage::TYPE_ID = 16;
      uint8_t *ImGuiImage::ms_Buffer = 0;
      Low::Util::Instances::Slot *ImGuiImage::ms_Slots = 0;
      Low::Util::List<ImGuiImage> ImGuiImage::ms_LivingInstances =
          Low::Util::List<ImGuiImage>();

      ImGuiImage::ImGuiImage() : Low::Util::Handle(0ull)
      {
      }
      ImGuiImage::ImGuiImage(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      ImGuiImage::ImGuiImage(ImGuiImage &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      ImGuiImage ImGuiImage::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = Low::Util::Instances::create_instance(
            ms_Buffer, ms_Slots, get_capacity());

        ImGuiImageData *l_DataPtr =
            (ImGuiImageData *)&ms_Buffer[l_Index * sizeof(ImGuiImageData)];
        new (l_DataPtr) ImGuiImageData();

        ImGuiImage l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = ImGuiImage::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, ImGuiImage, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        return l_Handle;
      }

      void ImGuiImage::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        Backend::callbacks().imgui_image_cleanup(get_imgui_image());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const ImGuiImage *l_Instances = living_instances();
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

      void ImGuiImage::initialize()
      {
        initialize_buffer(&ms_Buffer, ImGuiImageData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_ImGuiImage);
        LOW_PROFILE_ALLOC(type_slots_ImGuiImage);
      }

      void ImGuiImage::cleanup()
      {
        Low::Util::List<ImGuiImage> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_ImGuiImage);
        LOW_PROFILE_FREE(type_slots_ImGuiImage);
      }

      bool ImGuiImage::is_alive() const
      {
        return m_Data.m_Type == ImGuiImage::TYPE_ID &&
               check_alive(ms_Slots, ImGuiImage::get_capacity());
      }

      uint32_t ImGuiImage::get_capacity()
      {
        static uint32_t l_Capacity = 0u;
        if (l_Capacity == 0u) {
          l_Capacity =
              Low::Util::Config::get_capacity(N(LowRenderer), N(ImGuiImage));
        }
        return l_Capacity;
      }

      Backend::ImGuiImage &ImGuiImage::get_imgui_image() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(ImGuiImage, imgui_image, Backend::ImGuiImage);
      }

      Resource::Image ImGuiImage::get_image() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(ImGuiImage, image, Resource::Image);
      }
      void ImGuiImage::set_image(Resource::Image p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(ImGuiImage, image, Resource::Image) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_image
        // LOW_CODEGEN::END::CUSTOM:SETTER_image
      }

      Low::Util::Name ImGuiImage::get_name() const
      {
        _LOW_ASSERT(is_alive());
        return TYPE_SOA(ImGuiImage, name, Low::Util::Name);
      }
      void ImGuiImage::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // Set new value
        TYPE_SOA(ImGuiImage, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      ImGuiImage ImGuiImage::make(Util::Name p_Name, Resource::Image p_Image)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
        ImGuiImage l_ImGuiImage = ImGuiImage::make(p_Name);

        l_ImGuiImage.set_image(p_Image);

        Backend::callbacks().imgui_image_create(l_ImGuiImage.get_imgui_image(),
                                                p_Image.get_image());

        return l_ImGuiImage;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void ImGuiImage::render(Math::UVector2 &p_Dimensions)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_render
        Backend::callbacks().imgui_image_render(get_imgui_image(),
                                                p_Dimensions);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_render
      }

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
