#include "LowCoreTexture2D.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowUtilResource.h"
#include "LowRenderer.h"

namespace Low {
  namespace Core {
    const uint16_t Texture2D::TYPE_ID = 22;
    uint32_t Texture2D::ms_Capacity = 0u;
    uint8_t *Texture2D::ms_Buffer = 0;
    Low::Util::Instances::Slot *Texture2D::ms_Slots = 0;
    Low::Util::List<Texture2D> Texture2D::ms_LivingInstances =
        Low::Util::List<Texture2D>();

    Texture2D::Texture2D() : Low::Util::Handle(0ull)
    {
    }
    Texture2D::Texture2D(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Texture2D::Texture2D(Texture2D &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Texture2D::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Texture2D Texture2D::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      Texture2D l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Texture2D::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture2D, path, Util::String))
          Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture2D, renderer_texture,
                              Renderer::Texture2D)) Renderer::Texture2D();
      ACCESSOR_TYPE_SOA(l_Handle, Texture2D, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Texture2D::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Texture2D *l_Instances = living_instances();
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

    void Texture2D::initialize()
    {
      ms_Capacity = Low::Util::Config::get_capacity(N(LowCore), N(Texture2D));

      initialize_buffer(&ms_Buffer, Texture2DData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Texture2D);
      LOW_PROFILE_ALLOC(type_slots_Texture2D);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Texture2D);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Texture2D::is_alive;
      l_TypeInfo.destroy = &Texture2D::destroy;
      l_TypeInfo.serialize = &Texture2D::serialize;
      l_TypeInfo.deserialize = &Texture2D::deserialize;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Texture2D::_make;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Texture2D::living_instances);
      l_TypeInfo.get_living_count = &Texture2D::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Texture2D l_Handle = p_Handle.get_id();
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D, path,
                                            Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(renderer_texture);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, renderer_texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Renderer::Texture2D::TYPE_ID;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Texture2D l_Handle = p_Handle.get_id();
          l_Handle.get_renderer_texture();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Texture2D, renderer_texture, Renderer::Texture2D);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Texture2D l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Texture2D l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Texture2D::cleanup()
    {
      Low::Util::List<Texture2D> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Texture2D);
      LOW_PROFILE_FREE(type_slots_Texture2D);
    }

    Texture2D Texture2D::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Texture2D l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Texture2D::TYPE_ID;

      return l_Handle;
    }

    bool Texture2D::is_alive() const
    {
      return m_Data.m_Type == Texture2D::TYPE_ID &&
             check_alive(ms_Slots, Texture2D::get_capacity());
    }

    uint32_t Texture2D::get_capacity()
    {
      return ms_Capacity;
    }

    Texture2D Texture2D::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    void Texture2D::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      p_Node = get_path().c_str();
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Texture2D::serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node)
    {
      Texture2D l_Texture2D = p_Handle.get_id();
      l_Texture2D.serialize(p_Node);
    }

    Low::Util::Handle Texture2D::deserialize(Low::Util::Yaml::Node &p_Node,
                                             Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      Texture2D l_Texture = Texture2D::make(LOW_YAML_AS_STRING(p_Node));
      return l_Texture;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    Util::String &Texture2D::get_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(Texture2D, path, Util::String);
    }
    void Texture2D::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(Texture2D, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path
    }

    Renderer::Texture2D Texture2D::get_renderer_texture() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderer_texture
      // LOW_CODEGEN::END::CUSTOM:GETTER_renderer_texture

      return TYPE_SOA(Texture2D, renderer_texture, Renderer::Texture2D);
    }
    void Texture2D::set_renderer_texture(Renderer::Texture2D p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_renderer_texture
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_renderer_texture

      // Set new value
      TYPE_SOA(Texture2D, renderer_texture, Renderer::Texture2D) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderer_texture
      // LOW_CODEGEN::END::CUSTOM:SETTER_renderer_texture
    }

    uint32_t Texture2D::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(Texture2D, reference_count, uint32_t);
    }
    void Texture2D::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(Texture2D, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count
    }

    Low::Util::Name Texture2D::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Texture2D, name, Low::Util::Name);
    }
    void Texture2D::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Texture2D, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    Texture2D Texture2D::make(Util::String &p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      Util::String l_FileName = p_Path.substr(p_Path.find_last_of("/\\") + 1);
      Texture2D l_Texture = Texture2D::make(LOW_NAME(l_FileName.c_str()));
      l_Texture.set_path(p_Path);

      l_Texture.set_reference_count(0);

      return l_Texture;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    bool Texture2D::is_loaded()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded
      return get_renderer_texture().is_alive();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void Texture2D::load()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load
      LOW_ASSERT(is_alive(), "Texture2D was not alive on load");

      set_reference_count(get_reference_count() + 1);

      LOW_ASSERT(get_reference_count() > 0,
                 "Increased Texture2D reference count, but its not over 0. "
                 "Something went wrong.");

      if (is_loaded()) {
        return;
      }

      Util::String l_FullPath =
          Util::String(LOW_DATA_PATH) + "\\resources\\img2d\\" + get_path();

      Util::Resource::Image2D l_Image;
      Util::Resource::load_image2d(l_FullPath, l_Image);

      Renderer::Texture2D l_RendererTexture =
          Renderer::upload_texture(N(Texture2D), l_Image);

      set_renderer_texture(l_RendererTexture);

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
    }

    void Texture2D::unload()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload
      set_reference_count(get_reference_count() - 1);

      LOW_ASSERT(get_reference_count() >= 0,
                 "Texture2D reference count < 0. Something went wrong.");

      if (get_reference_count() <= 0) {
        _unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
    }

    void Texture2D::_unload()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__unload
      get_renderer_texture().destroy();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__unload
    }

    uint32_t Texture2D::create_instance()
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

    void Texture2D::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(Texture2DData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(Texture2DData, path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(Texture2DData, path) * (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(Texture2DData, renderer_texture) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(Texture2DData, renderer_texture) *
                          (l_Capacity)],
               l_Capacity * sizeof(Renderer::Texture2D));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(Texture2DData, reference_count) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(Texture2DData, reference_count) * (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(Texture2DData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(Texture2DData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for Texture2D from " << l_Capacity
                    << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Core
} // namespace Low
