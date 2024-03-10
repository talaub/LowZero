#include "LowCoreMaterial.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowRenderer.h"
#include "LowUtilSerialization.h"

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Material::TYPE_ID = 24;
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
      uint32_t l_Index = create_instance();

      Material l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Material::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Material, material_type,
                              Renderer::MaterialType))
          Renderer::MaterialType();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Material, renderer_material,
                              Renderer::Material))
          Renderer::Material();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, Material, properties,
          SINGLE_ARG(Util::Map<Util::Name, Util::Variant>)))
          Util::Map<Util::Name, Util::Variant>();
      ACCESSOR_TYPE_SOA(l_Handle, Material, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      l_Handle.set_unique_id(
          Low::Util::generate_unique_id(l_Handle.get_id()));
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      for (auto it =
               Renderer::MaterialType::ms_LivingInstances.begin();
           it != Renderer::MaterialType::ms_LivingInstances.end();
           ++it) {
        if (!it->is_internal()) {
          l_Handle.set_material_type(*it);
          break;
        }
      }
      LOW_ASSERT(l_Handle.get_material_type().is_alive(),
                 "Could not find valid material type");

      l_Handle.set_reference_count(0);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Material::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      _unload();
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      Low::Util::remove_unique_id(get_unique_id());

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
          Low::Util::Config::get_capacity(N(LowCore), N(Material));

      initialize_buffer(&ms_Buffer, MaterialData::get_size(),
                        get_capacity(), &ms_Slots);

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
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material_type);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialData, material_type);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Renderer::MaterialType::TYPE_ID;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          l_Handle.get_material_type();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Material,
                                            material_type,
                                            Renderer::MaterialType);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Material l_Handle = p_Handle.get_id();
          l_Handle.set_material_type(
              *(Renderer::MaterialType *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(renderer_material);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialData, renderer_material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Renderer::Material::TYPE_ID;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          l_Handle.get_renderer_material();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Material,
                                            renderer_material,
                                            Renderer::Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(properties);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialData, properties);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          l_Handle.get_properties();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Material, properties,
              SINGLE_ARG(Util::Map<Util::Name, Util::Variant>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialData, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(MaterialData, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Material, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(MaterialData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get =
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
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    Material Material::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Material l_Handle = make(p_Name);
      if (get_material_type().is_alive()) {
        l_Handle.set_material_type(get_material_type());
      }
      if (get_renderer_material().is_alive()) {
        l_Handle.set_renderer_material(get_renderer_material());
      }
      l_Handle.set_reference_count(get_reference_count());

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

      if (get_material_type().is_alive()) {
        get_material_type().serialize(p_Node["material_type"]);
      }
      p_Node["unique_id"] = get_unique_id();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      Util::Yaml::Node l_Properties;
      for (auto it = get_material_type().get_properties().begin();
           it != get_material_type().get_properties().end(); ++it) {
        const char *i_PropName = it->name.c_str();
        if (it->type ==
            Renderer::MaterialTypePropertyType::TEXTURE2D) {
          Texture2D i_Texture = get_properties()[it->name].m_Uint64;
          if (i_Texture.is_alive()) {
            i_Texture.serialize(l_Properties[i_PropName]);
          }
        } else if (it->type ==
                   Renderer::MaterialTypePropertyType::VECTOR4) {
          Math::Vector4 i_Value = get_properties()[it->name];
          Util::Serialization::serialize(l_Properties[i_PropName],
                                         i_Value);
        }
      }
      p_Node["properties"] = l_Properties;
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

      if (p_Node["unique_id"]) {
        Low::Util::remove_unique_id(l_Handle.get_unique_id());
        l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());
      }

      if (p_Node["material_type"]) {
        l_Handle.set_material_type(
            Renderer::MaterialType::deserialize(
                p_Node["material_type"], l_Handle.get_id())
                .get_id());
      }
      if (p_Node["properties"]) {
      }
      if (p_Node["unique_id"]) {
        l_Handle.set_unique_id(
            p_Node["unique_id"].as<Low::Util::UniqueId>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      for (auto it =
               l_Handle.get_material_type().get_properties().begin();
           it != l_Handle.get_material_type().get_properties().end();
           ++it) {
        const char *i_PropName = it->name.c_str();
        if (!p_Node["properties"][i_PropName]) {
          continue;
        }
        if (it->type ==
            Renderer::MaterialTypePropertyType::TEXTURE2D) {
          l_Handle.get_properties()[it->name] =
              Texture2D::deserialize(p_Node["properties"][i_PropName],
                                     l_Handle)
                  .get_id();
        } else if (it->type ==
                   Renderer::MaterialTypePropertyType::VECTOR4) {
          l_Handle.get_properties()[it->name] =
              Util::Serialization::deserialize_vector4(
                  p_Node["properties"][i_PropName]);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    Renderer::MaterialType Material::get_material_type() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:GETTER_material_type

      return TYPE_SOA(Material, material_type,
                      Renderer::MaterialType);
    }
    void Material::set_material_type(Renderer::MaterialType p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material_type
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material_type

      // Set new value
      TYPE_SOA(Material, material_type, Renderer::MaterialType) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_type
      get_properties().clear();
      // LOW_CODEGEN::END::CUSTOM:SETTER_material_type
    }

    Renderer::Material Material::get_renderer_material() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderer_material
      // LOW_CODEGEN::END::CUSTOM:GETTER_renderer_material

      return TYPE_SOA(Material, renderer_material,
                      Renderer::Material);
    }
    void Material::set_renderer_material(Renderer::Material p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_renderer_material
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_renderer_material

      // Set new value
      TYPE_SOA(Material, renderer_material, Renderer::Material) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderer_material
      // LOW_CODEGEN::END::CUSTOM:SETTER_renderer_material
    }

    Util::Map<Util::Name, Util::Variant> &
    Material::get_properties() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_properties
      // LOW_CODEGEN::END::CUSTOM:GETTER_properties

      return TYPE_SOA(
          Material, properties,
          SINGLE_ARG(Util::Map<Util::Name, Util::Variant>));
    }

    uint32_t Material::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(Material, reference_count, uint32_t);
    }
    void Material::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(Material, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count
    }

    Low::Util::UniqueId Material::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Material, unique_id, Low::Util::UniqueId);
    }
    void Material::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Material, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
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

    void Material::set_property(Util::Name p_Name,
                                Util::Variant &p_Value)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property
      if (is_loaded()) {
        uint8_t l_PropertyType =
            Renderer::MaterialTypePropertyType::VECTOR3;
        bool l_FoundProperty = false;
        for (auto it = get_material_type().get_properties().begin();
             it != get_material_type().get_properties().end(); ++it) {
          if (it->name == p_Name) {
            l_FoundProperty = true;
            l_PropertyType = it->type;
            break;
          }
        }
        LOW_ASSERT(l_FoundProperty,
                   "Could not find property in material type");

        if (l_PropertyType ==
            Renderer::MaterialTypePropertyType::TEXTURE2D) {
          Texture2D l_OldTexture = get_property(p_Name).m_Uint64;
          if (l_OldTexture.is_alive() && l_OldTexture.is_loaded()) {
            l_OldTexture.unload();
          }
          Texture2D l_Texture = p_Value.m_Uint64;
          if (l_Texture.is_alive()) {
            l_Texture.load();
          }
          if (l_Texture.is_alive()) {
            get_renderer_material().set_property(
                p_Name,
                Util::Variant::from_handle(
                    l_Texture.get_renderer_texture().get_id()));
          } else {
            get_renderer_material().set_property(
                p_Name, Util::Variant::from_handle(0));
          }
        } else {
          get_renderer_material().set_property(p_Name, p_Value);
        }
      }
      get_properties()[p_Name] = p_Value;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property
    }

    Util::Variant &Material::get_property(Util::Name p_Name)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property
      return get_properties()[p_Name];
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property
    }

    bool Material::is_loaded()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded
      return get_renderer_material().is_alive();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void Material::load()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load
      set_reference_count(get_reference_count() + 1);

      LOW_ASSERT(get_reference_count() > 0,
                 "Increased Texture2D reference count, but its "
                 "not over 0. "
                 "Something went wrong.");

      if (is_loaded()) {
        return;
      }
      set_renderer_material(
          Renderer::create_material(get_name(), get_material_type()));

      for (auto it = get_material_type().get_properties().begin();
           it != get_material_type().get_properties().end(); ++it) {
        if (it->type ==
            Renderer::MaterialTypePropertyType::TEXTURE2D) {
          Texture2D l_Texture = get_property(it->name).m_Uint64;
          if (l_Texture.is_alive()) {
            l_Texture.load();

            get_renderer_material().set_property(
                it->name, Util::Variant::from_handle(
                              l_Texture.get_renderer_texture()));
          }
        } else {
          get_renderer_material().set_property(
              it->name, get_property(it->name));
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
    }

    void Material::unload()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload
      set_reference_count(get_reference_count() - 1);

      LOW_ASSERT(get_reference_count() >= 0,
                 "Texture2D reference count < 0. Something "
                 "went wrong.");

      if (get_reference_count() <= 0) {
        _unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
    }

    void Material::_unload()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__unload
      for (auto it = get_material_type().get_properties().begin();
           it != get_material_type().get_properties().end(); ++it) {
        if (it->type ==
            Renderer::MaterialTypePropertyType::TEXTURE2D) {
          Texture2D l_Texture = get_property(it->name).m_Uint64;
          if (l_Texture.is_alive()) {
            l_Texture.unload();
          }
        }
      }

      if (get_renderer_material().is_alive()) {
        get_renderer_material().destroy();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__unload
    }

    uint32_t Material::create_instance()
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

    void Material::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(MaterialData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(MaterialData, material_type) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialData, material_type) *
                          (l_Capacity)],
               l_Capacity * sizeof(Renderer::MaterialType));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MaterialData, renderer_material) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MaterialData, renderer_material) *
                       (l_Capacity)],
            l_Capacity * sizeof(Renderer::Material));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(MaterialData, properties) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::Map<Util::Name,
                                             Util::Variant>))])
              Util::Map<Util::Name, Util::Variant>();
          *i_ValPtr = it->get_properties();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialData, reference_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialData, reference_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialData, unique_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialData, unique_id) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Util::UniqueId));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MaterialData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MaterialData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for Material from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Core
} // namespace Low
