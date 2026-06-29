#include "LowGfxVulkanBackend.h"
#include "LowGfxVulkanState.h"

#include "LowGfxLogInternal.h"
#include "LowGfxAssert.h"

namespace Low {
  namespace Gfx {
    namespace Vulkan {

      static bool has_usage(BufferUsage p_Value, BufferUsage p_Flag)
      {
        return (static_cast<u32>(p_Value) &
                static_cast<u32>(p_Flag)) != 0;
      }

      static VkBufferUsageFlags to_vulkan_usage(BufferUsage p_Usage)
      {
        VkBufferUsageFlags l_Usage = 0;

        if (has_usage(p_Usage, BufferUsage::Vertex)) {
          l_Usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (has_usage(p_Usage, BufferUsage::Index)) {
          l_Usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (has_usage(p_Usage, BufferUsage::Uniform)) {
          l_Usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if (has_usage(p_Usage, BufferUsage::Storage)) {
          l_Usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (has_usage(p_Usage, BufferUsage::TransferSrc)) {
          l_Usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if (has_usage(p_Usage, BufferUsage::TransferDst)) {
          l_Usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        if (has_usage(p_Usage, BufferUsage::Indirect)) {
          l_Usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        return l_Usage;
      }

      static VmaMemoryUsage
      to_vma_memory_usage(BufferMemoryUsage p_MemoryUsage)
      {
        switch (p_MemoryUsage) {
        case BufferMemoryUsage::GpuOnly:
          return VMA_MEMORY_USAGE_GPU_ONLY;
        case BufferMemoryUsage::CpuToGpu:
          return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case BufferMemoryUsage::GpuToCpu:
          return VMA_MEMORY_USAGE_GPU_TO_CPU;
        }

        return VMA_MEMORY_USAGE_GPU_ONLY;
      }

      static VmaAllocationCreateFlags
      to_vma_allocation_flags(BufferMemoryUsage p_MemoryUsage)
      {
        switch (p_MemoryUsage) {
        case BufferMemoryUsage::GpuOnly:
          return 0;
        case BufferMemoryUsage::CpuToGpu:
        case BufferMemoryUsage::GpuToCpu:
          return VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        return 0;
      }

      static void set_debug_name(const VulkanContextState &p_State,
                                 const VulkanBufferState &p_Buffer,
                                 const char *p_Name)
      {
        if (!p_Name || p_Buffer.buffer == VK_NULL_HANDLE) {
          return;
        }

        vmaSetAllocationName(p_State.allocator, p_Buffer.allocation,
                             p_Name);

        PFN_vkSetDebugUtilsObjectNameEXT l_SetObjectName =
            reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                vkGetDeviceProcAddr(p_State.device,
                                    "vkSetDebugUtilsObjectNameEXT"));
        if (!l_SetObjectName) {
          return;
        }

        VkDebugUtilsObjectNameInfoEXT l_NameInfo{};
        l_NameInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        l_NameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        l_NameInfo.objectHandle =
            reinterpret_cast<uint64_t>(p_Buffer.buffer);
        l_NameInfo.pObjectName = p_Name;

        l_SetObjectName(p_State.device, &l_NameInfo);
      }

      Detail::BackendBuffer
      create_buffer(Detail::ContextImpl &p_Context,
                    const BufferDesc &p_Desc)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        GFX_ASSERT(
            l_State,
            "Cannot create Vulkan buffer without context state");
        GFX_ASSERT(l_State->allocator,
                   "Cannot create Vulkan buffer without allocator");

        VulkanBufferState *l_BufferState = new VulkanBufferState();

        VkBufferCreateInfo l_BufferInfo{};
        l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        l_BufferInfo.size = p_Desc.size;
        l_BufferInfo.usage = to_vulkan_usage(p_Desc.usage);

        VmaAllocationCreateInfo l_AllocationInfo{};
        l_AllocationInfo.usage =
            to_vma_memory_usage(p_Desc.memory_usage);
        l_AllocationInfo.flags =
            to_vma_allocation_flags(p_Desc.memory_usage);

        VkResult l_Result = vmaCreateBuffer(
            l_State->allocator, &l_BufferInfo, &l_AllocationInfo,
            &l_BufferState->buffer, &l_BufferState->allocation,
            &l_BufferState->info);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create Vulkan buffer: {}",
                       static_cast<int>(l_Result));
          delete l_BufferState;
          GFX_ASSERT(false, "Failed to create Vulkan buffer");
        }

        set_debug_name(*l_State, *l_BufferState, p_Desc.debug_name);

        Detail::BackendBuffer l_Buffer;
        l_Buffer.size = p_Desc.size;
        l_Buffer.usage = p_Desc.usage;
        l_Buffer.backend_state = l_BufferState;
        return l_Buffer;
      }

      void destroy_buffer(Detail::ContextImpl &p_Context,
                          Detail::BackendBuffer &p_Buffer)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        GFX_ASSERT(
            l_State,
            "Cannot destroy Vulkan buffer without context state");

        VulkanBufferState *l_BufferState =
            static_cast<VulkanBufferState *>(p_Buffer.backend_state);
        if (l_BufferState) {
          if (l_State->allocator &&
              l_BufferState->buffer != VK_NULL_HANDLE &&
              l_BufferState->allocation) {
            vmaDestroyBuffer(l_State->allocator,
                             l_BufferState->buffer,
                             l_BufferState->allocation);
          }

          delete l_BufferState;
        }

        p_Buffer.size = 0;
        p_Buffer.usage = BufferUsage::None;
        p_Buffer.backend_state = nullptr;
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
