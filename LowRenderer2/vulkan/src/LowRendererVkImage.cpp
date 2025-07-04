#include "LowRendererVkImage.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererVulkanImage.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Image::TYPE_ID = 49;
      uint32_t Image::ms_Capacity = 0u;
      uint8_t *Image::ms_Buffer = 0;
      std::shared_mutex Image::ms_BufferMutex;
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
        WRITE_LOCK(l_Lock);
        uint32_t l_Index = create_instance();

        Image l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Image::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, Image, allocated_image,
                                AllocatedImage)) AllocatedImage();
        ACCESSOR_TYPE_SOA(l_Handle, Image, name, Low::Util::Name) =
            Low::Util::Name(0u);
        LOCK_UNLOCK(l_Lock);

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
        // TODO: Schedule image for deletion
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        WRITE_LOCK(l_Lock);
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
      }

      void Image::initialize()
      {
        WRITE_LOCK(l_Lock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                      N(Image));

        initialize_buffer(&ms_Buffer, ImageData::get_size(),
                          get_capacity(), &ms_Slots);
        LOCK_UNLOCK(l_Lock);

        LOW_PROFILE_ALLOC(type_buffer_Image);
        LOW_PROFILE_ALLOC(type_slots_Image);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Image);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Image::is_alive;
        l_TypeInfo.destroy = &Image::destroy;
        l_TypeInfo.serialize = &Image::serialize;
        l_TypeInfo.deserialize = &Image::deserialize;
        l_TypeInfo.find_by_index = &Image::_find_by_index;
        l_TypeInfo.find_by_name = &Image::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Image::_make;
        l_TypeInfo.duplicate_default = &Image::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Image::living_instances);
        l_TypeInfo.get_living_count = &Image::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: allocated_image
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(allocated_image);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ImageData, allocated_image);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Image l_Handle = p_Handle.get_id();
            l_Handle.get_allocated_image();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Image, allocated_image, AllocatedImage);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Image l_Handle = p_Handle.get_id();
            l_Handle.set_allocated_image(*(AllocatedImage *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Image l_Handle = p_Handle.get_id();
            *((AllocatedImage *)p_Data) =
                l_Handle.get_allocated_image();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: allocated_image
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ImageData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
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
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Image l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: unload
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(unload);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: unload
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Image::cleanup()
      {
        Low::Util::List<Image> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        WRITE_LOCK(l_Lock);
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Image);
        LOW_PROFILE_FREE(type_slots_Image);
        LOCK_UNLOCK(l_Lock);
      }

      Low::Util::Handle Image::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
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
        READ_LOCK(l_Lock);
        return m_Data.m_Type == Image::TYPE_ID &&
               check_alive(ms_Slots, Image::get_capacity());
      }

      uint32_t Image::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Image::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Image Image::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return 0ull;
      }

      Image Image::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Image l_Handle = make(p_Name);
        l_Handle.set_allocated_image(get_allocated_image());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Image Image::duplicate(Image p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Image::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
      {
        Image l_Image = p_Handle.get_id();
        return l_Image.duplicate(p_Name);
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

      Low::Util::Handle
      Image::deserialize(Low::Util::Yaml::Node &p_Node,
                         Low::Util::Handle p_Creator)
      {
        Image l_Handle = Image::make(N(Image));

        if (p_Node["allocated_image"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      AllocatedImage &Image::get_allocated_image() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_allocated_image
        // LOW_CODEGEN::END::CUSTOM:GETTER_allocated_image

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(Image, allocated_image, AllocatedImage);
      }
      void Image::set_allocated_image(AllocatedImage &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_allocated_image
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_allocated_image

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(Image, allocated_image, AllocatedImage) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_allocated_image
        // LOW_CODEGEN::END::CUSTOM:SETTER_allocated_image
      }

      Low::Util::Name Image::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(Image, name, Low::Util::Name);
      }
      void Image::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(Image, name, Low::Util::Name) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      bool Image::unload()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload
        return ImageUtil::Internal::destroy(get_allocated_image());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
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
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(ImageData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(ImageData, allocated_image) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ImageData, allocated_image) *
                            (l_Capacity)],
                 l_Capacity * sizeof(AllocatedImage));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ImageData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ImageData, name) * (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::Name));
        }
        for (uint32_t i = l_Capacity;
             i < l_Capacity + l_CapacityIncrease; ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for Image from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
