#include "LowCoreUiImage.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

#include "LowUtilAssetManager.h"
#include "LowRenderer.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        u16 Image::ms_TypeId = 0;
        const Low::Util::TypeIdentifier
            Image::IDENTIFIER(LOW_NAME(1181529166),
                              LOW_NAME(83635035));
        uint32_t Image::ms_Capacity = 0u;
        uint32_t Image::ms_PageSize = 0u;
        Low::Util::List<Image> Image::ms_LivingInstances;
        Low::Util::List<Low::Util::Instances::Page *> Image::ms_Pages;

        Low::Util::Handle Image::_make(Low::Util::Handle p_Element)
        {
          Low::Core::UI::Element l_Element = p_Element.get_id();
          LOW_ASSERT(l_Element.is_alive(),
                     "Cannot create component for dead element");
          return make(l_Element).get_id();
        }

        Image Image::make(Low::Core::UI::Element p_Element)
        {
          return make(p_Element, 0ull);
        }

        Image Image::make(Low::Core::UI::Element p_Element,
                          Low::Util::UniqueId p_UniqueId)
        {
          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          uint32_t l_Index =
              create_instance(l_PageIndex, l_SlotIndex);

          Image l_Handle;
          l_Handle.m_Data.m_Index = l_Index;
          l_Handle.m_Data.m_Generation =
              ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
          l_Handle.m_Data.m_Type = Image::ms_TypeId;

          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Image, texture,
                                     Low::Renderer::Texture))
              Low::Renderer::Texture();
          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Image, material,
                                     Low::Renderer::Material))
              Low::Renderer::Material();
          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Image, render_object,
                                     Low::Renderer::UiRenderObject))
              Low::Renderer::UiRenderObject();
          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Image, element,
                                     Low::Core::UI::Element))
              Low::Core::UI::Element();
          ACCESSOR_TYPE_SOA(l_Handle, Image, dirty, bool) = false;

          l_Handle.set_element(p_Element);
          p_Element.add_component(l_Handle);

          ms_LivingInstances.push_back(l_Handle);

          if (p_UniqueId > 0ull) {
            l_Handle.set_unique_id(p_UniqueId);
          } else {
            l_Handle.set_unique_id(
                Low::Util::generate_unique_id(l_Handle.get_id()));
          }
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

          l_Handle.set_material(
              Low::Renderer::get_default_material_ui());

          l_Handle.set_dirty(true);
          // LOW_CODEGEN::END::CUSTOM:MAKE

          return l_Handle;
        }

        void Image::destroy()
        {
          LOW_ASSERT(is_alive(), "Cannot destroy dead object");

          {
            // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

            if (get_render_object().is_alive()) {
              get_render_object().destroy();
            }
            // LOW_CODEGEN::END::CUSTOM:DESTROY
          }

          broadcast_observable(OBSERVABLE_DESTROY);

          Low::Util::remove_unique_id(get_unique_id());

          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                         l_SlotIndex));
          Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

          l_Page->slots[l_SlotIndex].m_Occupied = false;
          l_Page->slots[l_SlotIndex].m_Generation++;

          for (auto it = ms_LivingInstances.begin();
               it != ms_LivingInstances.end();) {
            if (it->get_id() == get_id()) {
              it = ms_LivingInstances.erase(it);
            } else {
              it++;
            }
          }
        }

        void Image::initialize()
        {
          const Low::Util::TypeIdentifier l_IdentifierNames(
              N(LowCore), N(Image));

          // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

          // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

          ms_Capacity =
              Low::Util::Config::get_capacity(N(LowCore), N(Image));

          ms_PageSize = Low::Math::Util::clamp(
              Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
          {
            u32 l_Capacity = 0u;
            while (l_Capacity < ms_Capacity) {
              Low::Util::Instances::Page *i_Page =
                  new Low::Util::Instances::Page;
              Low::Util::Instances::initialize_page(
                  i_Page, Image::Data::get_size(), ms_PageSize);
              ms_Pages.push_back(i_Page);
              l_Capacity += ms_PageSize;
            }
            ms_Capacity = l_Capacity;
          }

          Low::Util::RTTI::TypeInfo l_TypeInfo;
          l_TypeInfo.name = N(Image);
          l_TypeInfo.typeId = ms_TypeId;
          l_TypeInfo.get_capacity = &get_capacity;
          l_TypeInfo.is_alive = &Image::is_alive;
          l_TypeInfo.destroy = &Image::destroy;
          l_TypeInfo.serialize = &Image::serialize;
          l_TypeInfo.deserialize = &Image::deserialize;
          l_TypeInfo.find_by_index = &Image::_find_by_index;
          l_TypeInfo.notify = &Image::_notify;
          l_TypeInfo.post_load = nullptr;
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
            // Property: texture
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(texture);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Image::Data, texture);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Renderer::Texture::IDENTIFIER;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.get_texture();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Image, texture, Low::Renderer::Texture);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Image l_Handle = p_Handle.get_id();
              l_Handle.set_texture(*(Low::Renderer::Texture *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Image l_Handle = p_Handle.get_id();
              *((Low::Renderer::Texture *)p_Data) =
                  l_Handle.get_texture();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: texture
          }
          {
            // Property: material
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(material);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Image::Data, material);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Renderer::Material::IDENTIFIER;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.get_material();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Image, material, Low::Renderer::Material);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Image l_Handle = p_Handle.get_id();
              l_Handle.set_material(
                  *(Low::Renderer::Material *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Image l_Handle = p_Handle.get_id();
              *((Low::Renderer::Material *)p_Data) =
                  l_Handle.get_material();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: material
          }
          {
            // Property: render_object
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(render_object);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Image::Data, render_object);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Renderer::UiRenderObject::IDENTIFIER;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.get_render_object();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Image, render_object,
                  Low::Renderer::UiRenderObject);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Image l_Handle = p_Handle.get_id();
              l_Handle.set_render_object(
                  *(Low::Renderer::UiRenderObject *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Image l_Handle = p_Handle.get_id();
              *((Low::Renderer::UiRenderObject *)p_Data) =
                  l_Handle.get_render_object();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: render_object
          }
          {
            // Property: element
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(element);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Image::Data, element);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Core::UI::Element::IDENTIFIER;
            l_PropertyInfo.get_return =
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
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Image l_Handle = p_Handle.get_id();
              *((Low::Core::UI::Element *)p_Data) =
                  l_Handle.get_element();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: element
          }
          {
            // Property: unique_id
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(unique_id);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Image::Data, unique_id);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT64;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.get_unique_id();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Image, unique_id, Low::Util::UniqueId);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Image l_Handle = p_Handle.get_id();
              *((Low::Util::UniqueId *)p_Data) =
                  l_Handle.get_unique_id();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: unique_id
          }
          {
            // Property: dirty
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(dirty);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset = offsetof(Image::Data, dirty);
            l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Image l_Handle = p_Handle.get_id();
              l_Handle.is_dirty();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Image,
                                                dirty, bool);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Image l_Handle = p_Handle.get_id();
              l_Handle.set_dirty(*(bool *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Image l_Handle = p_Handle.get_id();
              *((bool *)p_Data) = l_Handle.is_dirty();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: dirty
          }
          ms_TypeId = Low::Util::Handle::register_type_info(
              IDENTIFIER, l_TypeInfo);
          // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE

          // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
        }

        void Image::cleanup()
        {
          Low::Util::List<Image> l_Instances = ms_LivingInstances;
          for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
            l_Instances[i].destroy();
          }
          for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
            Low::Util::Instances::Page *i_Page = *it;
            free(i_Page->buffer);
            free(i_Page->slots);
            delete i_Page;
            it = ms_Pages.erase(it);
          }

          ms_Capacity = 0;
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
          l_Handle.m_Data.m_Type = Image::ms_TypeId;

          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          if (!get_page_for_index(p_Index, l_PageIndex,
                                  l_SlotIndex)) {
            l_Handle.m_Data.m_Generation = 0;
          }
          Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
          l_Handle.m_Data.m_Generation =
              l_Page->slots[l_SlotIndex].m_Generation;

          return l_Handle;
        }

        Image Image::create_handle_by_index(u32 p_Index)
        {
          if (p_Index < get_capacity()) {
            return find_by_index(p_Index);
          }

          Image l_Handle;
          l_Handle.m_Data.m_Index = p_Index;
          l_Handle.m_Data.m_Generation = 0;
          l_Handle.m_Data.m_Type = Image::ms_TypeId;

          return l_Handle;
        }

        bool Image::is_alive() const
        {
          if (m_Data.m_Type != Image::ms_TypeId) {
            return false;
          }
          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          if (!get_page_for_index(get_index(), l_PageIndex,
                                  l_SlotIndex)) {
            return false;
          }
          Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
          return m_Data.m_Type == Image::ms_TypeId &&
                 l_Page->slots[l_SlotIndex].m_Occupied &&
                 l_Page->slots[l_SlotIndex].m_Generation ==
                     m_Data.m_Generation;
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
          if (get_material().is_alive()) {
            l_Handle.set_material(get_material());
          }
          if (get_render_object().is_alive()) {
            l_Handle.set_render_object(get_render_object());
          }
          l_Handle.set_dirty(is_dirty());

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

        void Image::serialize(Low::Util::Serial::Node &p_Node) const
        {
          _LOW_ASSERT(is_alive());

          p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};

          // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

          if (get_texture().is_alive()) {
            p_Node["texture"] =
                Util::U64Id{get_texture().get_unique_id()};
          }
          if (get_material().is_alive()) {
            p_Node["material"] =
                Util::U64Id{get_material().get_unique_id()};
          }
          // LOW_CODEGEN::END::CUSTOM:SERIALIZER
        }

        void Image::serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node)
        {
          Image l_Image = p_Handle.get_id();
          l_Image.serialize(p_Node);
        }

        Low::Util::Handle
        Image::deserialize(Low::Util::Serial::Node &p_Node,
                           Low::Util::Handle p_Creator)
        {
          Low::Util::UniqueId l_HandleUniqueId = 0ull;
          if (p_Node["unique_id"]) {
            l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
          } else if (p_Node["_unique_id"]) {
            l_HandleUniqueId = Low::Util::string_to_hash(
                p_Node["_unique_id"].as<Low::Util::String>());
          }

          Image l_Handle =
              Image::make(p_Creator.get_id(), l_HandleUniqueId);

          if (p_Node["unique_id"]) {
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<Low::Util::UniqueId>());
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

          if (p_Node["texture"]) {
            l_Handle.set_texture(Util::find_handle_by_unique_id(
                p_Node["texture"].as<Util::U64Id>().val));
          }
          if (p_Node["material"]) {
            l_Handle.set_material(Util::find_handle_by_unique_id(
                p_Node["material"].as<Util::U64Id>().val));
          }
          // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

          return l_Handle;
        }

        void Image::broadcast_observable(
            Low::Util::Name p_Observable) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          Low::Util::notify(l_Key);
        }

        u64 Image::observe(Low::Util::Name p_Observable,
                           Low::Util::Function<void(Low::Util::Handle,
                                                    Low::Util::Name)>
                               p_Observer) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          return Low::Util::observe(l_Key, p_Observer);
        }

        u64 Image::observe(Low::Util::Name p_Observable,
                           Low::Util::Handle p_Observer) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          return Low::Util::observe(l_Key, p_Observer);
        }

        void Image::notify(Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

          // LOW_CODEGEN::END::CUSTOM:NOTIFY
        }

        void Image::_notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
        {
          Image l_Image = p_Observer.get_id();
          l_Image.notify(p_Observed, p_Observable);
        }

        Low::Renderer::Texture Image::get_texture() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture

          // LOW_CODEGEN::END::CUSTOM:GETTER_texture

          return TYPE_SOA(Image, texture, Low::Renderer::Texture);
        }
        void Image::set_texture(Low::Renderer::Texture p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture
          if (get_texture().is_alive() && get_texture() != p_Value) {
            get_texture().dereference(get_id());
          }
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

          if (get_texture() != p_Value) {
            // Set dirty flags
            mark_dirty();

            // Set new value
            TYPE_SOA(Image, texture, Low::Renderer::Texture) =
                p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture
            if (p_Value.is_alive()) {
              p_Value.reference(get_id());
            }
            // LOW_CODEGEN::END::CUSTOM:SETTER_texture

            broadcast_observable(N(texture));
          }
        }

        Low::Renderer::Material Image::get_material() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material

          // LOW_CODEGEN::END::CUSTOM:GETTER_material

          return TYPE_SOA(Image, material, Low::Renderer::Material);
        }
        void Image::set_material(Low::Renderer::Material p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

          if (get_material() != p_Value) {
            // Set dirty flags
            mark_dirty();

            // Set new value
            TYPE_SOA(Image, material, Low::Renderer::Material) =
                p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material

            // LOW_CODEGEN::END::CUSTOM:SETTER_material

            broadcast_observable(N(material));
          }
        }

        Low::Renderer::UiRenderObject Image::get_render_object() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_object

          // LOW_CODEGEN::END::CUSTOM:GETTER_render_object

          return TYPE_SOA(Image, render_object,
                          Low::Renderer::UiRenderObject);
        }
        void Image::set_render_object(
            Low::Renderer::UiRenderObject p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_object

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_object

          // Set new value
          TYPE_SOA(Image, render_object,
                   Low::Renderer::UiRenderObject) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_object

          // LOW_CODEGEN::END::CUSTOM:SETTER_render_object

          broadcast_observable(N(render_object));
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

          broadcast_observable(N(element));
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

          broadcast_observable(N(unique_id));
        }

        bool Image::is_dirty() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty

          // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

          return TYPE_SOA(Image, dirty, bool);
        }
        void Image::toggle_dirty()
        {
          set_dirty(!is_dirty());
        }

        void Image::set_dirty(bool p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

          // Set new value
          TYPE_SOA(Image, dirty, bool) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty

          // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

          broadcast_observable(N(dirty));
        }

        void Image::mark_dirty()
        {
          if (!is_dirty()) {
            TYPE_SOA(Image, dirty, bool) = true;
            // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty

            // LOW_CODEGEN::END::CUSTOM:MARK_dirty
          }
        }

        uint32_t Image::create_instance(u32 &p_PageIndex,
                                        u32 &p_SlotIndex)
        {
          u32 l_Index = 0;
          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          bool l_FoundIndex = false;

          for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
               ++l_PageIndex) {
            for (l_SlotIndex = 0;
                 l_SlotIndex < ms_Pages[l_PageIndex]->size;
                 ++l_SlotIndex) {
              if (!ms_Pages[l_PageIndex]
                       ->slots[l_SlotIndex]
                       .m_Occupied) {
                l_FoundIndex = true;
                break;
              }
              l_Index++;
            }
            if (l_FoundIndex) {
              break;
            }
          }
          if (!l_FoundIndex) {
            l_SlotIndex = 0;
            l_PageIndex = create_page();
          }
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
          p_PageIndex = l_PageIndex;
          p_SlotIndex = l_SlotIndex;
          return l_Index;
        }

        u32 Image::create_page()
        {
          const u32 l_Capacity = get_capacity();
          LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                     "Could not increase capacity for Image.");

          Low::Util::Instances::Page *l_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              l_Page, Image::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(l_Page);

          ms_Capacity = l_Capacity + l_Page->size;
          return ms_Pages.size() - 1;
        }

        bool Image::get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex)
        {
          if (p_Index >= get_capacity()) {
            p_PageIndex = LOW_UINT32_MAX;
            p_SlotIndex = LOW_UINT32_MAX;
            return false;
          }
          p_PageIndex = p_Index / ms_PageSize;
          if (p_PageIndex > (ms_Pages.size() - 1)) {
            return false;
          }
          p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);
          return true;
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      } // namespace Component
    } // namespace UI
  } // namespace Core
} // namespace Low
