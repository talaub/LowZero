#include "LowRendererImGuiImage.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t ImGuiImage::TYPE_ID = 6;
      uint32_t ImGuiImage::ms_Capacity = 0u;
      uint8_t *ImGuiImage::ms_Buffer = 0;
      std::shared_mutex ImGuiImage::ms_BufferMutex;
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

      Low::Util::Handle ImGuiImage::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      ImGuiImage ImGuiImage::make(Low::Util::Name p_Name)
      {
        WRITE_LOCK(l_Lock);
        uint32_t l_Index = create_instance();

        ImGuiImage l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = ImGuiImage::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, ImGuiImage, imgui_image,
                                Backend::ImGuiImage))
            Backend::ImGuiImage();
        new (&ACCESSOR_TYPE_SOA(l_Handle, ImGuiImage, image,
                                Resource::Image)) Resource::Image();
        ACCESSOR_TYPE_SOA(l_Handle, ImGuiImage, name,
                          Low::Util::Name) = Low::Util::Name(0u);
        LOCK_UNLOCK(l_Lock);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void ImGuiImage::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        Backend::callbacks().imgui_image_cleanup(get_imgui_image());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        WRITE_LOCK(l_Lock);
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
      }

      void ImGuiImage::initialize()
      {
        WRITE_LOCK(l_Lock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                      N(ImGuiImage));

        initialize_buffer(&ms_Buffer, ImGuiImageData::get_size(),
                          get_capacity(), &ms_Slots);
        LOCK_UNLOCK(l_Lock);

        LOW_PROFILE_ALLOC(type_buffer_ImGuiImage);
        LOW_PROFILE_ALLOC(type_slots_ImGuiImage);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(ImGuiImage);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &ImGuiImage::is_alive;
        l_TypeInfo.destroy = &ImGuiImage::destroy;
        l_TypeInfo.serialize = &ImGuiImage::serialize;
        l_TypeInfo.deserialize = &ImGuiImage::deserialize;
        l_TypeInfo.find_by_index = &ImGuiImage::_find_by_index;
        l_TypeInfo.find_by_name = &ImGuiImage::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &ImGuiImage::_make;
        l_TypeInfo.duplicate_default = &ImGuiImage::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &ImGuiImage::living_instances);
        l_TypeInfo.get_living_count = &ImGuiImage::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: imgui_image
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(imgui_image);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ImGuiImageData, imgui_image);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ImGuiImage l_Handle = p_Handle.get_id();
            l_Handle.get_imgui_image();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImGuiImage,
                                              imgui_image,
                                              Backend::ImGuiImage);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ImGuiImage l_Handle = p_Handle.get_id();
            *((Backend::ImGuiImage *)p_Data) =
                l_Handle.get_imgui_image();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: imgui_image
        }
        {
          // Property: image
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(image);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ImGuiImageData, image);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Resource::Image::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ImGuiImage l_Handle = p_Handle.get_id();
            l_Handle.get_image();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImGuiImage,
                                              image, Resource::Image);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ImGuiImage l_Handle = p_Handle.get_id();
            *((Resource::Image *)p_Data) = l_Handle.get_image();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: image
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ImGuiImageData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            ImGuiImage l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, ImGuiImage,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            ImGuiImage l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            ImGuiImage l_Handle = p_Handle.get_id();
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
          l_FunctionInfo.handleType = ImGuiImage::TYPE_ID;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Image);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Resource::Image::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: render
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(render);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Dimensions);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: render
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void ImGuiImage::cleanup()
      {
        Low::Util::List<ImGuiImage> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        WRITE_LOCK(l_Lock);
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_ImGuiImage);
        LOW_PROFILE_FREE(type_slots_ImGuiImage);
        LOCK_UNLOCK(l_Lock);
      }

      Low::Util::Handle ImGuiImage::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      ImGuiImage ImGuiImage::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        ImGuiImage l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = ImGuiImage::TYPE_ID;

        return l_Handle;
      }

      bool ImGuiImage::is_alive() const
      {
        READ_LOCK(l_Lock);
        return m_Data.m_Type == ImGuiImage::TYPE_ID &&
               check_alive(ms_Slots, ImGuiImage::get_capacity());
      }

      uint32_t ImGuiImage::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      ImGuiImage::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      ImGuiImage ImGuiImage::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return 0ull;
      }

      ImGuiImage ImGuiImage::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        ImGuiImage l_Handle = make(p_Name);
        if (get_image().is_alive()) {
          l_Handle.set_image(get_image());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      ImGuiImage ImGuiImage::duplicate(ImGuiImage p_Handle,
                                       Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      ImGuiImage::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
      {
        ImGuiImage l_ImGuiImage = p_Handle.get_id();
        return l_ImGuiImage.duplicate(p_Name);
      }

      void ImGuiImage::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        if (get_image().is_alive()) {
          get_image().serialize(p_Node["image"]);
        }
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void ImGuiImage::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
      {
        ImGuiImage l_ImGuiImage = p_Handle.get_id();
        l_ImGuiImage.serialize(p_Node);
      }

      Low::Util::Handle
      ImGuiImage::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
      {
        ImGuiImage l_Handle = ImGuiImage::make(N(ImGuiImage));

        if (p_Node["imgui_image"]) {
        }
        if (p_Node["image"]) {
          l_Handle.set_image(Resource::Image::deserialize(
                                 p_Node["image"], l_Handle.get_id())
                                 .get_id());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Backend::ImGuiImage &ImGuiImage::get_imgui_image() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_imgui_image

        // LOW_CODEGEN::END::CUSTOM:GETTER_imgui_image

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ImGuiImage, imgui_image, Backend::ImGuiImage);
      }

      Resource::Image ImGuiImage::get_image() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_image

        // LOW_CODEGEN::END::CUSTOM:GETTER_image

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ImGuiImage, image, Resource::Image);
      }
      void ImGuiImage::set_image(Resource::Image p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_image

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_image

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ImGuiImage, image, Resource::Image) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_image

        // LOW_CODEGEN::END::CUSTOM:SETTER_image
      }

      Low::Util::Name ImGuiImage::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(ImGuiImage, name, Low::Util::Name);
      }
      void ImGuiImage::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(ImGuiImage, name, Low::Util::Name) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      ImGuiImage ImGuiImage::make(Util::Name p_Name,
                                  Resource::Image p_Image)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

        ImGuiImage l_ImGuiImage = ImGuiImage::make(p_Name);

        l_ImGuiImage.set_image(p_Image);

        Backend::callbacks().imgui_image_create(
            l_ImGuiImage.get_imgui_image(), p_Image.get_image());

        return l_ImGuiImage;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      void ImGuiImage::render(Math::UVector2 &p_Dimensions)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_render

        if (!get_image().is_alive()) {
          LOW_LOG_WARN << "DEAD" << LOW_LOG_END;
        }
        Backend::callbacks().imgui_image_render(get_imgui_image(),
                                                p_Dimensions);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_render
      }

      uint32_t ImGuiImage::create_instance()
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

      void ImGuiImage::increase_budget()
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
                              sizeof(ImGuiImageData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(&l_NewBuffer[offsetof(ImGuiImageData, imgui_image) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ImGuiImageData, imgui_image) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Backend::ImGuiImage));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ImGuiImageData, image) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ImGuiImageData, image) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Resource::Image));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ImGuiImageData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ImGuiImageData, name) *
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

        LOW_LOG_DEBUG << "Auto-increased budget for ImGuiImage from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
