#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererFrontendConfig.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    namespace MaterialTypePropertyType {
      enum Enum
      {
        VECTOR4,
        VECTOR3,
        VECTOR2,
        TEXTURE2D
      };
    }

    struct MaterialTypeProperty
    {
      Util::Name name;
      uint8_t type;
      uint32_t offset;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API MaterialTypeData
    {
      GraphicsPipelineConfig gbuffer_pipeline;
      GraphicsPipelineConfig depth_pipeline;
      bool internal;
      Util::List<MaterialTypeProperty> properties;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MaterialTypeData);
      }
    };

    struct LOW_RENDERER_API MaterialType : public Low::Util::Handle
    {
    public:
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

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static MaterialType find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
                                           Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == MaterialType::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MaterialType l_MaterialType = p_Handle.get_id();
        l_MaterialType.destroy();
      }

      GraphicsPipelineConfig &get_gbuffer_pipeline() const;
      void set_gbuffer_pipeline(GraphicsPipelineConfig &p_Value);

      GraphicsPipelineConfig &get_depth_pipeline() const;
      void set_depth_pipeline(GraphicsPipelineConfig &p_Value);

      bool is_internal() const;
      void set_internal(bool p_Value);

      Util::List<MaterialTypeProperty> &get_properties() const;
      void set_properties(Util::List<MaterialTypeProperty> &p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
    };
  } // namespace Renderer
} // namespace Low
