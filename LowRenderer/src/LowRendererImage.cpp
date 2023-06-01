#include "LowRendererImage.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

namespace Low {
  namespace Renderer {
    namespace Resource {
      const uint16_t Image::TYPE_ID = 7;
      uint32_t Image::ms_Capacity = 0u;
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

      Low::Util::Handle Image::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Image Image::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = create_instance();

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

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

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
        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer), N(Image));

        initialize_buffer(&ms_Buffer, ImageData::get_size(), get_capacity(),
                          &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Image);
        LOW_PROFILE_ALLOC(type_slots_Image);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Image);
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Image::is_alive;
        l_TypeInfo.destroy = &Image::destroy;
        l_TypeInfo.serialize = &Image::serialize;
        l_TypeInfo.deserialize = &Image::deserialize;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Image::_make;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Image::living_instances);
        l_TypeInfo.get_living_count = &Image::living_count;
        l_TypeInfo.component = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(image);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ImageData, image);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            Image l_Handle = p_Handle.get_id();
            l_Handle.get_image();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Image, image,
                                              Backend::ImageResource);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Image l_Handle = p_Handle.get_id();
            l_Handle.set_image(*(Backend::ImageResource *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ImageData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            Image l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Image, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Image l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
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

      Image Image::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Image l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = Image::TYPE_ID;

        return l_Handle;
      }

      bool Image::is_alive() const
      {
        return m_Data.m_Type == Image::TYPE_ID &&
               check_alive(ms_Slots, Image::get_capacity());
      }

      uint32_t Image::get_capacity()
      {
        return ms_Capacity;
      }

      Image Image::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
      }

      void Image::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Image::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node)
      {
        Image l_Image = p_Handle.get_id();
        l_Image.serialize(p_Node);
      }

      Low::Util::Handle Image::deserialize(Low::Util::Yaml::Node &p_Node,
                                           Low::Util::Handle p_Creator)
      {
        Image l_Handle = Image::make(N(Image));

        if (p_Node["image"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Backend::ImageResource &Image::get_image() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_image
        // LOW_CODEGEN::END::CUSTOM:GETTER_image

        return TYPE_SOA(Image, image, Backend::ImageResource);
      }
      void Image::set_image(Backend::ImageResource &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_image
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_image

        // Set new value
        TYPE_SOA(Image, image, Backend::ImageResource) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_image
        // LOW_CODEGEN::END::CUSTOM:SETTER_image
      }

      Low::Util::Name Image::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Image, name, Low::Util::Name);
      }
      void Image::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

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

      uint32_t Image::create_instance()
      {
        uint32_t l_Index = 0u;

        for (; l_Index < get_capacity(); ++l_Index) {
          if (!ms_Slots[l_Index].m_Occupied) {
            break;
          }
        }
        if (l_Index >= get_capacity()) {
          increase_budget();
        }
        ms_Slots[l_Index].m_Occupied = true;
        return l_Index;
      }

      void Image::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(ImageData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(ImageData, image) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ImageData, image) * (l_Capacity)],
                 l_Capacity * sizeof(Backend::ImageResource));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ImageData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ImageData, name) * (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::Name));
        }
        for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease;
             ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for Image from " << l_Capacity
                      << " to " << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }
    } // namespace Resource
  }   // namespace Renderer
} // namespace Low
