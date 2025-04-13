#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererVulkan.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER2_API ViewInfoData
      {
        AllocatedBuffer view_data_buffer;
        VkDescriptorSet view_data_descriptor_set;
        VkDescriptorSet lighting_descriptor_set;
        Low::Util::List<StagingBuffer> staging_buffers;
        bool initialized;
        VkDescriptorSet gbuffer_descriptor_set;
        AllocatedImage gbuffer_depth;
        AllocatedImage gbuffer_albedo;
        AllocatedImage gbuffer_normals;
        AllocatedImage img_lit;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(ViewInfoData);
        }
      };

      struct LOW_RENDERER2_API ViewInfo : public Low::Util::Handle
      {
      public:
        static std::shared_mutex ms_BufferMutex;
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<ViewInfo> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        ViewInfo();
        ViewInfo(uint64_t p_Id);
        ViewInfo(ViewInfo &p_Copy);

        static ViewInfo make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);
        explicit ViewInfo(const ViewInfo &p_Copy)
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
        static ViewInfo *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static ViewInfo find_by_index(uint32_t p_Index);
        static Low::Util::Handle _find_by_index(uint32_t p_Index);

        bool is_alive() const;

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        ViewInfo duplicate(Low::Util::Name p_Name) const;
        static ViewInfo duplicate(ViewInfo p_Handle,
                                  Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static ViewInfo find_by_name(Low::Util::Name p_Name);
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
          return p_Handle.get_type() == ViewInfo::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          ViewInfo l_ViewInfo = p_Handle.get_id();
          l_ViewInfo.destroy();
        }

        AllocatedBuffer &get_view_data_buffer() const;
        void set_view_data_buffer(AllocatedBuffer &p_Value);

        VkDescriptorSet get_view_data_descriptor_set() const;
        void set_view_data_descriptor_set(VkDescriptorSet p_Value);

        VkDescriptorSet &get_lighting_descriptor_set() const;
        void set_lighting_descriptor_set(VkDescriptorSet &p_Value);

        Low::Util::List<StagingBuffer> &get_staging_buffers() const;
        void
        set_staging_buffers(Low::Util::List<StagingBuffer> &p_Value);

        bool is_initialized() const;
        void set_initialized(bool p_Value);

        VkDescriptorSet &get_gbuffer_descriptor_set() const;
        void set_gbuffer_descriptor_set(VkDescriptorSet &p_Value);

        AllocatedImage &get_gbuffer_depth() const;
        void set_gbuffer_depth(AllocatedImage &p_Value);

        AllocatedImage &get_gbuffer_albedo() const;
        void set_gbuffer_albedo(AllocatedImage &p_Value);

        AllocatedImage &get_gbuffer_normals() const;
        void set_gbuffer_normals(AllocatedImage &p_Value);

        AllocatedImage &get_img_lit() const;
        void set_img_lit(AllocatedImage &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
