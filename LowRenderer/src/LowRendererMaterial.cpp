#include "LowRendererMaterial.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowRendererTexture2D.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Material::TYPE_ID = 17;
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
                                 MaterialType)) MaterialType();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Material, context,
                                 Interface::Context))
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

      {
        Low::Util::HandleLock<Material> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

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

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(Material));

      ms_PageSize = ms_Capacity;
      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Material::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);
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
        l_PropertyInfo.handleType = MaterialType::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          l_Handle.get_material_type();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Material, material_type, MaterialType);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Material l_Handle = p_Handle.get_id();
          l_Handle.set_material_type(*(MaterialType *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((MaterialType *)p_Data) = l_Handle.get_material_type();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material_type
      }
      {
        // Property: context
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Material::Data, context);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Interface::Context::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: context
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
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
          l_ParameterInfo.name = N(p_Context);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Interface::Context::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: set_property
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_property);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_PropertyName);
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
      if (get_context().is_alive()) {
        l_Handle.set_context(get_context());
      }

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

    Low::Util::Handle
    Material::deserialize(Low::Util::Yaml::Node &p_Node,
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
        l_Handle.set_context(Interface::Context::deserialize(
                                 p_Node["context"], l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

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

    MaterialType Material::get_material_type() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material_type

      // LOW_CODEGEN::END::CUSTOM:GETTER_material_type

      return TYPE_SOA(Material, material_type, MaterialType);
    }
    void Material::set_material_type(MaterialType p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material_type

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material_type

      // Set new value
      TYPE_SOA(Material, material_type, MaterialType) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material_type

      // LOW_CODEGEN::END::CUSTOM:SETTER_material_type

      broadcast_observable(N(material_type));
    }

    Interface::Context Material::get_context() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context

      // LOW_CODEGEN::END::CUSTOM:GETTER_context

      return TYPE_SOA(Material, context, Interface::Context);
    }
    void Material::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_context

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_context

      // Set new value
      TYPE_SOA(Material, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context

      // LOW_CODEGEN::END::CUSTOM:SETTER_context

      broadcast_observable(N(context));
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

    Material Material::make(Util::Name p_Name,
                            Interface::Context p_Context)
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
           (LOW_RENDERER_MATERIAL_DATA_VECTORS *
            sizeof(Math::Vector4))));

      return l_Material;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void Material::set_property(Util::Name p_PropertyName,
                                Util::Variant &p_Value)
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
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
                i_Offset + (get_index() *
                            (LOW_RENDERER_MATERIAL_DATA_VECTORS *
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
                i_Offset + (get_index() *
                            (LOW_RENDERER_MATERIAL_DATA_VECTORS *
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
      LOW_ASSERT(l_FoundIndex, "Budget blown for type Material");
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

  } // namespace Renderer
} // namespace Low
