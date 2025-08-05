#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererHelper.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    enum class MaterialTypeInputType
    {
      VECTOR4,
      VECTOR3,
      VECTOR2,
      FLOAT
    };

    struct MaterialTypeInput
    {
      Util::Name name;
      MaterialTypeInputType type;
      u32 offset;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API MaterialTypeData
    {
      bool transparent;
      uint64_t draw_pipeline_handle;
      uint64_t depth_pipeline_handle;
      Util::List<MaterialTypeInput> inputs;
      bool initialized;
      Low::Renderer::GraphicsPipelineConfig draw_pipeline_config;
      Low::Renderer::GraphicsPipelineConfig depth_pipeline_config;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MaterialTypeData);
      }
    };

    struct LOW_RENDERER2_API MaterialType : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<MaterialType> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      MaterialType();
      MaterialType(uint64_t p_Id);
      MaterialType(MaterialType &p_Copy);

      static MaterialType make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit MaterialType(const MaterialType &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static MaterialType *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static MaterialType find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      MaterialType duplicate(Low::Util::Name p_Name) const;
      static MaterialType duplicate(MaterialType p_Handle,
                                    Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static MaterialType find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == MaterialType::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MaterialType l_MaterialType = p_Handle.get_id();
        l_MaterialType.destroy();
      }

      bool is_transparent() const;
      void set_transparent(bool p_Value);
      void toggle_transparent();

      uint64_t get_draw_pipeline_handle() const;
      void set_draw_pipeline_handle(uint64_t p_Value);

      uint64_t get_depth_pipeline_handle() const;
      void set_depth_pipeline_handle(uint64_t p_Value);

      bool is_initialized() const;
      void set_initialized(bool p_Value);
      void toggle_initialized();

      Low::Renderer::GraphicsPipelineConfig &
      get_draw_pipeline_config() const;

      Low::Renderer::GraphicsPipelineConfig &
      get_depth_pipeline_config() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      bool get_input(Low::Util::Name p_Name,
                     MaterialTypeInput *p_Input);
      void finalize();
      void add_input(Low::Util::Name p_Name,
                     MaterialTypeInputType p_Type);
      bool has_input(Low::Util::Name p_Name);
      uint32_t get_input_offset(Low::Util::Name p_Name);
      MaterialTypeInputType &get_input_type(Low::Util::Name p_Name);
      uint32_t fill_input_names(
          Low::Util::List<Low::Util::Name> &p_InputNames);
      void set_draw_vertex_shader_path(Low::Util::String p_Path);
      void set_draw_fragment_shader_path(Low::Util::String p_Path);
      void set_depth_vertex_shader_path(Low::Util::String p_Path);
      void set_depth_fragment_shader_path(Low::Util::String p_Path);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      Util::List<MaterialTypeInput> &get_inputs() const;
      void calculate_offsets();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
