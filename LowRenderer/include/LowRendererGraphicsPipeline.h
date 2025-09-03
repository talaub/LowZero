#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererBackend.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      struct PipelineGraphicsCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER_API GraphicsPipelineData
      {
        Backend::Pipeline pipeline;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(GraphicsPipelineData);
        }
      };

      struct LOW_RENDERER_API GraphicsPipeline
          : public Low::Util::Handle
      {
      public:
        static std::shared_mutex ms_BufferMutex;
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<GraphicsPipeline> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        GraphicsPipeline();
        GraphicsPipeline(uint64_t p_Id);
        GraphicsPipeline(GraphicsPipeline &p_Copy);

      private:
        static GraphicsPipeline make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);

      public:
        explicit GraphicsPipeline(const GraphicsPipeline &p_Copy)
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
        static GraphicsPipeline *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static GraphicsPipeline create_handle_by_index(u32 p_Index);

        static GraphicsPipeline find_by_index(uint32_t p_Index);
        static Low::Util::Handle _find_by_index(uint32_t p_Index);

        bool is_alive() const;

        u64 observe(Low::Util::Name p_Observable,
                    Low::Util::Handle p_Observer) const;
        u64 observe(Low::Util::Name p_Observable,
                    Low::Util::Function<void(Low::Util::Handle,
                                             Low::Util::Name)>
                        p_Observer) const;
        void notify(Low::Util::Handle p_Observed,
                    Low::Util::Name p_Observable);
        void broadcast_observable(Low::Util::Name p_Observable) const;

        static void _notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable);

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        GraphicsPipeline duplicate(Low::Util::Name p_Name) const;
        static GraphicsPipeline duplicate(GraphicsPipeline p_Handle,
                                          Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static GraphicsPipeline find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Yaml::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          READ_LOCK(l_Lock);
          return p_Handle.get_type() == GraphicsPipeline::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          GraphicsPipeline l_GraphicsPipeline = p_Handle.get_id();
          l_GraphicsPipeline.destroy();
        }

        Backend::Pipeline &get_pipeline() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static GraphicsPipeline
        make(Util::Name p_Name,
             PipelineGraphicsCreateParams &p_Params);
        void bind();

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Interface
  } // namespace Renderer
} // namespace Low
