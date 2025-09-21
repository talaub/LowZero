#include "LowRendererMaterialType.h"

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

    const uint16_t MaterialType::TYPE_ID = 58;
    uint32_t MaterialType::ms_Capacity = 0u;
    uint32_t MaterialType::ms_PageSize = 0u;
    Low::Util::SharedMutex MaterialType::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        MaterialType::ms_PagesLock(MaterialType::ms_PagesMutex,
                                   std::defer_lock);
    Low::Util::List<MaterialType> MaterialType::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        MaterialType::ms_Pages;

    MaterialType::MaterialType() : Low::Util::Handle(0ull)
    {
    }
    MaterialType::MaterialType(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    MaterialType::MaterialType(MaterialType &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle MaterialType::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    MaterialType MaterialType::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);

      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, transparent, bool) =
          false;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MaterialType, inputs,
                                 Util::List<MaterialTypeInput>))
          Util::List<MaterialTypeInput>();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, initialized, bool) =
          false;
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, MaterialType, draw_pipeline_config,
          Low::Renderer::GraphicsPipelineConfig))
          Low::Renderer::GraphicsPipelineConfig();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, MaterialType, depth_pipeline_config,
          Low::Renderer::GraphicsPipelineConfig))
          Low::Renderer::GraphicsPipelineConfig();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MaterialType, family,
                                 Low::Renderer::MaterialTypeFamily))
          Low::Renderer::MaterialTypeFamily();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      {
        l_Handle.get_draw_pipeline_config().vertexShaderPath = "";
        l_Handle.get_draw_pipeline_config().fragmentShaderPath = "";
        l_Handle.get_draw_pipeline_config().depthTest = true;
        l_Handle.get_draw_pipeline_config().cullMode =
            GraphicsPipelineCullMode::BACK;
        l_Handle.get_draw_pipeline_config().frontFace =
            GraphicsPipelineFrontFace::COUNTER_CLOCKWISE;

        l_Handle.get_draw_pipeline_config().wireframe = false;
        // l_Handle.get_draw_pipeline_config().depthFormat
      }
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MaterialType::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<MaterialType> l_Lock(get_id());
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

    void MaterialType::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(MaterialType));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, MaterialType::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MaterialType);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MaterialType::is_alive;
      l_TypeInfo.destroy = &MaterialType::destroy;
      l_TypeInfo.serialize = &MaterialType::serialize;
      l_TypeInfo.deserialize = &MaterialType::deserialize;
      l_TypeInfo.find_by_index = &MaterialType::_find_by_index;
      l_TypeInfo.notify = &MaterialType::_notify;
      l_TypeInfo.find_by_name = &MaterialType::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &MaterialType::_make;
      l_TypeInfo.duplicate_default = &MaterialType::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MaterialType::living_instances);
      l_TypeInfo.get_living_count = &MaterialType::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: transparent
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(transparent);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, transparent);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          l_Handle.is_transparent();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType,
                                            transparent, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_transparent(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_transparent();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: transparent
      }
      {
        // Property: draw_pipeline_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_pipeline_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, draw_pipeline_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          l_Handle.get_draw_pipeline_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MaterialType, draw_pipeline_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_draw_pipeline_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          *((uint64_t *)p_Data) = l_Handle.get_draw_pipeline_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_pipeline_handle
      }
      {
        // Property: depth_pipeline_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_pipeline_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, depth_pipeline_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          l_Handle.get_depth_pipeline_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType,
                                            depth_pipeline_handle,
                                            uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_depth_pipeline_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          *((uint64_t *)p_Data) =
              l_Handle.get_depth_pipeline_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: depth_pipeline_handle
      }
      {
        // Property: inputs
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(inputs);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, inputs);
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
        // End property: inputs
      }
      {
        // Property: initialized
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(initialized);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, initialized);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          l_Handle.is_initialized();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType,
                                            initialized, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_initialized(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_initialized();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: initialized
      }
      {
        // Property: draw_pipeline_config
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_pipeline_config);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, draw_pipeline_config);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          l_Handle.get_draw_pipeline_config();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MaterialType, draw_pipeline_config,
              Low::Renderer::GraphicsPipelineConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          *((Low::Renderer::GraphicsPipelineConfig *)p_Data) =
              l_Handle.get_draw_pipeline_config();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_pipeline_config
      }
      {
        // Property: depth_pipeline_config
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(depth_pipeline_config);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, depth_pipeline_config);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          l_Handle.get_depth_pipeline_config();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MaterialType, depth_pipeline_config,
              Low::Renderer::GraphicsPipelineConfig);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          *((Low::Renderer::GraphicsPipelineConfig *)p_Data) =
              l_Handle.get_depth_pipeline_config();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: depth_pipeline_config
      }
      {
        // Property: family
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(family);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, family);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType = Low::Renderer::
            MaterialTypeFamilyEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          l_Handle.get_family();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MaterialType, family,
              Low::Renderer::MaterialTypeFamily);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          *((Low::Renderer::MaterialTypeFamily *)p_Data) =
              l_Handle.get_family();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: family
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MaterialType::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MaterialType,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MaterialType l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MaterialType l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MaterialType> l_HandleLock(l_Handle);
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
        l_FunctionInfo.handleType =
            Low::Renderer::MaterialType::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Family);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::ENUM;
          l_ParameterInfo.handleType = Low::Renderer::
              MaterialTypeFamilyEnumHelper::get_enum_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: calculate_offsets
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(calculate_offsets);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: calculate_offsets
      }
      {
        // Function: get_input
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_input);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
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
          l_ParameterInfo.name = N(p_Input);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_input
      }
      {
        // Function: finalize
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(finalize);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: finalize
      }
      {
        // Function: add_input
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(add_input);
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
          l_ParameterInfo.name = N(p_Type);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: add_input
      }
      {
        // Function: has_input
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(has_input);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: has_input
      }
      {
        // Function: get_input_offset
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_input_offset);
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
        // End function: get_input_offset
      }
      {
        // Function: get_input_type
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_input_type);
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
        // End function: get_input_type
      }
      {
        // Function: fill_input_names
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(fill_input_names);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_InputNames);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: fill_input_names
      }
      {
        // Function: set_draw_vertex_shader_path
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_draw_vertex_shader_path);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_draw_vertex_shader_path
      }
      {
        // Function: set_draw_fragment_shader_path
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_draw_fragment_shader_path);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_draw_fragment_shader_path
      }
      {
        // Function: set_depth_vertex_shader_path
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_depth_vertex_shader_path);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_depth_vertex_shader_path
      }
      {
        // Function: set_depth_fragment_shader_path
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_depth_fragment_shader_path);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_depth_fragment_shader_path
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MaterialType::cleanup()
    {
      Low::Util::List<MaterialType> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle MaterialType::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    MaterialType MaterialType::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

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

    MaterialType MaterialType::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      return l_Handle;
    }

    bool MaterialType::is_alive() const
    {
      if (m_Data.m_Type != MaterialType::TYPE_ID) {
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
      return m_Data.m_Type == MaterialType::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t MaterialType::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    MaterialType::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    MaterialType MaterialType::find_by_name(Low::Util::Name p_Name)
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

    MaterialType MaterialType::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      MaterialType l_Handle = make(p_Name);
      l_Handle.set_transparent(is_transparent());
      l_Handle.set_draw_pipeline_handle(get_draw_pipeline_handle());
      l_Handle.set_depth_pipeline_handle(get_depth_pipeline_handle());
      l_Handle.set_initialized(is_initialized());
      l_Handle.set_family(get_family());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    MaterialType MaterialType::duplicate(MaterialType p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    MaterialType::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      MaterialType l_MaterialType = p_Handle.get_id();
      return l_MaterialType.duplicate(p_Name);
    }

    void MaterialType::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["transparent"] = is_transparent();
      p_Node["draw_pipeline_handle"] = get_draw_pipeline_handle();
      p_Node["depth_pipeline_handle"] = get_depth_pipeline_handle();
      p_Node["initialized"] = is_initialized();
      Low::Util::Serialization::serialize_enum(
          p_Node["family"],
          Low::Renderer::MaterialTypeFamilyEnumHelper::get_enum_id(),
          static_cast<uint8_t>(get_family()));
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void MaterialType::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      MaterialType l_MaterialType = p_Handle.get_id();
      l_MaterialType.serialize(p_Node);
    }

    Low::Util::Handle
    MaterialType::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {
      MaterialType l_Handle = MaterialType::make(N(MaterialType));

      if (p_Node["transparent"]) {
        l_Handle.set_transparent(p_Node["transparent"].as<bool>());
      }
      if (p_Node["draw_pipeline_handle"]) {
        l_Handle.set_draw_pipeline_handle(
            p_Node["draw_pipeline_handle"].as<uint64_t>());
      }
      if (p_Node["depth_pipeline_handle"]) {
        l_Handle.set_depth_pipeline_handle(
            p_Node["depth_pipeline_handle"].as<uint64_t>());
      }
      if (p_Node["inputs"]) {
      }
      if (p_Node["initialized"]) {
        l_Handle.set_initialized(p_Node["initialized"].as<bool>());
      }
      if (p_Node["draw_pipeline_config"]) {
      }
      if (p_Node["depth_pipeline_config"]) {
      }
      if (p_Node["family"]) {
        l_Handle.set_family(
            static_cast<Low::Renderer::MaterialTypeFamily>(
                Low::Util::Serialization::deserialize_enum(
                    p_Node["family"])));
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void MaterialType::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 MaterialType::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 MaterialType::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void MaterialType::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void MaterialType::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      MaterialType l_MaterialType = p_Observer.get_id();
      l_MaterialType.notify(p_Observed, p_Observable);
    }

    bool MaterialType::is_transparent() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_transparent
      // LOW_CODEGEN::END::CUSTOM:GETTER_transparent

      return TYPE_SOA(MaterialType, transparent, bool);
    }
    void MaterialType::toggle_transparent()
    {
      set_transparent(!is_transparent());
    }

    void MaterialType::set_transparent(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_transparent
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_transparent

      // Set new value
      TYPE_SOA(MaterialType, transparent, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_transparent
      // LOW_CODEGEN::END::CUSTOM:SETTER_transparent

      broadcast_observable(N(transparent));
    }

    uint64_t MaterialType::get_draw_pipeline_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_pipeline_handle

      return TYPE_SOA(MaterialType, draw_pipeline_handle, uint64_t);
    }
    void MaterialType::set_draw_pipeline_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_draw_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_draw_pipeline_handle

      // Set new value
      TYPE_SOA(MaterialType, draw_pipeline_handle, uint64_t) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_draw_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_draw_pipeline_handle

      broadcast_observable(N(draw_pipeline_handle));
    }

    uint64_t MaterialType::get_depth_pipeline_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_pipeline_handle

      return TYPE_SOA(MaterialType, depth_pipeline_handle, uint64_t);
    }
    void MaterialType::set_depth_pipeline_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_pipeline_handle

      // Set new value
      TYPE_SOA(MaterialType, depth_pipeline_handle, uint64_t) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_pipeline_handle

      broadcast_observable(N(depth_pipeline_handle));
    }

    Util::List<MaterialTypeInput> &MaterialType::get_inputs() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_inputs
      // LOW_CODEGEN::END::CUSTOM:GETTER_inputs

      return TYPE_SOA(MaterialType, inputs,
                      Util::List<MaterialTypeInput>);
    }

    bool MaterialType::is_initialized() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_initialized
      // LOW_CODEGEN::END::CUSTOM:GETTER_initialized

      return TYPE_SOA(MaterialType, initialized, bool);
    }
    void MaterialType::toggle_initialized()
    {
      set_initialized(!is_initialized());
    }

    void MaterialType::set_initialized(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

      // Set new value
      TYPE_SOA(MaterialType, initialized, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized
      // LOW_CODEGEN::END::CUSTOM:SETTER_initialized

      broadcast_observable(N(initialized));
    }

    Low::Renderer::GraphicsPipelineConfig &
    MaterialType::get_draw_pipeline_config() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_pipeline_config
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_pipeline_config

      return TYPE_SOA(MaterialType, draw_pipeline_config,
                      Low::Renderer::GraphicsPipelineConfig);
    }

    Low::Renderer::GraphicsPipelineConfig &
    MaterialType::get_depth_pipeline_config() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_pipeline_config
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_pipeline_config

      return TYPE_SOA(MaterialType, depth_pipeline_config,
                      Low::Renderer::GraphicsPipelineConfig);
    }

    Low::Renderer::MaterialTypeFamily MaterialType::get_family() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_family
      // LOW_CODEGEN::END::CUSTOM:GETTER_family

      return TYPE_SOA(MaterialType, family,
                      Low::Renderer::MaterialTypeFamily);
    }
    void MaterialType::set_family(
        Low::Renderer::MaterialTypeFamily p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_family
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_family

      // Set new value
      TYPE_SOA(MaterialType, family,
               Low::Renderer::MaterialTypeFamily) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_family
      // LOW_CODEGEN::END::CUSTOM:SETTER_family

      broadcast_observable(N(family));
    }

    Low::Util::Name MaterialType::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(MaterialType, name, Low::Util::Name);
    }
    void MaterialType::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(MaterialType, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Low::Renderer::MaterialType
    MaterialType::make(Low::Util::Name p_Name,
                       Low::Renderer::MaterialTypeFamily p_Family)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      MaterialType l_Handle = make(p_Name);
      l_Handle.set_family(p_Family);

      if (p_Family == MaterialTypeFamily::SOLID) {
        l_Handle.get_draw_pipeline_config()
            .colorAttachmentFormats.push_back(
                ImageFormat::RGBA16_SFLOAT);
        l_Handle.get_draw_pipeline_config()
            .colorAttachmentFormats.push_back(
                ImageFormat::RGBA16_SFLOAT);
        l_Handle.get_draw_pipeline_config()
            .colorAttachmentFormats.push_back(
                ImageFormat::RGBA16_SFLOAT);
        l_Handle.get_draw_pipeline_config()
            .colorAttachmentFormats.push_back(ImageFormat::R32_UINT);
        l_Handle.get_draw_pipeline_config().depthFormat =
            ImageFormat::DEPTH;

        l_Handle.get_draw_pipeline_config().alphaBlending = false;
      } else if (p_Family == MaterialTypeFamily::UI) {
        l_Handle.get_draw_pipeline_config()
            .colorAttachmentFormats.push_back(
                ImageFormat::RGBA16_SFLOAT);

        l_Handle.get_draw_pipeline_config().alphaBlending = true;
        l_Handle.get_draw_pipeline_config().depthTest = false;
      } else if (p_Family == MaterialTypeFamily::DEBUGGEOMETRY) {
        l_Handle.get_draw_pipeline_config()
            .colorAttachmentFormats.push_back(
                ImageFormat::RGBA16_SFLOAT);

        l_Handle.get_draw_pipeline_config().alphaBlending = true;
        l_Handle.get_draw_pipeline_config().depthTest = true;
        l_Handle.get_draw_pipeline_config().depthFormat =
            ImageFormat::DEPTH;
      }

      return l_Handle;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void MaterialType::calculate_offsets()
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_calculate_offsets
      LOW_ASSERT(!is_initialized(),
                 "Cannot calculate offsets for a material type that "
                 "has already been initialized");

      Util::List<u32> l_FreeSingles;
      u32 l_Offset = 0;

      Util::List<MaterialTypeInput> &l_Inputs = get_inputs();

      // Assign all vector4
      for (auto it = l_Inputs.begin(); it != l_Inputs.end(); ++it) {
        if (it->type == MaterialTypeInputType::VECTOR4) {
          it->offset = l_Offset;
          l_Offset += sizeof(Math::Vector4);
        }
      }

      u32 l_Vector2Count = 0;
      // Assign all vector2
      for (auto it = l_Inputs.begin(); it != l_Inputs.end(); ++it) {
        if (it->type == MaterialTypeInputType::VECTOR2) {
          it->offset = l_Offset;
          l_Offset += sizeof(Math::Vector2);
          l_Vector2Count++;
        }
      }
      if (l_Vector2Count % 2 == 1) {
        // If there has been in odd number of vector2s that means that
        // there are two single fields free to be filled later on
        l_FreeSingles.push_back(l_Offset);
        l_FreeSingles.push_back(l_Offset + sizeof(u32));

        l_Offset += sizeof(u32) * 2;
      }

      // Assign all vector3
      for (auto it = l_Inputs.begin(); it != l_Inputs.end(); ++it) {
        if (it->type == MaterialTypeInputType::VECTOR3) {
          it->offset = l_Offset;
          l_Offset += sizeof(Math::Vector3);
          l_FreeSingles.push_back(l_Offset);
          l_Offset += sizeof(u32);
        }
      }

      // Assign all single space values
      for (auto it = l_Inputs.begin(); it != l_Inputs.end(); ++it) {
        if (it->type == MaterialTypeInputType::FLOAT ||
            it->type == MaterialTypeInputType::TEXTURE) {
          LOW_ASSERT(!l_FreeSingles.empty(),
                     "No space left in material info");

          u32 i_Offset = l_FreeSingles.front();
          l_FreeSingles.erase_first(i_Offset);

          it->offset = i_Offset;
        }
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_calculate_offsets
    }

    bool MaterialType::get_input(Low::Util::Name p_Name,
                                 MaterialTypeInput *p_Input)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_input
      Util::List<MaterialTypeInput> &l_Inputs = get_inputs();

      for (auto it = l_Inputs.begin(); it != l_Inputs.end(); ++it) {
        if (it->name == p_Name) {
          *p_Input = *it;
          return true;
        }
      }
      return false;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_input
    }

    void MaterialType::finalize()
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_finalize
      LOW_ASSERT(!is_initialized(),
                 "Cannot finalize materialtype that has already been "
                 "initialized");

      calculate_offsets();

      // TODO: Sort list based on offsets

      set_initialized(true);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_finalize
    }

    void MaterialType::add_input(Low::Util::Name p_Name,
                                 MaterialTypeInputType p_Type)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_input
      LOW_ASSERT(
          !is_initialized(),
          "Cannot add input to materialtype that has already been "
          "initialized");

      LOW_ASSERT(!has_input(p_Name),
                 "Material type already has input of that name");

      get_inputs().push_back({p_Name, p_Type, 0});
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_input
    }

    bool MaterialType::has_input(Low::Util::Name p_Name)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_has_input
      MaterialTypeInput l_Input;

      return get_input(p_Name, &l_Input);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_has_input
    }

    uint32_t MaterialType::get_input_offset(Low::Util::Name p_Name)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_input_offset
      LOW_ASSERT(is_initialized(),
                 "Cannot fetch offset from materialtype that has not "
                 "been finalized.");

      MaterialTypeInput l_Input;

      LOW_ASSERT(get_input(p_Name, &l_Input),
                 "Could not find input for material type");

      return l_Input.offset;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_input_offset
    }

    MaterialTypeInputType &
    MaterialType::get_input_type(Low::Util::Name p_Name)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_input_type
      MaterialTypeInput l_Input;

      LOW_ASSERT(get_input(p_Name, &l_Input),
                 "Could not find input for material type");

      return l_Input.type;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_input_type
    }

    uint32_t MaterialType::fill_input_names(
        Low::Util::List<Low::Util::Name> &p_InputNames)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_fill_input_names
      Util::List<MaterialTypeInput> &l_Inputs = get_inputs();

      for (auto it = l_Inputs.begin(); it != l_Inputs.end(); ++it) {
        p_InputNames.push_back(it->name);
      }

      return get_inputs().size();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_fill_input_names
    }

    void MaterialType::set_draw_vertex_shader_path(
        Low::Util::String p_Path)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_draw_vertex_shader_path
      get_draw_pipeline_config().vertexShaderPath = p_Path;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_draw_vertex_shader_path
    }

    void MaterialType::set_draw_fragment_shader_path(
        Low::Util::String p_Path)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_draw_fragment_shader_path
      get_draw_pipeline_config().fragmentShaderPath = p_Path;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_draw_fragment_shader_path
    }

    void MaterialType::set_depth_vertex_shader_path(
        Low::Util::String p_Path)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_depth_vertex_shader_path
      get_depth_pipeline_config().vertexShaderPath = p_Path;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_depth_vertex_shader_path
    }

    void MaterialType::set_depth_fragment_shader_path(
        Low::Util::String p_Path)
    {
      Low::Util::HandleLock<MaterialType> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_depth_fragment_shader_path
      get_depth_pipeline_config().fragmentShaderPath = p_Path;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_depth_fragment_shader_path
    }

    uint32_t MaterialType::create_instance(
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

    u32 MaterialType::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for MaterialType.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, MaterialType::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool MaterialType::get_page_for_index(const u32 p_Index,
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
