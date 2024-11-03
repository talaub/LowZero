#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include <vulkan/vulkan.h>
#include "LowRendererVulkan.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER2_API PipelineData
      {
        VkPipeline pipeline;
        VkPipelineLayout layout;
        Context *context;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(PipelineData);
        }
      };

      struct LOW_RENDERER2_API Pipeline : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Pipeline> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Pipeline();
        Pipeline(uint64_t p_Id);
        Pipeline(Pipeline &p_Copy);

      private:
        static Pipeline make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);

      public:
        explicit Pipeline(const Pipeline &p_Copy)
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
        static Pipeline *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Pipeline find_by_index(uint32_t p_Index);

        bool is_alive() const;

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        Pipeline duplicate(Low::Util::Name p_Name) const;
        static Pipeline duplicate(Pipeline p_Handle,
                                  Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static Pipeline find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Yaml::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          return p_Handle.get_type() == Pipeline::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Pipeline l_Pipeline = p_Handle.get_id();
          l_Pipeline.destroy();
        }

        VkPipeline &get_pipeline() const;
        void set_pipeline(VkPipeline &p_Value);

        VkPipelineLayout &get_layout() const;
        void set_layout(VkPipelineLayout &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Pipeline make(Context &p_Context);

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
        Context *get_context() const;
        void set_context(Context *p_Value);
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
