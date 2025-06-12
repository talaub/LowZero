#include "LowRendererVulkanDescriptor.h"

#include "LowRendererVulkanBase.h"

#include "LowUtilLogger.h"

#include "LowRendererVkImage.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace DescriptorUtil {
        void
        DescriptorLayoutBuilder::add_binding(u32 p_Binding,
                                             VkDescriptorType p_Type,
                                             u32 p_DescriptorCount)
        {
          VkDescriptorSetLayoutBinding l_Binding = {};
          l_Binding.binding = p_Binding;
          l_Binding.descriptorCount = p_DescriptorCount;
          l_Binding.descriptorType = p_Type;

          m_Bindings.push_back(l_Binding);
        }

        void DescriptorLayoutBuilder::clear()
        {
          m_Bindings.clear();
        }

        VkDescriptorSetLayout DescriptorLayoutBuilder::build(
            VkDevice p_Device, VkShaderStageFlags p_ShaderStages,
            void *p_Next, VkDescriptorSetLayoutCreateFlags p_Flags)
        {
          for (auto &it : m_Bindings) {
            it.stageFlags |= p_ShaderStages;
          }

          VkDescriptorSetLayoutCreateInfo l_Info;
          l_Info.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          l_Info.pNext = p_Next;

          l_Info.pBindings = m_Bindings.data();
          l_Info.bindingCount = (u32)m_Bindings.size();
          l_Info.flags = p_Flags;

          VkDescriptorSetLayout l_Set;
          LOWR_VK_CHECK(vkCreateDescriptorSetLayout(p_Device, &l_Info,
                                                    nullptr, &l_Set));

          return l_Set;
        }

        void DescriptorAllocator::init_pool(
            VkDevice p_Device, u32 p_MaxSets,
            Util::Span<PoolSizeRatio> p_PoolRatios)
        {
          Util::List<VkDescriptorPoolSize> l_PoolSizes;
          for (PoolSizeRatio i_Ratio : p_PoolRatios) {
            VkDescriptorPoolSize i_Size{};
            i_Size.type = i_Ratio.type;
            i_Size.descriptorCount = u32(i_Ratio.ratio * p_MaxSets);
            l_PoolSizes.push_back(i_Size);
          }

          VkDescriptorPoolCreateInfo l_PoolInfo{};
          l_PoolInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
          l_PoolInfo.flags = 0;
          l_PoolInfo.maxSets = p_MaxSets;
          l_PoolInfo.poolSizeCount = (u32)l_PoolSizes.size();
          l_PoolInfo.pPoolSizes = l_PoolSizes.data();

          vkCreateDescriptorPool(p_Device, &l_PoolInfo, nullptr,
                                 &m_Pool);
        }

        void DescriptorAllocator::clear_descriptors(VkDevice p_Device)
        {
          vkResetDescriptorPool(p_Device, m_Pool, 0);
        }

        void DescriptorAllocator::destroy_pool(VkDevice p_Device)
        {
          vkDestroyDescriptorPool(p_Device, m_Pool, nullptr);
        }

        VkDescriptorSet
        DescriptorAllocator::allocate(VkDevice p_Device,
                                      VkDescriptorSetLayout p_Layout)
        {
          VkDescriptorSetAllocateInfo l_Info{};
          l_Info.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          l_Info.pNext = nullptr;
          l_Info.descriptorPool = m_Pool;
          l_Info.descriptorSetCount = 1;
          l_Info.pSetLayouts = &p_Layout;

          VkDescriptorSet l_DescriptorSet;
          LOWR_VK_CHECK(vkAllocateDescriptorSets(p_Device, &l_Info,
                                                 &l_DescriptorSet));

          return l_DescriptorSet;
        }

        VkDescriptorPool
        DescriptorAllocatorGrowable::get_pool(VkDevice p_Device)
        {
          VkDescriptorPool l_NewPool;

          if (!m_ReadyPools.empty()) {
            l_NewPool = m_ReadyPools.back();
            m_ReadyPools.pop_back();
          } else {
            l_NewPool =
                create_pool(p_Device, m_SetsPerPool, m_Ratios);

            m_SetsPerPool *= 1.5f;
            if (m_SetsPerPool > 4092) {
              m_SetsPerPool = 4092;
            }
          }

          return l_NewPool;
        }

        VkDescriptorPool DescriptorAllocatorGrowable::create_pool(
            VkDevice p_Device, u32 p_SetCount,
            Util::Span<PoolSizeRatio> p_PoolRatios)
        {
          Util::List<VkDescriptorPoolSize> l_PoolSizes;
          for (PoolSizeRatio i_Ratio : p_PoolRatios) {
            VkDescriptorPoolSize i_Size{};
            i_Size.type = i_Ratio.type;
            i_Size.descriptorCount = u32(i_Ratio.ratio * p_SetCount);

            l_PoolSizes.push_back(i_Size);
          }

          VkDescriptorPoolCreateInfo l_PoolInfo = {};
          l_PoolInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
          l_PoolInfo.flags = 0;
          l_PoolInfo.maxSets = p_SetCount;
          l_PoolInfo.poolSizeCount = (u32)l_PoolSizes.size();
          l_PoolInfo.pPoolSizes = l_PoolSizes.data();

          VkDescriptorPool l_NewPool;
          vkCreateDescriptorPool(p_Device, &l_PoolInfo, nullptr,
                                 &l_NewPool);

          return l_NewPool;
        }

        void DescriptorAllocatorGrowable::init(
            VkDevice p_Device, u32 p_MaxSets,
            Util::Span<PoolSizeRatio> p_PoolRatios)
        {
          m_Ratios.clear();

          for (auto i_Ratio : p_PoolRatios) {
            m_Ratios.push_back(i_Ratio);
          }

          VkDescriptorPool l_NewPool =
              create_pool(p_Device, p_MaxSets, p_PoolRatios);

          m_SetsPerPool = p_MaxSets * 1.5f;

          m_ReadyPools.push_back(l_NewPool);
        }

        void
        DescriptorAllocatorGrowable::clear_pools(VkDevice p_Device)
        {
          for (auto i_Pool : m_ReadyPools) {
            vkResetDescriptorPool(p_Device, i_Pool, 0);
          }
          m_ReadyPools.clear();

          for (auto i_Pool : m_FullPools) {
            vkResetDescriptorPool(p_Device, i_Pool, 0);
          }
          m_FullPools.clear();
        }

        void
        DescriptorAllocatorGrowable::destroy_pools(VkDevice p_Device)
        {
          for (auto i_Pool : m_ReadyPools) {
            vkDestroyDescriptorPool(p_Device, i_Pool, 0);
          }
          m_ReadyPools.clear();

          for (auto i_Pool : m_FullPools) {
            vkDestroyDescriptorPool(p_Device, i_Pool, 0);
          }
          m_FullPools.clear();
        }

        VkDescriptorSet DescriptorAllocatorGrowable::allocate(
            VkDevice p_Device, VkDescriptorSetLayout p_Layout,
            void *p_Next)
        {
          VkDescriptorPool l_PoolToUse = get_pool(p_Device);

          VkDescriptorSetAllocateInfo l_AllocInfo = {};
          l_AllocInfo.pNext = p_Next;
          l_AllocInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          l_AllocInfo.descriptorPool = l_PoolToUse;
          l_AllocInfo.descriptorSetCount = 1;
          l_AllocInfo.pSetLayouts = &p_Layout;

          VkDescriptorSet l_DescriptorSet;
          VkResult l_Result = vkAllocateDescriptorSets(
              p_Device, &l_AllocInfo, &l_DescriptorSet);

          if (l_Result == VK_ERROR_OUT_OF_POOL_MEMORY ||
              l_Result == VK_ERROR_FRAGMENTED_POOL) {
            m_FullPools.push_back(l_PoolToUse);

            l_PoolToUse = get_pool(p_Device);
            l_AllocInfo.descriptorPool = l_PoolToUse;

            // TODO: Maybe try multiple times here

            LOWR_VK_CHECK(vkAllocateDescriptorSets(
                p_Device, &l_AllocInfo, &l_DescriptorSet));
          }

          m_ReadyPools.push_back(l_PoolToUse);
          return l_DescriptorSet;
        }

        bool DescriptorWriter::write_buffer(int p_Binding,
                                            VkBuffer p_Buffer,
                                            size_t p_Size,
                                            size_t p_Offset,
                                            VkDescriptorType p_Type)
        {
          {
            VkDescriptorBufferInfo l_LocalInfo{};
            l_LocalInfo.buffer = p_Buffer;
            l_LocalInfo.offset = p_Offset;
            l_LocalInfo.range = p_Size;

            m_BufferInfos.push_back(l_LocalInfo);
          }
          VkDescriptorBufferInfo &l_Info = m_BufferInfos.back();

          VkWriteDescriptorSet l_Write{};
          l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

          l_Write.dstBinding = p_Binding;
          l_Write.dstSet = VK_NULL_HANDLE;
          l_Write.descriptorCount = 1;
          l_Write.descriptorType = p_Type;
          l_Write.pBufferInfo = &l_Info;

          m_Writes.push_back(l_Write);

          return true;
        }

        bool DescriptorWriter::write_image(int p_Binding,
                                           Image p_Image,
                                           VkSampler p_Sampler,
                                           VkImageLayout p_Layout,
                                           VkDescriptorType p_Type,
                                           int p_ArrayElement)
        {
          return write_image(
              p_Binding, p_Image.get_allocated_image().imageView,
              p_Sampler, p_Layout, p_Type, p_ArrayElement);
        }

        bool DescriptorWriter::write_image(int p_Binding,
                                           VkImageView p_ImageView,
                                           VkSampler p_Sampler,
                                           VkImageLayout p_Layout,
                                           VkDescriptorType p_Type,
                                           int p_ArrayElement)
        {
          {
            VkDescriptorImageInfo l_LocalInfo{};

            l_LocalInfo.sampler = p_Sampler;
            l_LocalInfo.imageView = p_ImageView;
            l_LocalInfo.imageLayout = p_Layout;

            m_ImageInfos.push_back(l_LocalInfo);
          }
          VkDescriptorImageInfo &l_Info = m_ImageInfos.back();

          VkWriteDescriptorSet l_Write{};
          l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

          l_Write.dstBinding = p_Binding;
          l_Write.dstSet = VK_NULL_HANDLE;
          l_Write.descriptorCount = 1;
          l_Write.descriptorType = p_Type;
          l_Write.pImageInfo = &l_Info;
          l_Write.dstArrayElement = p_ArrayElement;

          m_Writes.push_back(l_Write);

          return true;
        }

        bool DescriptorWriter::clear()
        {
          m_ImageInfos.clear();
          m_Writes.clear();
          m_BufferInfos.clear();

          return true;
        }

        bool DescriptorWriter::update_set(VkDevice p_Device,
                                          VkDescriptorSet p_Set)
        {
          for (VkWriteDescriptorSet &i_Write : m_Writes) {
            i_Write.dstSet = p_Set;
          }

          vkUpdateDescriptorSets(p_Device, (u32)m_Writes.size(),
                                 m_Writes.data(), 0, nullptr);

          return true;
        }
      } // namespace DescriptorUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
