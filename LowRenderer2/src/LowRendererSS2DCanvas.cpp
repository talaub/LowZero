#include "LowRendererSS2DCanvas.h"

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

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 SS2DCanvas::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        SS2DCanvas::IDENTIFIER(LOW_NAME(509652687),
                               LOW_NAME(2653676905));
    uint32_t SS2DCanvas::ms_Capacity = 0u;
    uint32_t SS2DCanvas::ms_PageSize = 0u;
    Low::Util::List<SS2DCanvas> SS2DCanvas::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        SS2DCanvas::ms_Pages;

    Low::Util::Handle SS2DCanvas::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    SS2DCanvas SS2DCanvas::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

      SS2DCanvas l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = SS2DCanvas::ms_TypeId;

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SS2DCanvas, draw_commands,
                                 Low::Util::List<SS2DDrawCommand>))
          Low::Util::List<SS2DDrawCommand>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SS2DCanvas, out_image,
                                 Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, SS2DCanvas, drawcommand_index_buffer,
          Low::Renderer::Buffer)) Low::Renderer::Buffer();
      ACCESSOR_TYPE_SOA(l_Handle, SS2DCanvas, z_dirty, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, SS2DCanvas, dimensions_dirty,
                        bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, SS2DCanvas, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void SS2DCanvas::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
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

    void SS2DCanvas::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(SS2DCanvas));

      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(SS2DCanvas));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, SS2DCanvas::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SS2DCanvas);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SS2DCanvas::is_alive;
      l_TypeInfo.destroy = &SS2DCanvas::destroy;
      l_TypeInfo.serialize = &SS2DCanvas::serialize;
      l_TypeInfo.deserialize = &SS2DCanvas::deserialize;
      l_TypeInfo.find_by_index = &SS2DCanvas::_find_by_index;
      l_TypeInfo.notify = &SS2DCanvas::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &SS2DCanvas::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &SS2DCanvas::_make;
      l_TypeInfo.duplicate_default = &SS2DCanvas::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &SS2DCanvas::living_instances);
      l_TypeInfo.get_living_count = &SS2DCanvas::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: draw_commands
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_commands);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DCanvas::Data, draw_commands);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_draw_commands();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SS2DCanvas, draw_commands,
              Low::Util::List<SS2DDrawCommand>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((Low::Util::List<SS2DDrawCommand> *)p_Data) =
              l_Handle.get_draw_commands();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_commands
      }
      {
        // Property: out_image
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(out_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DCanvas::Data, out_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::Texture::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_out_image();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DCanvas,
                                            out_image,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_out_image(*(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_out_image();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: out_image
      }
      {
        // Property: dimensions
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DCanvas::Data, dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SS2DCanvas, dimensions, Low::Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_actual_dimensions(
              *(Low::Math::UVector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((Low::Math::UVector2 *)p_Data) =
              l_Handle.get_dimensions();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dimensions
      }
      {
        // Property: desired_dimensions
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(desired_dimensions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DCanvas::Data, desired_dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_desired_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DCanvas,
                                            desired_dimensions,
                                            Low::Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_dimensions(*(Low::Math::UVector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((Low::Math::UVector2 *)p_Data) =
              l_Handle.get_desired_dimensions();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: desired_dimensions
      }
      {
        // Property: drawcommand_index_buffer
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(drawcommand_index_buffer);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DCanvas::Data, drawcommand_index_buffer);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Buffer::IDENTIFIER;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_drawcommand_index_buffer();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DCanvas,
                                            drawcommand_index_buffer,
                                            Low::Renderer::Buffer);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_drawcommand_index_buffer(
              *(Low::Renderer::Buffer *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((Low::Renderer::Buffer *)p_Data) =
              l_Handle.get_drawcommand_index_buffer();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: drawcommand_index_buffer
      }
      {
        // Property: backend_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(backend_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DCanvas::Data, backend_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_backend_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DCanvas,
                                            backend_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_backend_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_backend_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: backend_handle
      }
      {
        // Property: z_dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(z_dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DCanvas::Data, z_dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.is_z_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DCanvas,
                                            z_dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_z_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_z_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: z_dirty
      }
      {
        // Property: dimensions_dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions_dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DCanvas::Data, dimensions_dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.is_dimensions_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DCanvas,
                                            dimensions_dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_dimensions_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_dimensions_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dimensions_dirty
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SS2DCanvas::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DCanvas,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DCanvas l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void SS2DCanvas::cleanup()
    {
      Low::Util::List<SS2DCanvas> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle SS2DCanvas::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    SS2DCanvas SS2DCanvas::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      SS2DCanvas l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = SS2DCanvas::ms_TypeId;

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
        l_Handle.m_Data.m_Generation = 0;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      l_Handle.m_Data.m_Generation =
          l_Page->slots[l_SlotIndex].m_Generation;

      return l_Handle;
    }

    SS2DCanvas SS2DCanvas::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      SS2DCanvas l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = SS2DCanvas::ms_TypeId;

      return l_Handle;
    }

    bool SS2DCanvas::is_alive() const
    {
      if (m_Data.m_Type != SS2DCanvas::ms_TypeId) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      return m_Data.m_Type == SS2DCanvas::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t SS2DCanvas::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    SS2DCanvas::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    SS2DCanvas SS2DCanvas::find_by_name(Low::Util::Name p_Name)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME

      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
    }

    SS2DCanvas SS2DCanvas::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      SS2DCanvas l_Handle = make(p_Name);
      if (get_out_image().is_alive()) {
        l_Handle.set_out_image(get_out_image());
      }
      l_Handle.set_actual_dimensions(get_dimensions());
      l_Handle.set_dimensions(get_desired_dimensions());
      if (get_drawcommand_index_buffer().is_alive()) {
        l_Handle.set_drawcommand_index_buffer(
            get_drawcommand_index_buffer());
      }
      l_Handle.set_backend_handle(get_backend_handle());
      l_Handle.set_z_dirty(is_z_dirty());
      l_Handle.set_dimensions_dirty(is_dimensions_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    SS2DCanvas SS2DCanvas::duplicate(SS2DCanvas p_Handle,
                                     Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    SS2DCanvas::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
    {
      SS2DCanvas l_SS2DCanvas = p_Handle.get_id();
      return l_SS2DCanvas.duplicate(p_Name);
    }

    void SS2DCanvas::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void SS2DCanvas::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Serial::Node &p_Node)
    {
      SS2DCanvas l_SS2DCanvas = p_Handle.get_id();
      l_SS2DCanvas.serialize(p_Node);
    }

    Low::Util::Handle
    SS2DCanvas::deserialize(Low::Util::Serial::Node &p_Node,
                            Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void SS2DCanvas::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 SS2DCanvas::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 SS2DCanvas::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void SS2DCanvas::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void SS2DCanvas::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      SS2DCanvas l_SS2DCanvas = p_Observer.get_id();
      l_SS2DCanvas.notify(p_Observed, p_Observable);
    }

    Low::Util::List<SS2DDrawCommand> &
    SS2DCanvas::get_draw_commands() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_commands

      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_commands

      return TYPE_SOA(SS2DCanvas, draw_commands,
                      Low::Util::List<SS2DDrawCommand>);
    }

    Low::Renderer::Texture SS2DCanvas::get_out_image() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_out_image

      // LOW_CODEGEN::END::CUSTOM:GETTER_out_image

      return TYPE_SOA(SS2DCanvas, out_image, Low::Renderer::Texture);
    }
    void SS2DCanvas::set_out_image(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_out_image

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_out_image

      // Set new value
      TYPE_SOA(SS2DCanvas, out_image, Low::Renderer::Texture) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_out_image

      // LOW_CODEGEN::END::CUSTOM:SETTER_out_image

      broadcast_observable(N(out_image));
    }

    Low::Math::UVector2 SS2DCanvas::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions

      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions

      return TYPE_SOA(SS2DCanvas, dimensions, Low::Math::UVector2);
    }
    void SS2DCanvas::set_actual_dimensions(u32 p_X, u32 p_Y)
    {
      Low::Math::UVector2 l_Val(p_X, p_Y);
      set_actual_dimensions(l_Val);
    }

    void SS2DCanvas::set_actual_dimensions_x(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.x = p_Value;
      set_actual_dimensions(l_Value);
    }

    void SS2DCanvas::set_actual_dimensions_y(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.y = p_Value;
      set_actual_dimensions(l_Value);
    }

    void
    SS2DCanvas::set_actual_dimensions(Low::Math::UVector2 p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions

      // Set new value
      TYPE_SOA(SS2DCanvas, dimensions, Low::Math::UVector2) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions

      // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions

      broadcast_observable(N(dimensions));
    }

    Low::Math::UVector2 SS2DCanvas::get_desired_dimensions() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_desired_dimensions

      // LOW_CODEGEN::END::CUSTOM:GETTER_desired_dimensions

      return TYPE_SOA(SS2DCanvas, desired_dimensions,
                      Low::Math::UVector2);
    }
    void SS2DCanvas::set_dimensions(u32 p_X, u32 p_Y)
    {
      Low::Math::UVector2 l_Val(p_X, p_Y);
      set_dimensions(l_Val);
    }

    void SS2DCanvas::set_dimensions_x(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_desired_dimensions();
      l_Value.x = p_Value;
      set_dimensions(l_Value);
    }

    void SS2DCanvas::set_dimensions_y(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_desired_dimensions();
      l_Value.y = p_Value;
      set_dimensions(l_Value);
    }

    void SS2DCanvas::set_dimensions(Low::Math::UVector2 p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_desired_dimensions

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_desired_dimensions

      if (get_desired_dimensions() != p_Value) {
        // Set dirty flags
        mark_dimensions_dirty();

        // Set new value
        TYPE_SOA(SS2DCanvas, desired_dimensions,
                 Low::Math::UVector2) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_desired_dimensions

        // LOW_CODEGEN::END::CUSTOM:SETTER_desired_dimensions

        broadcast_observable(N(desired_dimensions));
      }
    }

    Low::Renderer::Buffer
    SS2DCanvas::get_drawcommand_index_buffer() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_drawcommand_index_buffer

      // LOW_CODEGEN::END::CUSTOM:GETTER_drawcommand_index_buffer

      return TYPE_SOA(SS2DCanvas, drawcommand_index_buffer,
                      Low::Renderer::Buffer);
    }
    void SS2DCanvas::set_drawcommand_index_buffer(
        Low::Renderer::Buffer p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_drawcommand_index_buffer

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_drawcommand_index_buffer

      // Set new value
      TYPE_SOA(SS2DCanvas, drawcommand_index_buffer,
               Low::Renderer::Buffer) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_drawcommand_index_buffer

      // LOW_CODEGEN::END::CUSTOM:SETTER_drawcommand_index_buffer

      broadcast_observable(N(drawcommand_index_buffer));
    }

    uint64_t SS2DCanvas::get_backend_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_backend_handle

      // LOW_CODEGEN::END::CUSTOM:GETTER_backend_handle

      return TYPE_SOA(SS2DCanvas, backend_handle, uint64_t);
    }
    void SS2DCanvas::set_backend_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_backend_handle

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_backend_handle

      // Set new value
      TYPE_SOA(SS2DCanvas, backend_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_backend_handle

      // LOW_CODEGEN::END::CUSTOM:SETTER_backend_handle

      broadcast_observable(N(backend_handle));
    }

    bool SS2DCanvas::is_z_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_z_dirty

      // LOW_CODEGEN::END::CUSTOM:GETTER_z_dirty

      return TYPE_SOA(SS2DCanvas, z_dirty, bool);
    }
    void SS2DCanvas::toggle_z_dirty()
    {
      set_z_dirty(!is_z_dirty());
    }

    void SS2DCanvas::set_z_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_z_dirty

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_z_dirty

      // Set new value
      TYPE_SOA(SS2DCanvas, z_dirty, bool) = p_Value;

      if (p_Value) {
        mark_z_dirty();
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_z_dirty

      // LOW_CODEGEN::END::CUSTOM:SETTER_z_dirty

      broadcast_observable(N(z_dirty));
    }

    void SS2DCanvas::mark_z_dirty()
    {
      if (!is_z_dirty()) {
        TYPE_SOA(SS2DCanvas, z_dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_z_dirty

        // LOW_CODEGEN::END::CUSTOM:MARK_z_dirty
      }
    }

    bool SS2DCanvas::is_dimensions_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions_dirty

      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions_dirty

      return TYPE_SOA(SS2DCanvas, dimensions_dirty, bool);
    }
    void SS2DCanvas::toggle_dimensions_dirty()
    {
      set_dimensions_dirty(!is_dimensions_dirty());
    }

    void SS2DCanvas::set_dimensions_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions_dirty

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions_dirty

      // Set new value
      TYPE_SOA(SS2DCanvas, dimensions_dirty, bool) = p_Value;

      if (p_Value) {
        mark_dimensions_dirty();
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions_dirty

      // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions_dirty

      broadcast_observable(N(dimensions_dirty));
    }

    void SS2DCanvas::mark_dimensions_dirty()
    {
      if (!is_dimensions_dirty()) {
        TYPE_SOA(SS2DCanvas, dimensions_dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dimensions_dirty

        // LOW_CODEGEN::END::CUSTOM:MARK_dimensions_dirty
      }
    }

    Low::Util::Name SS2DCanvas::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(SS2DCanvas, name, Low::Util::Name);
    }
    void SS2DCanvas::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(SS2DCanvas, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t SS2DCanvas::create_instance(u32 &p_PageIndex,
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
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
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

    u32 SS2DCanvas::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for SS2DCanvas.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, SS2DCanvas::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool SS2DCanvas::get_page_for_index(const u32 p_Index,
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

  } // namespace Renderer
} // namespace Low
