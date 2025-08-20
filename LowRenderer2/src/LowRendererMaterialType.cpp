#include "LowRendererMaterialType.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
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
    uint8_t *MaterialType::ms_Buffer = 0;
    std::shared_mutex MaterialType::ms_BufferMutex;
    Low::Util::Instances::Slot *MaterialType::ms_Slots = 0;
    Low::Util::List<MaterialType> MaterialType::ms_LivingInstances =
        Low::Util::List<MaterialType>();

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
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      MaterialType l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, transparent, bool) =
          false;
      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType, inputs,
                              Util::List<MaterialTypeInput>))
          Util::List<MaterialTypeInput>();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, initialized, bool) =
          false;
      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType,
                              draw_pipeline_config,
                              Low::Renderer::GraphicsPipelineConfig))
          Low::Renderer::GraphicsPipelineConfig();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType,
                              depth_pipeline_config,
                              Low::Renderer::GraphicsPipelineConfig))
          Low::Renderer::GraphicsPipelineConfig();
      new (&ACCESSOR_TYPE_SOA(l_Handle, MaterialType, family,
                              Low::Renderer::MaterialTypeFamily))
          Low::Renderer::MaterialTypeFamily();
      ACCESSOR_TYPE_SOA(l_Handle, MaterialType, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

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
        // l_Handle.get_draw_pipeline_config().depthFormat
      }
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MaterialType::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const MaterialType *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void MaterialType::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(MaterialType));

      initialize_buffer(&ms_Buffer, MaterialTypeData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_MaterialType);
      LOW_PROFILE_ALLOC(type_slots_MaterialType);

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
            offsetof(MaterialTypeData, transparent);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
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
            offsetof(MaterialTypeData, draw_pipeline_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
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
            offsetof(MaterialTypeData, depth_pipeline_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
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
            offsetof(MaterialTypeData, inputs);
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
            offsetof(MaterialTypeData, initialized);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
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
            offsetof(MaterialTypeData, draw_pipeline_config);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
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
            offsetof(MaterialTypeData, depth_pipeline_config);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
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
            offsetof(MaterialTypeData, family);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType = Low::Renderer::
            MaterialTypeFamilyEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(MaterialTypeData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MaterialType l_Handle = p_Handle.get_id();
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
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_MaterialType);
      LOW_PROFILE_FREE(type_slots_MaterialType);
      LOCK_UNLOCK(l_Lock);
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
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = MaterialType::TYPE_ID;

      return l_Handle;
    }

    bool MaterialType::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == MaterialType::TYPE_ID &&
             check_alive(ms_Slots, MaterialType::get_capacity());
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
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
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

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_transparent
      // LOW_CODEGEN::END::CUSTOM:GETTER_transparent

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, transparent, bool);
    }
    void MaterialType::toggle_transparent()
    {
      set_transparent(!is_transparent());
    }

    void MaterialType::set_transparent(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_transparent
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_transparent

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, transparent, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_transparent
      // LOW_CODEGEN::END::CUSTOM:SETTER_transparent

      broadcast_observable(N(transparent));
    }

    uint64_t MaterialType::get_draw_pipeline_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_pipeline_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, draw_pipeline_handle, uint64_t);
    }
    void MaterialType::set_draw_pipeline_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_draw_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_draw_pipeline_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, draw_pipeline_handle, uint64_t) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_draw_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_draw_pipeline_handle

      broadcast_observable(N(draw_pipeline_handle));
    }

    uint64_t MaterialType::get_depth_pipeline_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_pipeline_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, depth_pipeline_handle, uint64_t);
    }
    void MaterialType::set_depth_pipeline_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_depth_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_depth_pipeline_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, depth_pipeline_handle, uint64_t) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_depth_pipeline_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_depth_pipeline_handle

      broadcast_observable(N(depth_pipeline_handle));
    }

    Util::List<MaterialTypeInput> &MaterialType::get_inputs() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_inputs
      // LOW_CODEGEN::END::CUSTOM:GETTER_inputs

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, inputs,
                      Util::List<MaterialTypeInput>);
    }

    bool MaterialType::is_initialized() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_initialized
      // LOW_CODEGEN::END::CUSTOM:GETTER_initialized

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, initialized, bool);
    }
    void MaterialType::toggle_initialized()
    {
      set_initialized(!is_initialized());
    }

    void MaterialType::set_initialized(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_initialized
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_initialized

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, initialized, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_initialized
      // LOW_CODEGEN::END::CUSTOM:SETTER_initialized

      broadcast_observable(N(initialized));
    }

    Low::Renderer::GraphicsPipelineConfig &
    MaterialType::get_draw_pipeline_config() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_pipeline_config
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_pipeline_config

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, draw_pipeline_config,
                      Low::Renderer::GraphicsPipelineConfig);
    }

    Low::Renderer::GraphicsPipelineConfig &
    MaterialType::get_depth_pipeline_config() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_depth_pipeline_config
      // LOW_CODEGEN::END::CUSTOM:GETTER_depth_pipeline_config

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, depth_pipeline_config,
                      Low::Renderer::GraphicsPipelineConfig);
    }

    Low::Renderer::MaterialTypeFamily MaterialType::get_family() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_family
      // LOW_CODEGEN::END::CUSTOM:GETTER_family

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, family,
                      Low::Renderer::MaterialTypeFamily);
    }
    void MaterialType::set_family(
        Low::Renderer::MaterialTypeFamily p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_family
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_family

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, family,
               Low::Renderer::MaterialTypeFamily) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_family
      // LOW_CODEGEN::END::CUSTOM:SETTER_family

      broadcast_observable(N(family));
    }

    Low::Util::Name MaterialType::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(MaterialType, name, Low::Util::Name);
    }
    void MaterialType::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(MaterialType, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

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
        l_Handle.get_draw_pipeline_config().depthFormat =
            ImageFormat::DEPTH;
      } else if (p_Family == MaterialTypeFamily::UI) {
        l_Handle.get_draw_pipeline_config()
            .colorAttachmentFormats.push_back(
                ImageFormat::RGBA16_SFLOAT);

        l_Handle.get_draw_pipeline_config().depthTest = false;
      }

      return l_Handle;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void MaterialType::calculate_offsets()
    {
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
        if (it->type == MaterialTypeInputType::FLOAT) {
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
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_finalize
      LOW_ASSERT(!is_initialized(),
                 "Cannot finalize materialtype that has already been "
                 "initialized");

      calculate_offsets();

      // TODO: Sort list based on offsets
      // TODO: Create graphics pipelines

      set_initialized(true);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_finalize
    }

    void MaterialType::add_input(Low::Util::Name p_Name,
                                 MaterialTypeInputType p_Type)
    {
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
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_has_input
      MaterialTypeInput l_Input;

      return get_input(p_Name, &l_Input);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_has_input
    }

    uint32_t MaterialType::get_input_offset(Low::Util::Name p_Name)
    {
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
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_draw_vertex_shader_path
      get_draw_pipeline_config().vertexShaderPath = p_Path;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_draw_vertex_shader_path
    }

    void MaterialType::set_draw_fragment_shader_path(
        Low::Util::String p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_draw_fragment_shader_path
      get_draw_pipeline_config().fragmentShaderPath = p_Path;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_draw_fragment_shader_path
    }

    void MaterialType::set_depth_vertex_shader_path(
        Low::Util::String p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_depth_vertex_shader_path
      get_depth_pipeline_config().vertexShaderPath = p_Path;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_depth_vertex_shader_path
    }

    void MaterialType::set_depth_fragment_shader_path(
        Low::Util::String p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_depth_fragment_shader_path
      get_depth_pipeline_config().fragmentShaderPath = p_Path;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_depth_fragment_shader_path
    }

    uint32_t MaterialType::create_instance()
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

    void MaterialType::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer =
          (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                            sizeof(MaterialTypeData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, transparent) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, transparent) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData,
                                     draw_pipeline_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData,
                                   draw_pipeline_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData,
                                     depth_pipeline_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData,
                                   depth_pipeline_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          MaterialType i_MaterialType = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(MaterialTypeData, inputs) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::List<MaterialTypeInput>))])
              Util::List<MaterialTypeInput>();
          *i_ValPtr =
              ACCESSOR_TYPE_SOA(i_MaterialType, MaterialType, inputs,
                                Util::List<MaterialTypeInput>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, initialized) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, initialized) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData,
                                     draw_pipeline_config) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData,
                                   draw_pipeline_config) *
                          (l_Capacity)],
               l_Capacity *
                   sizeof(Low::Renderer::GraphicsPipelineConfig));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData,
                                     depth_pipeline_config) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData,
                                   depth_pipeline_config) *
                          (l_Capacity)],
               l_Capacity *
                   sizeof(Low::Renderer::GraphicsPipelineConfig));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, family) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, family) *
                          (l_Capacity)],
               l_Capacity *
                   sizeof(Low::Renderer::MaterialTypeFamily));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MaterialTypeData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MaterialTypeData, name) *
                          (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for MaterialType from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
