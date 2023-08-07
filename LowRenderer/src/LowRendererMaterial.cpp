#include "LowRendererMaterial.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowRendererTexture2D.h"

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Material::TYPE_ID = 17;
    uint32_t Material::ms_Capacity = 0u;
    uint8_t *Material::ms_Buffer = 0;
    Low::Util::Instances::Slot *Material::ms_Slots = 0;
    Low::Util::List<Material> Material::ms_LivingInstances =
        Low::Util::List<Material>();

    Material::Material() : Low::Util::Handle(0ull)
    {
    }
    Material::Material(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Material::Material(Material &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Material::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Material Material::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      Material l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Material::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Material, material_type, MaterialType))
          MaterialType();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Material, context, Interface::Context))
          Interface::Context();
      ACCESSOR_TYPE_SOA(l_Handle, Material, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Material::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Material *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Material::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(Material));

      initialize_buffer(&ms_Buffer, MaterialData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Material);
      LOW_PROFILE_ALLOC(type_slots_Material);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Material);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Material::is_alive;
      l_TypeInfo.destroy = &Material::destroy;
      l_TypeInfo.serialize = &Material::serialize;
      l_TypeInfo.deserialize = &Material::deserialize;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Material::_make;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Material::living_instances);
      l_TypeInfo.get_living_count = &Material::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material_type);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MaterialData, material_type);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = MaterialType::TYPE_ID;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          l_Handle.get_material_type();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Material, material_type,
                                            MaterialType);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Material l_Handle = p_Handle.get_id();
          l_Handle.set_material_type(*(MaterialType *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MaterialData, context);
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
        l_PropertyInfo.dataOffset = offsetof(MaterialData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Material, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Material l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Material::cleanup()
    {
      Low::Util::List<Material> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Material);
      LOW_PROFILE_FREE(type_slots_Material);
    }

    Material Material::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Material l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Material::TYPE_ID;

      return l_Handle;
    }

    bool Material::is_alive() const
    {
      return m_Data.m_Type == Material::TYPE_ID &&
             check_alive(ms_Slots, Material::get_capacity());
    }

    uint32_t Material::get_capacity()
    {
      return ms_Capacity;
    }

    Material Material::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    void Material::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      if (get_material_type().is_alive()) {
        get_material_type().serialize(p_Node["material_type"]);
      }
      if (get_context().is_alive()) {
        get_context().serialize(p_Node["context"]);
      }
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Material::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Yaml::Node &p_Node)
    {
      Material l_Material = p_Handle.get_id();
      l_Material.serialize(p_Node);
    }

    Low::Util::Handle Material::deserialize(Low::Util::Yaml::Node &p_Node,
                                            Low::Util::Handle p_Creator)
    {
      Material l_Handle = Material::make(N(Material));

      if (p_Node["material_type"]) {
        l_Handle.set_material_type(
            MaterialType::deserialize(p_Node["material_type"],
                                      l_Handle.get_id())
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

    MaterialType Material::get_material_type() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:GETTER_material_type

      return TYPE_SOA(Material, material_type, MaterialType);
    }
    void Material::set_material_type(MaterialType p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material_type

      // Set new value
      TYPE_SOA(Material, material_type, MaterialType) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:SETTER_material_type
    }

    Interface::Context Material::get_context() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context
      // LOW_CODEGEN::END::CUSTOM:GETTER_context

      return TYPE_SOA(Material, context, Interface::Context);
    }
    void Material::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_context
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_context

      // Set new value
      TYPE_SOA(Material, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
      // LOW_CODEGEN::END::CUSTOM:SETTER_context
    }

    Low::Util::Name Material::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Material, name, Low::Util::Name);
    }
    void Material::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Material, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    Material Material::make(Util::Name p_Name, Interface::Context p_Context)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      Material l_Material = Material::make(p_Name);

      l_Material.set_context(p_Context);

      // Clear material data in buffer
      Math::Vector4 l_EmptyData[LOW_RENDERER_MATERIAL_DATA_VECTORS];
      l_Material.get_context().get_material_data_buffer().write(
          l_EmptyData,
          LOW_RENDERER_MATERIAL_DATA_VECTORS * sizeof(Math::Vector4),
          (l_Material.get_index() *
           (LOW_RENDERER_MATERIAL_DATA_VECTORS * sizeof(Math::Vector4))));

      return l_Material;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void Material::set_property(Util::Name p_PropertyName,
                                Util::Variant &p_Value)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property
      for (MaterialTypeProperty &i_MaterialTypeProperty :
           get_material_type().get_properties()) {
        if (p_PropertyName == i_MaterialTypeProperty.name) {

          if (i_MaterialTypeProperty.type ==
              MaterialTypePropertyType::VECTOR4) {
            uint32_t i_Size = sizeof(Math::Vector4);
            uint32_t i_Offset = i_MaterialTypeProperty.offset;

            Math::Vector4 i_Data = p_Value;

            get_context().get_material_data_buffer().write(
                &i_Data, i_Size,
                i_Offset + (get_index() * (LOW_RENDERER_MATERIAL_DATA_VECTORS *
                                           sizeof(Math::Vector4))));

          } else if (i_MaterialTypeProperty.type ==
                     MaterialTypePropertyType::TEXTURE2D) {
            uint32_t i_Size = sizeof(uint32_t);
            uint32_t i_Offset = i_MaterialTypeProperty.offset;

            Util::Handle i_DataHandle = p_Value;
            uint32_t i_TextureIndex = 0;
            if (Texture2D::is_alive(i_DataHandle)) {
              i_TextureIndex = i_DataHandle.get_index();
            }
            float i_TextureIndexFloat = i_TextureIndex;

            get_context().get_material_data_buffer().write(
                &i_TextureIndexFloat, i_Size,
                i_Offset + (get_index() * (LOW_RENDERER_MATERIAL_DATA_VECTORS *
                                           sizeof(Math::Vector4))));
          } else {
            LOW_ASSERT(false, "Unknown material property type");
          }

          return;
        }
      }

      LOW_ASSERT(false, "Could not find property");
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property
    }

    uint32_t Material::create_instance()
    {
      uint32_t l_Index = 0u;

      for (; l_Index < get_capacity(); ++l_Index) {
        if (!ms_Slots[l_Index].m_Occupied) {
          break;
        }
      }
      LOW_ASSERT(l_Index < get_capacity(), "Budget blown for type Material");
      ms_Slots[l_Index].m_Occupied = true;
      return l_Index;
    }

  } // namespace Renderer
} // namespace Low
