#include "LowCoreMaterial.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowRenderer.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Material::TYPE_ID = 24;
    uint32_t Material::ms_Capacity = 0u;
    uint32_t Material::ms_PageSize = 0u;
    Low::Util::SharedMutex Material::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Material::ms_PagesLock(Material::ms_PagesMutex,
                               std::defer_lock);
    Low::Util::List<Material> Material::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Material::ms_Pages;

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
      return make(p_Name, 0ull);
    }

    Material Material::make(Low::Util::Name p_Name,
                            Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Material l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Material::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Material> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Material, material_type,
                                 Renderer::MaterialType))
          Renderer::MaterialType();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, Material, renderer_material, Renderer::Material))
          Renderer::Material();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, Material, properties,
          SINGLE_ARG(Util::Map<Util::Name, Util::Variant>)))
          Util::Map<Util::Name, Util::Variant>();
      ACCESSOR_TYPE_SOA(l_Handle, Material, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

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

      {
        Low::Util::HandleLock<Material> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        _unload();
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      Low::Util::remove_unique_id(get_unique_id());

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Page->slots[l_SlotIndex].m_Occupied = false;
      l_Page->slots[l_SlotIndex].m_Generation++;

      ms_PagesLock.lock();
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
    }

    void Material::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(Material));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Material::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Material);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Material::is_alive;
      l_TypeInfo.destroy = &Material::destroy;
      l_TypeInfo.serialize = &Material::serialize;
      l_TypeInfo.deserialize = &Material::deserialize;
      l_TypeInfo.find_by_index = &Material::_find_by_index;
      l_TypeInfo.notify = &Material::_notify;
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
            offsetof(Material::Data, material_type);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Renderer::MaterialType::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
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
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((Renderer::MaterialType *)p_Data) =
              l_Handle.get_material_type();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material_type
      }
      {
        // Property: renderer_material
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(renderer_material);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Material::Data, renderer_material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Renderer::Material::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          l_Handle.get_renderer_material();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Material,
                                            renderer_material,
                                            Renderer::Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((Renderer::Material *)p_Data) =
              l_Handle.get_renderer_material();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: renderer_material
      }
      {
        // Property: properties
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(properties);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Material::Data, properties);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          l_Handle.get_properties();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Material, properties,
              SINGLE_ARG(Util::Map<Util::Name, Util::Variant>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((Util::Map<Util::Name, Util::Variant> *)p_Data) =
              l_Handle.get_properties();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: properties
      }
      {
        // Property: reference_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Material::Data, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
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
        // End property: reference_count
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Material::Data, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Material, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((Low::Util::UniqueId *)p_Data) = l_Handle.get_unique_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: unique_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(Material::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: set_property
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_property);
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
        // End function: set_property
      }
      {
        // Function: get_property
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_property);
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
        // End function: get_property
      }
      {
        // Function: is_loaded
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_loaded);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: is_loaded
      }
      {
        // Function: load
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: load
      }
      {
        // Function: unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: unload
      }
      {
        // Function: _unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: _unload
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Material::cleanup()
    {
      Low::Util::List<Material> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      ms_PagesLock.lock();
      for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
        Low::Util::Instances::Page *i_Page = *it;
        free(i_Page->buffer);
        free(i_Page->slots);
        free(i_Page->lockWords);
        delete i_Page;
        it = ms_Pages.erase(it);
      }

      ms_Capacity = 0;

      ms_PagesLock.unlock();
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
      l_Handle.m_Data.m_Type = Material::TYPE_ID;

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
        l_Handle.m_Data.m_Generation = 0;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Handle.m_Data.m_Generation =
          l_Page->slots[l_SlotIndex].m_Generation;

      return l_Handle;
    }

    Material Material::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Material l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Material::TYPE_ID;

      return l_Handle;
    }

    bool Material::is_alive() const
    {
      if (m_Data.m_Type != Material::TYPE_ID) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      return m_Data.m_Type == Material::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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
      p_Node["_unique_id"] =
          Low::Util::hash_to_string(get_unique_id()).c_str();
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
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            LOW_YAML_AS_STRING(p_Node["_unique_id"]));
      }

      Material l_Handle =
          Material::make(N(Material), l_HandleUniqueId);

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

    void
    Material::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Material::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Material::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Material::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Material::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
    {
      Material l_Material = p_Observer.get_id();
      l_Material.notify(p_Observed, p_Observable);
    }

    Renderer::MaterialType Material::get_material_type() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material_type

      // LOW_CODEGEN::END::CUSTOM:GETTER_material_type

      return TYPE_SOA(Material, material_type,
                      Renderer::MaterialType);
    }
    void Material::set_material_type(Renderer::MaterialType p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material_type

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material_type

      // Set new value
      TYPE_SOA(Material, material_type, Renderer::MaterialType) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_type

      get_properties().clear();
      // LOW_CODEGEN::END::CUSTOM:SETTER_material_type

      broadcast_observable(N(material_type));
    }

    Renderer::Material Material::get_renderer_material() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderer_material

      // LOW_CODEGEN::END::CUSTOM:GETTER_renderer_material

      return TYPE_SOA(Material, renderer_material,
                      Renderer::Material);
    }
    void Material::set_renderer_material(Renderer::Material p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_renderer_material

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_renderer_material

      // Set new value
      TYPE_SOA(Material, renderer_material, Renderer::Material) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderer_material

      // LOW_CODEGEN::END::CUSTOM:SETTER_renderer_material

      broadcast_observable(N(renderer_material));
    }

    Util::Map<Util::Name, Util::Variant> &
    Material::get_properties() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_properties

      // LOW_CODEGEN::END::CUSTOM:GETTER_properties

      return TYPE_SOA(
          Material, properties,
          SINGLE_ARG(Util::Map<Util::Name, Util::Variant>));
    }

    uint32_t Material::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(Material, reference_count, uint32_t);
    }
    void Material::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(Material, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count

      broadcast_observable(N(reference_count));
    }

    Low::Util::UniqueId Material::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Material, unique_id, Low::Util::UniqueId);
    }
    void Material::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Material, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name Material::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Material, name, Low::Util::Name);
    }
    void Material::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Material, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    void Material::set_property(Util::Name p_Name,
                                Util::Variant &p_Value)
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
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
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property

      return get_properties()[p_Name];
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property
    }

    bool Material::is_loaded()
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded

      return get_renderer_material().is_alive();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void Material::load()
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
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
          } else {
            get_renderer_material().set_property(
                it->name, Util::Variant::from_handle(0ull));
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
      Low::Util::HandleLock<Material> l_Lock(get_id());
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
      Low::Util::HandleLock<Material> l_Lock(get_id());
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

    uint32_t Material::create_instance(
        u32 &p_PageIndex, u32 &p_SlotIndex,
        Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      u32 l_Index = 0;
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      bool l_FoundIndex = false;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

      for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
           ++l_PageIndex) {
        Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
            ms_Pages[l_PageIndex]->mutex);
        for (l_SlotIndex = 0;
             l_SlotIndex < ms_Pages[l_PageIndex]->size;
             ++l_SlotIndex) {
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
            l_FoundIndex = true;
            l_PageLock = std::move(i_PageLock);
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
        Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
            ms_Pages[l_PageIndex]->mutex);
        l_PageLock = std::move(l_NewLock);
      }
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      p_PageLock = std::move(l_PageLock);
      LOCK_UNLOCK(l_PagesLock);
      return l_Index;
    }

    u32 Material::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Material.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Material::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Material::get_page_for_index(const u32 p_Index,
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

  } // namespace Core
} // namespace Low
