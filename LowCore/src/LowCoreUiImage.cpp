#include "LowCoreUiImage.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        const uint16_t Image::TYPE_ID = 40;
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

        Low::Util::Handle Image::_make(Low::Util::Handle p_Element)
        {
          Low::Core::UI::Element l_Element = p_Element.get_id();
          LOW_ASSERT(l_Element.is_alive(),
                     "Cannot create component for dead element");
          return make(l_Element).get_id();
        }

        Image Image::make(Low::Core::UI::Element p_Element)
        {
          uint32_t l_Index = create_instance();

          Image l_Handle;
          l_Handle.m_Data.m_Index = l_Index;
          l_Handle.m_Data.m_Generation =
              ms_Slots[l_Index].m_Generation;
          l_Handle.m_Data.m_Type = Image::TYPE_ID;

          new (&ACCESSOR_TYPE_SOA(l_Handle, Image, texture,
                                  Low::Core::Texture2D))
              Low::Core::Texture2D();
          new (&ACCESSOR_TYPE_SOA(l_Handle, Image, renderer_material,
                                  Renderer::Material))
              Renderer::Material();
          new (&ACCESSOR_TYPE_SOA(l_Handle, Image, element,
                                  Low::Core::UI::Element))
              Low::Core::UI::Element();

          l_Handle.set_element(p_Element);
          p_Element.add_component(l_Handle);

          ms_LivingInstances.push_back(l_Handle);

          l_Handle.set_unique_id(
              Low::Util::generate_unique_id(l_Handle.get_id()));
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
          l_Handle.set_renderer_material(Renderer::create_material(
              p_Element.get_name(),
              Renderer::MaterialType::find_by_name(N(ui_base))));
          // LOW_CODEGEN::END::CUSTOM:MAKE

          return l_Handle;
        }

        void Image::destroy()
        {
          LOW_ASSERT(is_alive(), "Cannot destroy dead object");

          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          if (get_renderer_material().is_alive()) {
            get_renderer_material().destroy();
          }
          // LOW_CODEGEN::END::CUSTOM:DESTROY

          Low::Util::remove_unique_id(get_unique_id());

          ms_Slots[this->m_Data.m_Index].m_Occupied = false;
          ms_Slots[this->m_Data.m_Index].m_Generation++;

          const Image *l_Instances = living_instances();
          bool l_LivingInstanceFound = false;
          for (uint32_t i = 0u; i < living_count(); ++i) {
            if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
              ms_LivingInstances.erase(ms_LivingInstances.begin() +
                                       i);
              l_LivingInstanceFound = true;
              break;
            }
          }
        }

        void Image::initialize()
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
          // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

          ms_Capacity =
              Low::Util::Config::get_capacity(N(LowCore), N(Image));

          initialize_buffer(&ms_Buffer, ImageData::get_size(),
                            get_capacity(), &ms_Slots);

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
          l_TypeInfo.make_default = nullptr;
          l_TypeInfo.make_component = &Image::_make;
          l_TypeInfo.duplicate_default = nullptr;
          l_TypeInfo.duplicate_component = &Image::_duplicate;
          l_TypeInfo.get_living_instances = reinterpret_cast<
              Low::Util::RTTI::LivingInstancesGetter>(
              &Image::living_instances);
          l_TypeInfo.get_living_count = &Image::living_count;
          l_TypeInfo.component = false;
          l_TypeInfo.uiComponent = true;
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(texture);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(ImageData, texture);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType = Low::Core::Texture2D::TYPE_ID;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.get_texture();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Image, texture, Low::Core::Texture2D);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Image l_Handle = p_Handle.get_id();
              l_Handle.set_texture(*(Low::Core::Texture2D *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(renderer_material);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(ImageData, renderer_material);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType = Renderer::Material::TYPE_ID;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.get_renderer_material();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Image,
                                                renderer_material,
                                                Renderer::Material);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(element);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset = offsetof(ImageData, element);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Core::UI::Element::TYPE_ID;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.get_element();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Image, element, Low::Core::UI::Element);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Image l_Handle = p_Handle.get_id();
              l_Handle.set_element(*(Low::Core::UI::Element *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(unique_id);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(ImageData, unique_id);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT64;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.get_unique_id();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Image, unique_id, Low::Util::UniqueId);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
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
          l_Handle.m_Data.m_Generation =
              ms_Slots[p_Index].m_Generation;
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

        Image Image::duplicate(Low::Core::UI::Element p_Element) const
        {
          _LOW_ASSERT(is_alive());

          Image l_Handle = make(p_Element);
          if (get_texture().is_alive()) {
            l_Handle.set_texture(get_texture());
          }
          if (get_renderer_material().is_alive()) {
            l_Handle.set_renderer_material(get_renderer_material());
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
          // LOW_CODEGEN::END::CUSTOM:DUPLICATE

          return l_Handle;
        }

        Image Image::duplicate(Image p_Handle,
                               Low::Core::UI::Element p_Element)
        {
          return p_Handle.duplicate(p_Element);
        }

        Low::Util::Handle
        Image::_duplicate(Low::Util::Handle p_Handle,
                          Low::Util::Handle p_Element)
        {
          Image l_Image = p_Handle.get_id();
          Low::Core::UI::Element l_Element = p_Element.get_id();
          return l_Image.duplicate(l_Element);
        }

        void Image::serialize(Low::Util::Yaml::Node &p_Node) const
        {
          _LOW_ASSERT(is_alive());

          p_Node["unique_id"] = get_unique_id();

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
          Image l_Handle = Image::make(p_Creator.get_id());

          if (p_Node["unique_id"]) {
            Low::Util::remove_unique_id(l_Handle.get_unique_id());
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<uint64_t>());
            Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                          l_Handle.get_id());
          }

          if (p_Node["unique_id"]) {
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<Low::Util::UniqueId>());
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
          // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

          return l_Handle;
        }

        Low::Core::Texture2D Image::get_texture() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture
          // LOW_CODEGEN::END::CUSTOM:GETTER_texture

          return TYPE_SOA(Image, texture, Low::Core::Texture2D);
        }
        void Image::set_texture(Low::Core::Texture2D p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture
          Core::Texture2D l_OldTexture =
              TYPE_SOA(Image, texture, Core::Texture2D);
          if (l_OldTexture.is_alive()) {
            l_OldTexture.unload();
          }
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

          // Set new value
          TYPE_SOA(Image, texture, Low::Core::Texture2D) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture
          if (p_Value.is_alive()) {
            p_Value.load();

            get_renderer_material().set_property(
                N(image_map),
                Util::Variant::from_handle(
                    p_Value.get_renderer_texture().get_id()));
          }
          // LOW_CODEGEN::END::CUSTOM:SETTER_texture
        }

        Renderer::Material Image::get_renderer_material() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderer_material
          // LOW_CODEGEN::END::CUSTOM:GETTER_renderer_material

          return TYPE_SOA(Image, renderer_material,
                          Renderer::Material);
        }
        void Image::set_renderer_material(Renderer::Material p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_renderer_material
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_renderer_material

          // Set new value
          TYPE_SOA(Image, renderer_material, Renderer::Material) =
              p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderer_material
          // LOW_CODEGEN::END::CUSTOM:SETTER_renderer_material
        }

        Low::Core::UI::Element Image::get_element() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_element
          // LOW_CODEGEN::END::CUSTOM:GETTER_element

          return TYPE_SOA(Image, element, Low::Core::UI::Element);
        }
        void Image::set_element(Low::Core::UI::Element p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_element
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_element

          // Set new value
          TYPE_SOA(Image, element, Low::Core::UI::Element) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_element
          // LOW_CODEGEN::END::CUSTOM:SETTER_element
        }

        Low::Util::UniqueId Image::get_unique_id() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

          return TYPE_SOA(Image, unique_id, Low::Util::UniqueId);
        }
        void Image::set_unique_id(Low::Util::UniqueId p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

          // Set new value
          TYPE_SOA(Image, unique_id, Low::Util::UniqueId) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
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
          l_CapacityIncrease = std::min(l_CapacityIncrease,
                                        LOW_UINT32_MAX - l_Capacity);

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
            memcpy(&l_NewBuffer[offsetof(ImageData, texture) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(ImageData, texture) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Core::Texture2D));
          }
          {
            memcpy(
                &l_NewBuffer[offsetof(ImageData, renderer_material) *
                             (l_Capacity + l_CapacityIncrease)],
                &ms_Buffer[offsetof(ImageData, renderer_material) *
                           (l_Capacity)],
                l_Capacity * sizeof(Renderer::Material));
          }
          {
            memcpy(&l_NewBuffer[offsetof(ImageData, element) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(ImageData, element) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Core::UI::Element));
          }
          {
            memcpy(&l_NewBuffer[offsetof(ImageData, unique_id) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(ImageData, unique_id) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Util::UniqueId));
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

      } // namespace Component
    }   // namespace UI
  }     // namespace Core
} // namespace Low
