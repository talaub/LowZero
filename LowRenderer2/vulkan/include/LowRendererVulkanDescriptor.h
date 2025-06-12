#pragma once

#include "LowUtilContainers.h"

#include "LowMath.h"

#include "vulkan/vulkan.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      struct Image;
      namespace DescriptorUtil {
        struct DescriptorLayoutBuilder
        {
          Util::List<VkDescriptorSetLayoutBinding> m_Bindings;

          void add_binding(u32 p_Binding, VkDescriptorType p_Type,
                           u32 p_DescriptorCount = 1);
          void clear();
          VkDescriptorSetLayout
          build(VkDevice p_Device, VkShaderStageFlags p_SHaderStaged,
                void *p_Next = nullptr,
                VkDescriptorSetLayoutCreateFlags p_Flags = 0);
        };

        struct DescriptorAllocator
        {
          struct PoolSizeRatio
          {
            VkDescriptorType type;
            float ratio;
          };

          VkDescriptorPool m_Pool;

          void init_pool(VkDevice p_Device, u32 p_MaxSets,
                         Util::Span<PoolSizeRatio> p_PoolRatios);
          void clear_descriptors(VkDevice p_Device);
          void destroy_pool(VkDevice p_Device);

          VkDescriptorSet allocate(VkDevice p_Device,
                                   VkDescriptorSetLayout p_Layout);
        };

        struct DescriptorAllocatorGrowable
        {
          struct PoolSizeRatio
          {
            VkDescriptorType type;
            float ratio;
          };

          void init(VkDevice p_Deivice, u32 p_InitialSets,
                    Util::Span<PoolSizeRatio> p_PoolRatio);
          void clear_pools(VkDevice p_Device);
          void destroy_pools(VkDevice p_Device);

          VkDescriptorSet allocate(VkDevice p_Device,
                                   VkDescriptorSetLayout p_Layout,
                                   void *p_Next = nullptr);

        private:
          VkDescriptorPool get_pool(VkDevice p_Device);
          VkDescriptorPool
          create_pool(VkDevice p_Device, u32 p_SetCount,
                      Util::Span<PoolSizeRatio> p_PoolRatio);

          Util::List<PoolSizeRatio> m_Ratios;
          Util::List<VkDescriptorPool> m_FullPools;
          Util::List<VkDescriptorPool> m_ReadyPools;
          u32 m_SetsPerPool;
        };

        struct DescriptorWriter
        {
          Util::Deque<VkDescriptorImageInfo> m_ImageInfos;
          Util::Deque<VkDescriptorBufferInfo> m_BufferInfos;
          Util::List<VkWriteDescriptorSet> m_Writes;


        bool write_image(int p_Binding,
                                             Low::Renderer::Vulkan::Image p_Image,
                                             VkSampler p_Sampler,
                                             VkImageLayout p_Layout,
                                             VkDescriptorType p_Type,
                                             int p_ArrayElement=0);
          bool write_image(int p_Binding, VkImageView p_ImageView,
                           VkSampler p_Sampler,
                           VkImageLayout p_Layout,
                           VkDescriptorType p_Type,
                           int p_ArrayElement = 0);
          bool write_buffer(int p_Binding, VkBuffer p_Buffer,
                            size_t p_Size, size_t p_Offset,
                            VkDescriptorType p_Type);

          bool clear();
          bool update_set(VkDevice p_Device, VkDescriptorSet p_Set);
        };

      } // namespace DescriptorUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
