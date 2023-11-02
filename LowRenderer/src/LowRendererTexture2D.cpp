#include "LowRendererTexture2D.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowUtilResource.h"

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Texture2D::TYPE_ID = 14;
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

      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture2D, image, Resource::Image))
          Resource::Image();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture2D, context, Interface::Context))
          Interface::Context();
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
      get_context().wait_idle();

      if (get_image().is_alive()) {
        get_image().destroy();

        if (ms_LivingInstances.size() > 0 &&
            ms_LivingInstances[0].get_image().is_alive()) {
          get_context().get_global_signature().set_sampler_resource(
              N(g_Texture2Ds), get_index(), ms_LivingInstances[0].get_image());
        }
      }
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
    }

    void Texture2D::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(Texture2D));

      initialize_buffer(&ms_Buffer, Texture2DData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Texture2D);
      LOW_PROFILE_ALLOC(type_slots_Texture2D);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Texture2D);
      l_TypeInfo.typeId = TYPE_ID;
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
        l_PropertyInfo.name = N(image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Resource::Image::TYPE_ID;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Texture2D l_Handle = p_Handle.get_id();
          l_Handle.get_image();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D, image,
                                            Resource::Image);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2DData, context);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Interface::Context::TYPE_ID;
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

      if (get_image().is_alive()) {
        get_image().serialize(p_Node["image"]);
      }
      if (get_context().is_alive()) {
        get_context().serialize(p_Node["context"]);
      }
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
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
      Texture2D l_Handle = Texture2D::make(N(Texture2D));

      if (p_Node["image"]) {
        l_Handle.set_image(
            Resource::Image::deserialize(p_Node["image"], l_Handle.get_id())
                .get_id());
      }
      if (p_Node["context"]) {
        l_Handle.set_context(Interface::Context::deserialize(p_Node["context"],
                                                             l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    Resource::Image Texture2D::get_image() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_image
      // LOW_CODEGEN::END::CUSTOM:GETTER_image

      return TYPE_SOA(Texture2D, image, Resource::Image);
    }
    void Texture2D::set_image(Resource::Image p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_image
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_image

      // Set new value
      TYPE_SOA(Texture2D, image, Resource::Image) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_image
      // LOW_CODEGEN::END::CUSTOM:SETTER_image
    }

    Interface::Context Texture2D::get_context() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context
      // LOW_CODEGEN::END::CUSTOM:GETTER_context

      return TYPE_SOA(Texture2D, context, Interface::Context);
    }
    void Texture2D::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_context
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_context

      // Set new value
      TYPE_SOA(Texture2D, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
      // LOW_CODEGEN::END::CUSTOM:SETTER_context
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

    Texture2D Texture2D::make(Util::Name p_Name, Interface::Context p_Context,
                              Util::Resource::Image2D &p_Image2d)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      Texture2D l_Texture2D = Texture2D::make(p_Name);

      l_Texture2D.assign_image(p_Context, p_Image2d);

      return l_Texture2D;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void Texture2D::assign_image(Interface::Context p_Context,
                                 Util::Resource::Image2D &p_Image2d)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_assign_image
      set_context(p_Context);

      Backend::ImageResourceCreateParams l_Params;
      l_Params.context = &p_Context.get_context();
      l_Params.createImage = true;
      l_Params.depth = false;
      l_Params.format = Backend::ImageFormat::RGBA8_UNORM;
      l_Params.writable = false;
      l_Params.mip0Size = p_Image2d.data.size();
      l_Params.mip0Data = p_Image2d.data.data();
      l_Params.mip0Dimensions = p_Image2d.dimensions;
      l_Params.sampleFilter = Backend::ImageSampleFilter::LINEAR;

      set_image(Resource::Image::make(get_name(), l_Params));

      p_Context.get_global_signature().set_sampler_resource(
          N(g_Texture2Ds), get_index(), get_image());
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_assign_image
    }

    uint32_t Texture2D::create_instance()
    {
      uint32_t l_Index = 0u;

      for (; l_Index < get_capacity(); ++l_Index) {
        if (!ms_Slots[l_Index].m_Occupied) {
          break;
        }
      }
      LOW_ASSERT(l_Index < get_capacity(), "Budget blown for type Texture2D");
      ms_Slots[l_Index].m_Occupied = true;
      return l_Index;
    }

  } // namespace Renderer
} // namespace Low
