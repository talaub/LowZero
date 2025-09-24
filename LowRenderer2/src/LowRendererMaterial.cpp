#include "LowRendererMaterial.h"

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
#define SET_GPU_PROPERTY(type)                                       \
  LOCK_HANDLE(get_gpu());                                            \
  u32 l_Offset = get_material_type().get_input_offset(p_Name);       \
  mark_dirty();                                                      \
  if (get_gpu().is_alive()) {                                        \
    memcpy(&((u8 *)get_gpu().get_data())[l_Offset], &p_Value,        \
           sizeof(type));                                            \
  }

#include "LowRendererGlobals.h"
#include "LowRendererGpuTexture.h"
#include "LowRendererResourceManager.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    Util::List<PendingTextureBinding>
        Material::ms_PendingTextureBindings;

    Util::Mutex Material::ms_PendingTextureBindingsMutex;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Material::TYPE_ID = 59;
    uint32_t Material::ms_Capacity = 0u;
    uint32_t Material::ms_PageSize = 0u;
    Low::Util::SharedMutex Material::ms_LivingMutex;
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

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Material, state,
                                 MaterialState)) MaterialState();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Material, material_type,
                                 MaterialType)) MaterialType();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Material, resource,
                                 Low::Renderer::MaterialResource))
          Low::Renderer::MaterialResource();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Material, gpu,
                                 Low::Renderer::GpuMaterial))
          Low::Renderer::GpuMaterial();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, Material, properties,
          SINGLE_ARG(
              Low::Util::Map<Low::Util::Name, Low::Util::Variant>)))
          Low::Util::Map<Low::Util::Name, Low::Util::Variant>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Material, references,
                                 Low::Util::Set<u64>))
          Low::Util::Set<u64>();
      ACCESSOR_TYPE_SOA(l_Handle, Material, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      if (p_UniqueId > 0ull) {
        l_Handle.set_unique_id(p_UniqueId);
      } else {
        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
      }
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_state(MaterialState::MEMORYLOADED);

      ResourceManager::register_asset(l_Handle.get_unique_id(),
                                      l_Handle);
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
      Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
      l_LivingLock.unlock();
    }

    void Material::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(Material));

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
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Material::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            MaterialStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Material, state,
                                            MaterialState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Material l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(MaterialState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((MaterialState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
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
                                const void *p_Data) -> void {};
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
        // Property: resource
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Material::Data, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::MaterialResource::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          l_Handle.get_resource();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Material, resource,
              Low::Renderer::MaterialResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Material l_Handle = p_Handle.get_id();
          l_Handle.set_resource(
              *(Low::Renderer::MaterialResource *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((Low::Renderer::MaterialResource *)p_Data) =
              l_Handle.get_resource();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource
      }
      {
        // Property: gpu
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gpu);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Material::Data, gpu);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::GpuMaterial::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          l_Handle.get_gpu();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Material, gpu, Low::Renderer::GpuMaterial);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Material l_Handle = p_Handle.get_id();
          l_Handle.set_gpu(*(Low::Renderer::GpuMaterial *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((Low::Renderer::GpuMaterial *)p_Data) =
              l_Handle.get_gpu();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gpu
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
              SINGLE_ARG(Low::Util::Map<Low::Util::Name,
                                        Low::Util::Variant>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Material l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Material> l_HandleLock(l_Handle);
          *((Low::Util::Map<Low::Util::Name, Low::Util::Variant> *)
                p_Data) = l_Handle.get_properties();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: properties
      }
      {
        // Property: references
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(references);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Material::Data, references);
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
        // End property: references
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
        // Function: make_gpu_ready
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_gpu_ready);
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
        // End function: make_gpu_ready
      }
      {
        // Function: update_gpu
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(update_gpu);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: update_gpu
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
        // Function: set_property_texture
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_property_texture);
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
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::Texture::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_property_texture
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
      {
        // Function: get_property_texture
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_property_texture);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_property_texture
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

      Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
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
      l_Handle.set_state(get_state());
      if (get_material_type().is_alive()) {
        l_Handle.set_material_type(get_material_type());
      }
      if (get_resource().is_alive()) {
        l_Handle.set_resource(get_resource());
      }
      if (get_gpu().is_alive()) {
        l_Handle.set_gpu(get_gpu());
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

      p_Node["_unique_id"] =
          Low::Util::hash_to_string(get_unique_id()).c_str();
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
        case MaterialTypeInputType::TEXTURE: {
          Texture i_Texture = get_property_texture(i_Name);
          i_Node = 0;
          if (i_Texture.is_alive() &&
              i_Texture.get_resource().is_alive()) {
            i_Node = Util::hash_to_string(
                         i_Texture.get_resource().get_texture_id())
                         .c_str();
          }
          break;
        }
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
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            LOW_YAML_AS_STRING(p_Node["_unique_id"]));
      }

      Material l_Handle =
          Material::make(N(Material), l_HandleUniqueId);

      if (p_Node["references"]) {
      }
      if (p_Node["unique_id"]) {
        l_Handle.set_unique_id(
            p_Node["unique_id"].as<Low::Util::UniqueId>());
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
          case MaterialTypeInputType::TEXTURE: {
            const u64 l_AssetId =
                Util::string_to_hash(LOW_YAML_AS_STRING(i_Node));
            Texture i_Texture =
                ResourceManager::find_asset<Texture>(l_AssetId);

            l_Handle.set_property_texture(i_Name, i_Texture);
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

    void Material::reference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      Low::Util::HandleLock<Material> l_HandleLock(get_id());
      const u32 l_OldReferences =
          (TYPE_SOA(Material, references, Low::Util::Set<u64>))
              .size();

      (TYPE_SOA(Material, references, Low::Util::Set<u64>))
          .insert(p_Id);

      const u32 l_References =
          (TYPE_SOA(Material, references, Low::Util::Set<u64>))
              .size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:NEW_REFERENCE
        ResourceManager::load_material(get_id());
        // LOW_CODEGEN::END::CUSTOM:NEW_REFERENCE
      }
    }

    void Material::dereference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      Low::Util::HandleLock<Material> l_HandleLock(get_id());
      const u32 l_OldReferences =
          (TYPE_SOA(Material, references, Low::Util::Set<u64>))
              .size();

      (TYPE_SOA(Material, references, Low::Util::Set<u64>))
          .erase(p_Id);

      const u32 l_References =
          (TYPE_SOA(Material, references, Low::Util::Set<u64>))
              .size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:REFERENCE_REMOVED
        // LOW_CODEGEN::END::CUSTOM:REFERENCE_REMOVED
      }
    }

    u32 Material::references() const
    {
      return get_references().size();
    }

    MaterialState Material::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(Material, state, MaterialState);
    }
    void Material::set_state(MaterialState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(Material, state, MaterialState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
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

    Low::Renderer::MaterialResource Material::get_resource() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      return TYPE_SOA(Material, resource,
                      Low::Renderer::MaterialResource);
    }
    void
    Material::set_resource(Low::Renderer::MaterialResource p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      TYPE_SOA(Material, resource, Low::Renderer::MaterialResource) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    Low::Renderer::GpuMaterial Material::get_gpu() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:GETTER_gpu

      return TYPE_SOA(Material, gpu, Low::Renderer::GpuMaterial);
    }
    void Material::set_gpu(Low::Renderer::GpuMaterial p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gpu
      LOCK_HANDLE(p_Value);
      if (p_Value.is_alive()) {
        p_Value.set_material_handle(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gpu

      // Set new value
      TYPE_SOA(Material, gpu, Low::Renderer::GpuMaterial) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:SETTER_gpu

      broadcast_observable(N(gpu));
    }

    Low::Util::Map<Low::Util::Name, Low::Util::Variant> &
    Material::get_properties() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_properties
      // LOW_CODEGEN::END::CUSTOM:GETTER_properties

      return TYPE_SOA(
          Material, properties,
          SINGLE_ARG(
              Low::Util::Map<Low::Util::Name, Low::Util::Variant>));
    }

    Low::Util::Set<u64> &Material::get_references() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Material> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
      // LOW_CODEGEN::END::CUSTOM:GETTER_references

      return TYPE_SOA(Material, references, Low::Util::Set<u64>);
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

    void Material::mark_dirty()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
      LOCK_HANDLE(*this);
      LOCK_HANDLE(get_gpu());

      if (get_gpu().is_alive()) {
        get_gpu().mark_dirty();
      }
      // LOW_CODEGEN::END::CUSTOM:MARK_dirty
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

    Material
    Material::make(Low::Util::Name p_Name,
                   Low::Renderer::MaterialType p_MaterialType)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      LOW_ASSERT(p_MaterialType.is_alive(),
                 "Cannot create material from dead type");

      Material l_Material = make(p_Name);
      l_Material.set_material_type(p_MaterialType);

      {
        Util::List<Util::Name> l_Names;
        p_MaterialType.fill_input_names(l_Names);

        for (u32 i = 0; i < l_Names.size(); ++i) {
          Util::Name i_Name = l_Names[i];

          if (p_MaterialType.get_input_type(i_Name) ==
              MaterialTypeInputType::TEXTURE) {
            l_Material.set_property_texture(i_Name,
                                            Util::Handle::DEAD);
          }
        }
      }

      return l_Material;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    Material Material::make_gpu_ready(
        Low::Util::Name p_Name,
        Low::Renderer::MaterialType p_MaterialType)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_gpu_ready
      Material l_Material = make(p_Name, p_MaterialType);

      LOW_ASSERT(GpuMaterial::living_count() <
                     GpuMaterial::get_capacity(),
                 "GpuMaterial capacity blown, we cannot set up a new "
                 "gpu ready material.");

      l_Material.set_gpu(GpuMaterial::make(p_Name));

      l_Material.update_gpu();

      l_Material.set_state(MaterialState::UPLOADINGTOGPU);

      return l_Material;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_gpu_ready
    }

    void Material::update_gpu()
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_gpu
      LOCK_HANDLE(get_gpu());
      LOCK_HANDLE(get_material_type());
      _LOW_ASSERT(get_gpu().is_alive());

#define SET_GPU_PROPERTY_ITER(type)                                  \
  {                                                                  \
    u32 i_Offset = l_MaterialType.get_input_offset(*it);             \
    type i_Value = (type)get_properties()[*it];                      \
    memcpy(&((u8 *)get_gpu().get_data())[i_Offset], &i_Value,        \
           sizeof(type));                                            \
  }

      MaterialType l_MaterialType = get_material_type();

      Util::List<Util::Name> l_InputNames;
      l_MaterialType.fill_input_names(l_InputNames);

      for (auto it = l_InputNames.begin(); it != l_InputNames.end();
           ++it) {
        switch (l_MaterialType.get_input_type(*it)) {
        case MaterialTypeInputType::TEXTURE: {
          set_property_texture(*it, get_property_texture(*it));
          break;
        }
        case MaterialTypeInputType::FLOAT: {
          SET_GPU_PROPERTY_ITER(float);
          break;
        }
        case MaterialTypeInputType::VECTOR2: {
          SET_GPU_PROPERTY_ITER(Math::Vector2);
          break;
        }
        case MaterialTypeInputType::VECTOR3: {
          SET_GPU_PROPERTY_ITER(Math::Vector3);
          break;
        }
        case MaterialTypeInputType::VECTOR4: {
          SET_GPU_PROPERTY_ITER(Math::Vector4);
          break;
        }
        default: {
          LOW_ASSERT(false, "Unsupported material input type.");
        }
        }
      }

#undef SET_GPU_PROPERTY_ITER

      mark_dirty();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_gpu
    }

    void Material::set_property_vector4(Util::Name p_Name,
                                        Math::Vector4 &p_Value)
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_vector4
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR4);
      get_properties()[p_Name] = p_Value;

      SET_GPU_PROPERTY(Math::Vector4);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_vector4
    }

    void Material::set_property_vector3(Util::Name p_Name,
                                        Math::Vector3 &p_Value)
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_vector3
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR3);
      get_properties()[p_Name] = p_Value;

      SET_GPU_PROPERTY(Math::Vector3);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_vector3
    }

    void Material::set_property_vector2(Util::Name p_Name,
                                        Math::Vector2 &p_Value)
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_vector2
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR2);
      get_properties()[p_Name] = p_Value;

      SET_GPU_PROPERTY(Math::Vector2);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_vector2
    }

    void Material::set_property_float(Util::Name p_Name,
                                      float p_Value)
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_float
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::FLOAT);
      get_properties()[p_Name] = p_Value;

      SET_GPU_PROPERTY(float);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_float
    }

    void Material::set_property_u32(Util::Name p_Name,
                                    uint32_t p_Value)
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_u32
      _LOW_ASSERT(false);
      get_properties()[p_Name] = p_Value;

      SET_GPU_PROPERTY(u32);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_u32
    }

    void
    Material::set_property_texture(Util::Name p_Name,
                                   Low::Renderer::Texture p_Value)
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_property_texture
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::TEXTURE);

      LOCK_HANDLE(get_gpu());

      Texture l_OldTexture = get_property_texture(p_Name);

      get_properties()[p_Name].set_handle(p_Value.get_id());

      if (get_gpu().is_alive()) {
        if (l_OldTexture != p_Value ||
            get_state() != MaterialState::LOADED) {
          if (l_OldTexture.is_alive()) {
            l_OldTexture.dereference(get_id());
          }

          if (p_Value.is_alive()) {
            p_Value.reference(get_id());
          }
        }

        u32 l_Index = GpuTexture::get_capacity() + 5;

        for (auto it = ms_PendingTextureBindings.begin();
             it != ms_PendingTextureBindings.end();) {
          if (it->propertyName == p_Name &&
              it->material.get_id() == get_id()) {
            it = ms_PendingTextureBindings.erase(it);
          } else {
            ++it;
          }
        }

        if (p_Value.is_alive() &&
            p_Value.get_state() == TextureState::LOADED) {
          l_Index = p_Value.get_gpu().get_index();
        } else if (p_Value.is_alive()) {
          PendingTextureBinding l_Binding;
          l_Binding.propertyName = p_Name;
          l_Binding.material = get_id();
          l_Binding.texture = p_Value;
          ms_PendingTextureBindings.push_back(l_Binding);
        }

        {
          const float l_FloatIndex = l_Index;

          const u32 l_Offset =
              get_material_type().get_input_offset(p_Name);
          mark_dirty();
          memcpy(&((u8 *)get_gpu().get_data())[l_Offset],
                 &l_FloatIndex, sizeof(float));
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_property_texture
    }

    Low::Math::Vector4 &
    Material::get_property_vector4(Util::Name p_Name) const
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_vector4
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR4);
      return (Math::Vector4)get_properties()[p_Name];
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_vector4
    }

    Low::Math::Vector3 &
    Material::get_property_vector3(Util::Name p_Name) const
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_vector3
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR3);
      return (Math::Vector3)get_properties()[p_Name];
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_vector3
    }

    Low::Math::Vector2 &
    Material::get_property_vector2(Util::Name p_Name) const
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_vector2
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::VECTOR2);
      return (Math::Vector2)get_properties()[p_Name];
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_vector2
    }

    float Material::get_property_float(Util::Name p_Name) const
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_float
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::FLOAT);
      return (float)get_properties()[p_Name];
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_float
    }

    uint32_t Material::get_property_u32(Util::Name p_Name) const
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_u32
      _LOW_ASSERT(false);
      //_LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
      // MaterialTypeInputType::);

      LOW_NOT_IMPLEMENTED;

      return 0ull;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_u32
    }

    Low::Renderer::Texture
    Material::get_property_texture(Util::Name p_Name) const
    {
      Low::Util::HandleLock<Material> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_property_texture
      _LOW_ASSERT(get_material_type().get_input_type(p_Name) ==
                  MaterialTypeInputType::TEXTURE);

      return get_properties()[p_Name].as_u64();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_property_texture
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

  } // namespace Renderer
} // namespace Low
