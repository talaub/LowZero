#include "LowRendererTexture2D.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowUtilResource.h"

namespace Low {
  namespace Renderer {
    const uint16_t Texture2D::TYPE_ID = 7;
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

    Texture2D Texture2D::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      Texture2D l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Texture2D::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture2D, image, Resource::Image))
          Resource::Image();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture2D, context, Interface::Context))
          Interface::Context();
      ACCESSOR_TYPE_SOA(l_Handle, Texture2D, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void Texture2D::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      get_image().destroy();
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
      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(Texture2D));

      initialize_buffer(&ms_Buffer, Texture2DData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Texture2D);
      LOW_PROFILE_ALLOC(type_slots_Texture2D);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Texture2D);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Texture2D::is_alive;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(image);
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D, image,
                                            Resource::Image);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Texture2D, image, Resource::Image) =
              *(Resource::Image *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, context);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D, context,
                                            Interface::Context);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Texture2D, context, Interface::Context) =
              *(Interface::Context *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, Texture2D, name, Low::Util::Name) =
              *(Low::Util::Name *)p_Data;
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

    bool Texture2D::is_alive() const
    {
      return m_Data.m_Type == Texture2D::TYPE_ID &&
             check_alive(ms_Slots, Texture2D::get_capacity());
    }

    uint32_t Texture2D::get_capacity()
    {
      return ms_Capacity;
    }

    Resource::Image Texture2D::get_image() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Texture2D, image, Resource::Image);
    }
    void Texture2D::set_image(Resource::Image p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Texture2D, image, Resource::Image) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_image
      // LOW_CODEGEN::END::CUSTOM:SETTER_image
    }

    Interface::Context Texture2D::get_context() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Texture2D, context, Interface::Context);
    }
    void Texture2D::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Texture2D, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
      // LOW_CODEGEN::END::CUSTOM:SETTER_context
    }

    Low::Util::Name Texture2D::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Texture2D, name, Low::Util::Name);
    }
    void Texture2D::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(Texture2D, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    Texture2D Texture2D::make(Util::Name p_Name, Interface::Context p_Context,
                              Util::Resource::Image2D &p_Image2d)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      Texture2D l_Texture2D = Texture2D::make(p_Name);
      l_Texture2D.set_context(p_Context);

      Backend::ImageResourceCreateParams l_Params;
      l_Params.context = &p_Context.get_context();
      l_Params.createImage = true;
      l_Params.depth = false;
      l_Params.format = Backend::ImageFormat::RGBA8_UNORM;
      l_Params.writable = false;
      l_Params.imageDataSize = p_Image2d.data[0].size();
      l_Params.imageData = p_Image2d.data[0].data();
      l_Params.dimensions = p_Image2d.dimensions[0];

      l_Texture2D.set_image(Resource::Image::make(p_Name, l_Params));

      p_Context.get_global_signature().set_sampler_resource(
          N(g_Texture2Ds), l_Texture2D.get_index(), l_Texture2D.get_image());

      return l_Texture2D;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
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
        memcpy(&l_NewBuffer[offsetof(Texture2DData, image) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(Texture2DData, image) * (l_Capacity)],
               l_Capacity * sizeof(Resource::Image));
      }
      {
        memcpy(&l_NewBuffer[offsetof(Texture2DData, context) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(Texture2DData, context) * (l_Capacity)],
               l_Capacity * sizeof(Interface::Context));
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
  } // namespace Renderer
} // namespace Low
