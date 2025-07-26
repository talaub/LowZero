#include "LowRendererMaterial.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#define SET_PROPERTY(type)                                           \
  u32 l_Offset = get_material_type().get_input_offset(p_Name);       \
  set_dirty(true);                                                   \
  memcpy(&((u8 *)get_data())[l_Offset], &p_Value, sizeof(type));

#define GET_PROPERTY(type)                                           \
  u32 l_Offset = get_material_type().get_input_offset(p_Name);       \
  type l_Value;                                                      \
  memcpy(&l_Value, &((u8 *)get_data())[l_Offset], sizeof(type));     \
  return l_Value;

#include "LowRendererGlobals.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Material::TYPE_ID = 59;
    uint32_t Material::ms_Capacity = 0u;
    uint8_t *Material::ms_Buffer = 0;
    std::shared_mutex Material::ms_BufferMutex;
    Low::Util::Instances::Slot *Material::ms_Slots = 0;
    Low::Util::List<Material> Material::ms_LivingInstances =
        Low::Util::List<Material>();

    Material::Material() : Low::Util::Handle(0ull)
    {
    }
    Material::Material(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Material::Material(Material &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Material::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Material Material::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      Material l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Material::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Material, material_type,
                              MaterialType)) MaterialType();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Material, data,
                              Util::List<uint8_t>))
          Util::List<uint8_t>();
      ACCESSOR_TYPE_SOA(l_Handle, Material, dirty, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, Material, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.data().resize(MATERIAL_DATA_SIZE);

      l_Handle.set_dirty(true);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Material::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
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
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(Material));

      initialize_buffer(&ms_Buffer, MaterialData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_Material);
      LOW_PROFILE_ALLOC(type_slots_Material);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Material);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Material::is_alive;
      l_TypeInfo.destroy = &Material::destroy;
      l_TypeInfo.serialize = &Material::serialize;
      l_TypeInfo.deserialize = &Material::deserialize;
      l_TypeInfo.find_by_index = &Material::_find_by_index;
      l_TypeInfo.find_by_name = &Material::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Material::_make;
      l_TypeInfo.duplicate_default = &Material::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Material::living_instances);
      l_TypeInfo.get_living_count = &Material::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: material_type
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material_type);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialData, material_type);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = MaterialType::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          l_Handle.get_material_type();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Material, material_type, MaterialType);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          *((MaterialType *)p_Data) = l_Handle.get_material_type();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material_type
      }
      {
        // Property: data
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(data);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MaterialData, data);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data
      }
      {
        // Property: dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MaterialData, dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          l_Handle.is_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Material, dirty,
                                            bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Material l_Handle = p_Handle.get_id();
          l_Handle.set_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dirty
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MaterialData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
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
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = Material::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_MaterialType);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::MaterialType::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: get_data
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_data);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_data
      }
      {
        // Function: set_property_vector4
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_property_vector4);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Value);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_property_vector4
      }
      {
        // Function: set_property_vector3
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_property_vector3);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Value);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR3;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_property_vector3
      }
      {
        // Function: set_property_vector2
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_property_vector2);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Value);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR2;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_property_vector2
      }
      {
        // Function: set_property_float
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_property_float);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Value);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_property_float
      }
      {
        // Function: set_property_u32
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_property_u32);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Value);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UINT32;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_property_u32
      }
      {
        // Function: get_property_vector4
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_property_vector4);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_property_vector4
      }
      {
        // Function: get_property_vector3
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_property_vector3);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_property_vector3
      }
      {
        // Function: get_property_vector2
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_property_vector2);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VECTOR2;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_property_vector2
      }
      {
        // Function: get_property_float
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_property_float);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_property_float
      }
      {
        // Function: get_property_u32
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_property_u32);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_property_u32
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Material::cleanup()
    {
      Low::Util::List<Material> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Material);
      LOW_PROFILE_FREE(type_slots_Material);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle Material::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
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
      READ_LOCK(l_Lock);
      return m_Data.m_Type == Material::TYPE_ID &&
             check_alive(ms_Slots, Material::get_capacity());
    }

    uint32_t Material::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Material::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Material Material::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    Material Material::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Material l_Handle = make(p_Name);
      if (get_material_type().is_alive()) {
        l_Handle.set_material_type(get_material_type());
      }
      l_Handle.set_dirty(is_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Material Material::duplicate(Material p_Handle,
                                 Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Material::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
    {
      Material l_Material = p_Handle.get_id();
      return l_Material.duplicate(p_Name);
    }

    void Material::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["dirty"] = is_dirty();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      p_Node["material_type"] =
          get_material_type().get_name().c_str();

      Util::List<Util::Name> l_InputNames;
      get_material_type().fill_input_names(l_InputNames);

      Util::Yaml::Node &l_PropertiesNode = p_Node["properties"];

      for (u32 i = 0; i < l_InputNames.size(); ++i) {
        Util::Name i_Name = l_InputNames[i];
        const char *i_NameC = i_Name.c_str();
        Util::Yaml::Node &i_Node = l_PropertiesNode[i_NameC];
        MaterialTypeInputType i_Type =
            get_material_type().get_input_type(i_Name);
        switch (i_Type) {
        case MaterialTypeInputType::VECTOR4:
          Low::Util::Serialization::serialize(
              i_Node, get_property_vector4(i_Name));
          break;
        case MaterialTypeInputType::VECTOR3:
          Low::Util::Serialization::serialize(
              i_Node, get_property_vector3(i_Name));
          break;
        case MaterialTypeInputType::VECTOR2:
          Low::Util::Serialization::serialize(
              i_Node, get_property_vector2(i_Name));
          break;
        case MaterialTypeInputType::FLOAT:
          i_Node = get_property_float(i_Name);
          break;
        default:
          LOW_ASSERT(false, "Unsupported material type input type");
          break;
        }
      }
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Material::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Yaml::Node &p_Node)
    {
      Material l_Material = p_Handle.get_id();
      l_Material.serialize(p_Node);
    }

    Low::Util::Handle
    Material::deserialize(Low::Util::Yaml::Node &p_Node,
                          Low::Util::Handle p_Creator)
    {
      Material l_Handle = Material::make(N(Material));

      if (p_Node["data"]) {
      }
      if (p_Node["dirty"]) {
        l_Handle.set_dirty(p_Node["dirty"].as<bool>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      if (p_Node["material_type"]) {
        l_Handle.set_material_type(MaterialType::find_by_name(
            LOW_YAML_AS_NAME(p_Node["material_type"])));

        _LOW_ASSERT(l_Handle.get_material_type().is_alive());
      }

      if (p_Node["properties"]) {
        Util::List<Util::Name> l_InputNames;
        l_Handle.get_material_type().fill_input_names(l_InputNames);

        Util::Yaml::Node l_PropertiesNode = p_Node["properties"];

        for (u32 i = 0; i < l_InputNames.size(); ++i) {
          Util::Name i_Name = l_InputNames[i];
          const char *i_NameC = i_Name.c_str();
          Util::Yaml::Node &i_Node = l_PropertiesNode[i_NameC];
          MaterialTypeInputType i_Type =
              l_Handle.get_material_type().get_input_type(i_Name);
          switch (i_Type) {
          case MaterialTypeInputType::VECTOR4: {
            Math::Vector4 i_Val =
                Low::Util::Serialization::deserialize_vector4(i_Node);
            l_Handle.set_property_vector4(i_Name, i_Val);
            break;
          }
          case MaterialTypeInputType::VECTOR3: {
            Math::Vector3 i_Val =
                Low::Util::Serialization::deserialize_vector3(i_Node);
            l_Handle.set_property_vector3(i_Name, i_Val);
            break;
          }
          case MaterialTypeInputType::VECTOR2: {
            Math::Vector2 i_Val =
                Low::Util::Serialization::deserialize_vector2(i_Node);
            l_Handle.set_property_vector2(i_Name, i_Val);
            break;
          }
          case MaterialTypeInputType::FLOAT: {
            l_Handle.set_property_float(i_Name, i_Node.as<float>());
            break;
          }
          default:
            LOW_ASSERT(false, "Unsupported material type input type");
            break;
          }
        }
      }
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    MaterialType Material::get_material_type() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:GETTER_material_type

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Material, material_type, MaterialType);
    }
    void Material::set_material_type(MaterialType p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material_type

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Material, material_type, MaterialType) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:SETTER_material_type
    }

    Util::List<uint8_t> &Material::data() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data
      // LOW_CODEGEN::END::CUSTOM:GETTER_data

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Material, data, Util::List<uint8_t>);
    }

    bool Material::is_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Material, dirty, bool);
    }
    void Material::toggle_dirty()
    {
      set_dirty(!is_dirty());
    }

    void Material::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Material, dirty, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_dirty
    }

    Low::Util::Name Material::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Material, name, Low::Util::Name);
    }
    void Material::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Material, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    Material
    Material::make(Low::Util::Name p_Name,
                   Low::Renderer::MaterialType p_MaterialType)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      LOW_ASSERT(p_MaterialType.is_alive(),
                 "Cannot create material from dead type");

      Material l_Material = make(p_Name);
      l_Material.set_material_type(p_MaterialType);

      return l_Material;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void *Material::get_data() const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_data
      return data().data();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_data
    }

    void Material::set_property_vector4(Util::Name p_Name,
                                        Math::Vector4 &p_Value)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_vector4
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR4);
      SET_PROPERTY(Low::Math::Vector4);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_vector4
    }

    void Material::set_property_vector3(Util::Name p_Name,
                                        Math::Vector3 &p_Value)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_vector3
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR3);
      SET_PROPERTY(Low::Math::Vector3);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_vector3
    }

    void Material::set_property_vector2(Util::Name p_Name,
                                        Math::Vector2 &p_Value)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_vector2
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR2);
      SET_PROPERTY(Low::Math::Vector2);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_vector2
    }

    void Material::set_property_float(Util::Name p_Name,
                                      float p_Value)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_float
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::FLOAT);
      SET_PROPERTY(float);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_float
    }

    void Material::set_property_u32(Util::Name p_Name,
                                    uint32_t p_Value)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_u32
      _LOW_ASSERT(false);
      SET_PROPERTY(u32);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_u32
    }

    Low::Math::Vector4 &
    Material::get_property_vector4(Util::Name p_Name) const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_vector4
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR4);
      GET_PROPERTY(Low::Math::Vector4);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_vector4
    }

    Low::Math::Vector3 &
    Material::get_property_vector3(Util::Name p_Name) const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_vector3
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR3);
      GET_PROPERTY(Low::Math::Vector3);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_vector3
    }

    Low::Math::Vector2 &
    Material::get_property_vector2(Util::Name p_Name) const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_vector2
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR2);
      GET_PROPERTY(Low::Math::Vector2);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_vector2
    }

    float Material::get_property_float(Util::Name p_Name) const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_float
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::FLOAT);
      GET_PROPERTY(float);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_float
    }

    uint32_t Material::get_property_u32(Util::Name p_Name) const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_u32
      _LOW_ASSERT(false);
      //_LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
      // MaterialTypeInputType::);
      GET_PROPERTY(u32);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_u32
    }

    uint32_t Material::create_instance()
    {
      uint32_t l_Index = 0u;

      for (; l_Index < get_capacity(); ++l_Index) {
        if (!ms_Slots[l_Index].m_Occupied) {
          break;
        }
      }
      LOW_ASSERT(l_Index < get_capacity(),
                 "Budget blown for type Material");
      ms_Slots[l_Index].m_Occupied = true;
      return l_Index;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
