#include "LowRendererImageResource.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererVkImage.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t ImageResource::TYPE_ID = 48;
    uint32_t ImageResource::ms_Capacity = 0u;
    uint8_t *ImageResource::ms_Buffer = 0;
    std::shared_mutex ImageResource::ms_BufferMutex;
    Low::Util::Instances::Slot *ImageResource::ms_Slots = 0;
    Low::Util::List<ImageResource> ImageResource::ms_LivingInstances =
        Low::Util::List<ImageResource>();

    ImageResource::ImageResource() : Low::Util::Handle(0ull)
    {
    }
    ImageResource::ImageResource(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    ImageResource::ImageResource(ImageResource &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle ImageResource::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    ImageResource ImageResource::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      ImageResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = ImageResource::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, ImageResource, path,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, ImageResource, resource_image,
                              Util::Resource::ImageMipMaps))
          Util::Resource::ImageMipMaps();
      new (&ACCESSOR_TYPE_SOA(l_Handle, ImageResource, state,
                              ImageResourceState))
          ImageResourceState();
      new (&ACCESSOR_TYPE_SOA(l_Handle, ImageResource, loaded_mips,
                              Low::Util::List<uint8_t>))
          Low::Util::List<uint8_t>();
      ACCESSOR_TYPE_SOA(l_Handle, ImageResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_state(ImageResourceState::UNLOADED);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void ImageResource::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      Vulkan::Image l_VkImage = get_data_handle();
      if (get_state() == ImageResourceState::LOADED) {
        l_VkImage.unload();
      }
      l_VkImage.destroy();
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const ImageResource *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void ImageResource::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(ImageResource));

      initialize_buffer(&ms_Buffer, ImageResourceData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_ImageResource);
      LOW_PROFILE_ALLOC(type_slots_ImageResource);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(ImageResource);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &ImageResource::is_alive;
      l_TypeInfo.destroy = &ImageResource::destroy;
      l_TypeInfo.serialize = &ImageResource::serialize;
      l_TypeInfo.deserialize = &ImageResource::deserialize;
      l_TypeInfo.find_by_index = &ImageResource::_find_by_index;
      l_TypeInfo.find_by_name = &ImageResource::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &ImageResource::_make;
      l_TypeInfo.duplicate_default = &ImageResource::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &ImageResource::living_instances);
      l_TypeInfo.get_living_count = &ImageResource::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(ImageResourceData, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImageResource,
                                            path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ImageResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: resource_image
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ImageResourceData, resource_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.get_resource_image();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ImageResource, resource_image,
              Util::Resource::ImageMipMaps);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ImageResource l_Handle = p_Handle.get_id();
          *((Util::Resource::ImageMipMaps *)p_Data) =
              l_Handle.get_resource_image();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource_image
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ImageResourceData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            ImageResourceStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, ImageResource, state, ImageResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(ImageResourceState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ImageResource l_Handle = p_Handle.get_id();
          *((ImageResourceState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: data_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(data_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ImageResourceData, data_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.get_data_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImageResource,
                                            data_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.set_data_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ImageResource l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_data_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_handle
      }
      {
        // Property: loaded_mips
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(loaded_mips);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(ImageResourceData, loaded_mips);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.loaded_mips();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImageResource,
                                            loaded_mips,
                                            Low::Util::List<uint8_t>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ImageResource l_Handle = p_Handle.get_id();
          *((Low::Util::List<uint8_t> *)p_Data) =
              l_Handle.loaded_mips();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: loaded_mips
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(ImageResourceData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImageResource,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ImageResource l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          ImageResource l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = ImageResource::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void ImageResource::cleanup()
    {
      Low::Util::List<ImageResource> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_ImageResource);
      LOW_PROFILE_FREE(type_slots_ImageResource);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle ImageResource::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    ImageResource ImageResource::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      ImageResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = ImageResource::TYPE_ID;

      return l_Handle;
    }

    bool ImageResource::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == ImageResource::TYPE_ID &&
             check_alive(ms_Slots, ImageResource::get_capacity());
    }

    uint32_t ImageResource::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    ImageResource::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    ImageResource ImageResource::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    ImageResource
    ImageResource::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      ImageResource l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_state(get_state());
      l_Handle.set_data_handle(get_data_handle());
      l_Handle.set_loaded_mips(loaded_mips());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    ImageResource ImageResource::duplicate(ImageResource p_Handle,
                                           Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    ImageResource::_duplicate(Low::Util::Handle p_Handle,
                              Low::Util::Name p_Name)
    {
      ImageResource l_ImageResource = p_Handle.get_id();
      return l_ImageResource.duplicate(p_Name);
    }

    void ImageResource::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void ImageResource::serialize(Low::Util::Handle p_Handle,
                                  Low::Util::Yaml::Node &p_Node)
    {
      ImageResource l_ImageResource = p_Handle.get_id();
      l_ImageResource.serialize(p_Node);
    }

    Low::Util::Handle
    ImageResource::deserialize(Low::Util::Yaml::Node &p_Node,
                               Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return 0ull;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    Util::String &ImageResource::get_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(ImageResource, path, Util::String);
    }
    void ImageResource::set_path(const char *p_Value)
    {
      set_path(Low::Util::String(p_Value));
    }

    void ImageResource::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(ImageResource, path, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path
    }

    Util::Resource::ImageMipMaps &
    ImageResource::get_resource_image() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource_image
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource_image

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(ImageResource, resource_image,
                      Util::Resource::ImageMipMaps);
    }

    ImageResourceState ImageResource::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(ImageResource, state, ImageResourceState);
    }
    void ImageResource::set_state(ImageResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(ImageResource, state, ImageResourceState) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state
    }

    uint64_t ImageResource::get_data_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(ImageResource, data_handle, uint64_t);
    }
    void ImageResource::set_data_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(ImageResource, data_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_handle
    }

    Low::Util::List<uint8_t> &ImageResource::loaded_mips() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_loaded_mips
      // LOW_CODEGEN::END::CUSTOM:GETTER_loaded_mips

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(ImageResource, loaded_mips,
                      Low::Util::List<uint8_t>);
    }
    void
    ImageResource::set_loaded_mips(Low::Util::List<uint8_t> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_loaded_mips
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_loaded_mips

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(ImageResource, loaded_mips, Low::Util::List<uint8_t>) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_loaded_mips
      // LOW_CODEGEN::END::CUSTOM:SETTER_loaded_mips
    }

    Low::Util::Name ImageResource::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(ImageResource, name, Low::Util::Name);
    }
    void ImageResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(ImageResource, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    ImageResource ImageResource::make(Util::String &p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      Util::String l_FileName =
          p_Path.substr(p_Path.find_last_of("/\\") + 1);
      ImageResource l_Image =
          ImageResource::make(LOW_NAME(l_FileName.c_str()));
      l_Image.set_path(p_Path);

      return l_Image;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t ImageResource::create_instance()
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

    void ImageResource::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer =
          (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                            sizeof(ImageResourceData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(ImageResourceData, path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ImageResourceData, path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(ImageResourceData, resource_image) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(ImageResourceData, resource_image) *
                       (l_Capacity)],
            l_Capacity * sizeof(Util::Resource::ImageMipMaps));
      }
      {
        memcpy(&l_NewBuffer[offsetof(ImageResourceData, state) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ImageResourceData, state) *
                          (l_Capacity)],
               l_Capacity * sizeof(ImageResourceState));
      }
      {
        memcpy(&l_NewBuffer[offsetof(ImageResourceData, data_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ImageResourceData, data_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          ImageResource i_ImageResource = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(ImageResourceData, loaded_mips) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<uint8_t>))])
              Low::Util::List<uint8_t>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(i_ImageResource,
                                        ImageResource, loaded_mips,
                                        Low::Util::List<uint8_t>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(ImageResourceData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(ImageResourceData, name) *
                          (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for ImageResource from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
