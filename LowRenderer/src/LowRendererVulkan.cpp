#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

#include "LowRendererImGuiHelper.h"
#include "LowRendererImGuiImage.h"

#include "ImGuizmo.h"

#include "vulkan/vulkan_core.h"
#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000
#include "../../LowDependencies/VulkanMemoryAllocator/include/vk_mem_alloc.h"

#include "IconsFontAwesome5.h"
#include "IconsLucide.h"

#include "LowRendererVulkan.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilContainers.h"
#include "LowUtilFileIO.h"
#include "LowUtilGlobals.h"

#include "LowMath.h"

#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowRendererBuffer.h"

#include <GLFW/glfw3.h>

#include <corecrt_malloc.h>
#include <stdint.h>
#include <string>

#define SKIP_DEBUG_LEVEL true

#define MAX_PRS 255

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      void renderpass_create_internal(
          Backend::Renderpass &p_Renderpass,
          Backend::RenderpassCreateParams &p_Params, bool, bool);
      void
      renderpass_cleanup_internal(Backend::Renderpass &p_Renderpass,
                                  bool p_ClearVulkanRenderpass);
      void
      vk_renderpass_create(Backend::Renderpass &p_Renderpass,
                           Backend::RenderpassCreateParams &p_Params);
      void vk_renderpass_cleanup(Backend::Renderpass &p_Renderpass);
      void vk_renderpass_begin(Backend::Renderpass &p_Renderpass);
      void vk_renderpass_end(Backend::Renderpass &p_Renderpass);
      void vk_imageresource_create(
          Backend::ImageResource &p_Image,
          Backend::ImageResourceCreateParams &p_Params);
      void vk_imageresource_cleanup(Backend::ImageResource &p_Image);

      void vk_perform_barriers(Backend::Context &p_Context);
      void vk_bind_resources(Backend::Context &p_Context,
                             VkPipelineBindPoint p_BindPoint);

      void imageresource_create_internal(
          Backend::ImageResource &p_Image,
          Backend::ImageResourceCreateParams &p_Params,
          bool p_CreateSampler);

      namespace Helper {

        struct SwapChainSupportDetails
        {
          VkSurfaceCapabilitiesKHR m_Capabilities;
          Util::List<VkSurfaceFormatKHR> m_Formats;
          Util::List<VkPresentModeKHR> m_PresentModes;
        };

        struct QueueFamilyIndices
        {
          Util::Optional<uint32_t> m_GraphicsFamily;
          Util::Optional<uint32_t> m_PresentFamily;
          Util::Optional<uint32_t> m_TransferFamily;

          bool is_complete()
          {
            return m_GraphicsFamily.has_value() &&
                   m_PresentFamily.has_value() &&
                   m_TransferFamily.has_value();
          }
        };

        uint32_t find_memory_type(Context &p_Context,
                                  uint32_t p_TypeFilter,
                                  VkMemoryPropertyFlags p_Properties)
        {
          VkPhysicalDeviceMemoryProperties l_MemProperties;
          vkGetPhysicalDeviceMemoryProperties(
              p_Context.m_PhysicalDevice, &l_MemProperties);

          for (uint32_t i_Iter = 0;
               i_Iter < l_MemProperties.memoryTypeCount; i_Iter++) {
            if ((p_TypeFilter & (1 << i_Iter)) &&
                (l_MemProperties.memoryTypes[i_Iter].propertyFlags &
                 p_Properties) == p_Properties) {
              return i_Iter;
            }
          }
          LOW_ASSERT(false, "Failed to find suitable memory type");
        }

        static VkSurfaceFormatKHR choose_swap_surface_format(
            const Low::Util::List<VkSurfaceFormatKHR>
                &p_AvailableFormats)
        {
          for (const auto &i_AvailableFormat : p_AvailableFormats) {
            if (i_AvailableFormat.format ==
                VK_FORMAT_B8G8R8A8_UNORM) { // Should be SRGB but
                                            // UNORM to be compatible
                                            // with ImGui viewports
              return i_AvailableFormat;
            }
          }
          return p_AvailableFormats[0];
        }

        static SwapChainSupportDetails
        query_swap_chain_support(Context &p_Context,
                                 VkPhysicalDevice p_Device)
        {
          SwapChainSupportDetails l_Details;

          // Query device surface capabilities
          vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
              p_Device, p_Context.m_Surface,
              &l_Details.m_Capabilities);

          // Query supported surface formats
          uint32_t l_FormatCount;
          vkGetPhysicalDeviceSurfaceFormatsKHR(
              p_Device, p_Context.m_Surface, &l_FormatCount, nullptr);

          if (l_FormatCount != 0) {
            l_Details.m_Formats.resize(l_FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                p_Device, p_Context.m_Surface, &l_FormatCount,
                l_Details.m_Formats.data());
          }

          // Query supported presentations modes
          uint32_t l_PresentModeCount;
          vkGetPhysicalDeviceSurfacePresentModesKHR(
              p_Device, p_Context.m_Surface, &l_PresentModeCount,
              nullptr);

          if (l_PresentModeCount != 0) {
            l_Details.m_PresentModes.resize(l_PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                p_Device, p_Context.m_Surface, &l_PresentModeCount,
                l_Details.m_PresentModes.data());
          }

          return l_Details;
        }

        static VkPresentModeKHR choose_swap_present_mode(
            const Low::Util::List<VkPresentModeKHR>
                &p_AvailablePresentModes)
        {
          // We look for our prefered present mode
          for (const auto &i_AvalablePresentMode :
               p_AvailablePresentModes) {
            if (i_AvalablePresentMode ==
                VK_PRESENT_MODE_MAILBOX_KHR) {
              return i_AvalablePresentMode;
            }
          }

          // The FIFO_KHR mode is the only mode that is guaranteed to
          // be supported so this will be our fallback
          return VK_PRESENT_MODE_FIFO_KHR;
        }

        static Helper::QueueFamilyIndices
        find_queue_families(Context &p_Context,
                            VkPhysicalDevice p_Device)
        {
          QueueFamilyIndices l_Indices;

          uint32_t l_QueueFamilyCount = 0;
          vkGetPhysicalDeviceQueueFamilyProperties(
              p_Device, &l_QueueFamilyCount, nullptr);

          Low::Util::List<VkQueueFamilyProperties> l_QueueFamilies(
              l_QueueFamilyCount);
          vkGetPhysicalDeviceQueueFamilyProperties(
              p_Device, &l_QueueFamilyCount, l_QueueFamilies.data());

          int l_Iterator = 0;
          for (const auto &i_QueueFamiliy : l_QueueFamilies) {
            VkBool32 i_PresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(p_Device, l_Iterator,
                                                 p_Context.m_Surface,
                                                 &i_PresentSupport);
            if (i_PresentSupport) {
              l_Indices.m_PresentFamily = l_Iterator;
            }

            if (i_QueueFamiliy.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
              l_Indices.m_GraphicsFamily = l_Iterator;
            }
            if (i_QueueFamiliy.queueFlags & VK_QUEUE_TRANSFER_BIT) {
              l_Indices.m_TransferFamily = l_Iterator;
            }
            l_Iterator++;
          }

          return l_Indices;
        }

        static VkFormat find_supported_format(
            Context &p_Context,
            const Low::Util::List<VkFormat> &p_Candidates,
            VkImageTiling p_Tiling, VkFormatFeatureFlags p_Features)
        {
          for (VkFormat i_Format : p_Candidates) {
            VkFormatProperties i_Properties;
            vkGetPhysicalDeviceFormatProperties(
                p_Context.m_PhysicalDevice, i_Format, &i_Properties);

            if (p_Tiling == VK_IMAGE_TILING_LINEAR &&
                (i_Properties.linearTilingFeatures & p_Features) ==
                    p_Features) {
              return i_Format;
            } else if (p_Tiling == VK_IMAGE_TILING_OPTIMAL &&
                       (i_Properties.optimalTilingFeatures &
                        p_Features) == p_Features) {
              return i_Format;
            }
          }

          LOW_ASSERT(false, "Failed to find supported format");
          return {};
        }

        static uint8_t vkformat_to_imageformat(VkFormat p_Format)
        {
          switch (p_Format) {
          case VK_FORMAT_B8G8R8A8_SRGB:
            return Backend::ImageFormat::BGRA8_SRGB;
          case VK_FORMAT_B8G8R8A8_UNORM:
            return Backend::ImageFormat::BGRA8_UNORM;
          case VK_FORMAT_R32G32B32A32_SFLOAT:
            return Backend::ImageFormat::RGBA32_SFLOAT;
          case VK_FORMAT_R16G16B16A16_SFLOAT:
            return Backend::ImageFormat::RGBA16_SFLOAT;
          case VK_FORMAT_R8G8B8A8_UNORM:
            return Backend::ImageFormat::RGBA8_UNORM;
          case VK_FORMAT_R8_UNORM:
            return Backend::ImageFormat::R8_UNORM;
          case VK_FORMAT_R32_UINT:
            return Backend::ImageFormat::R32_UINT;
          case VK_FORMAT_D32_SFLOAT:
            return Backend::ImageFormat::D32_SFLOAT;
          case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return Backend::ImageFormat::D32_SFLOAT_S8_UINT;
          case VK_FORMAT_D24_UNORM_S8_UINT:
            return Backend::ImageFormat::D24_UNORM_S8_UINT;
          default:
            LOW_ASSERT(false, "Unknown vk format");
            return 0;
          }
        }

        static VkFormat imageformat_to_vkformat(uint8_t p_Format)
        {
          switch (p_Format) {
          case Backend::ImageFormat::BGRA8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
          case Backend::ImageFormat::BGRA8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;
          case Backend::ImageFormat::RGBA16_SFLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
          case Backend::ImageFormat::RGBA32_SFLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
          case Backend::ImageFormat::RGBA8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
          case Backend::ImageFormat::R8_UNORM:
            return VK_FORMAT_R8_UNORM;
          case Backend::ImageFormat::R32_UINT:
            return VK_FORMAT_R32_UINT;
          case Backend::ImageFormat::D32_SFLOAT:
            return VK_FORMAT_D32_SFLOAT;
          case Backend::ImageFormat::D32_SFLOAT_S8_UINT:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
          case Backend::ImageFormat::D24_UNORM_S8_UINT:
            return VK_FORMAT_D24_UNORM_S8_UINT;
          default:
            LOW_ASSERT(false, "Unknown image format");
            return VK_FORMAT_A2B10G10R10_SINT_PACK32;
          }
        }

        static VkCommandBuffer
        begin_single_time_commands(Backend::Context &p_Context)
        {
          VkCommandBufferAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType =
              VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          l_AllocInfo.commandPool = p_Context.vk.m_CommandPool;
          l_AllocInfo.commandBufferCount = 1;

          VkCommandBuffer l_CommandBuffer;
          vkAllocateCommandBuffers(p_Context.vk.m_Device,
                                   &l_AllocInfo, &l_CommandBuffer);

          VkCommandBufferBeginInfo l_BeginInfo{};
          l_BeginInfo.sType =
              VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
          l_BeginInfo.flags =
              VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

          LOW_ASSERT(vkBeginCommandBuffer(l_CommandBuffer,
                                          &l_BeginInfo) == VK_SUCCESS,
                     "Failed to begine one time command buffer");

          return l_CommandBuffer;
        }

        static void
        end_single_time_commands(Backend::Context &p_Context,
                                 VkCommandBuffer p_CommandBuffer)
        {
          vkEndCommandBuffer(p_CommandBuffer);

          VkSubmitInfo l_SubmitInfo{};
          l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
          l_SubmitInfo.commandBufferCount = 1;
          l_SubmitInfo.pCommandBuffers = &p_CommandBuffer;

          vkQueueSubmit(p_Context.vk.m_GraphicsQueue, 1,
                        &l_SubmitInfo, VK_NULL_HANDLE);
          vkQueueWaitIdle(p_Context.vk.m_GraphicsQueue);
          vkDeviceWaitIdle(p_Context.vk.m_Device);

          vkFreeCommandBuffers(p_Context.vk.m_Device,
                               p_Context.vk.m_CommandPool, 1,
                               &p_CommandBuffer);
        }

        void create_buffer(Backend::Context &p_Context,
                           VkDeviceSize p_Size,
                           VkBufferUsageFlags p_Usage,
                           VkMemoryPropertyFlags p_Properties,
                           VkBuffer &p_Buffer,
                           VkDeviceMemory &p_BufferMemory)
        {
          VkBufferCreateInfo l_BufferInfo{};
          l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
          l_BufferInfo.size = p_Size;
          l_BufferInfo.usage = p_Usage;
          l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

          LOW_ASSERT(vkCreateBuffer(p_Context.vk.m_Device,
                                    &l_BufferInfo, nullptr,
                                    &p_Buffer) == VK_SUCCESS,
                     "Failed to create buffer");

          VkMemoryRequirements l_MemoryRequirements;
          vkGetBufferMemoryRequirements(
              p_Context.vk.m_Device, p_Buffer, &l_MemoryRequirements);

          VkMemoryAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
          l_AllocInfo.allocationSize = l_MemoryRequirements.size;
          l_AllocInfo.memoryTypeIndex = find_memory_type(
              p_Context.vk, l_MemoryRequirements.memoryTypeBits,
              p_Properties);

          LOW_ASSERT(vkAllocateMemory(p_Context.vk.m_Device,
                                      &l_AllocInfo, nullptr,
                                      &p_BufferMemory) == VK_SUCCESS,
                     "Failed to allocate buffer memor");

          vkBindBufferMemory(p_Context.vk.m_Device, p_Buffer,
                             p_BufferMemory, 0);
        }
        static void copy_buffer(Backend::Context &p_Context,
                                VkBuffer p_SourceBuffer,
                                VkBuffer p_DestBuffer,
                                VkDeviceSize p_Size,
                                VkDeviceSize p_SourceOffset = 0u,
                                VkDeviceSize p_DestOffset = 0u)
        {

          VkCommandBuffer l_CommandBuffer;

          bool l_UsingTransferCommandBuffer = p_Context.running;

          if (l_UsingTransferCommandBuffer) {
            l_CommandBuffer = p_Context.vk.m_TransferCommandBuffer;
            if (p_Context.debugEnabled) {
              LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
                  p_Context, "Copy buffer - Transfer",
                  Math::Color(0.241f, 0.6249f, 0.6341f, 1.0f));
            }
          } else {
            VkCommandBufferAllocateInfo l_AllocInfo{};
            l_AllocInfo.sType =
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            l_AllocInfo.commandPool =
                p_Context.vk.m_TransferCommandPool;
            l_AllocInfo.commandBufferCount = 1;
            vkAllocateCommandBuffers(p_Context.vk.m_Device,
                                     &l_AllocInfo, &l_CommandBuffer);

            VkCommandBufferBeginInfo l_BeginInfo{};
            l_BeginInfo.sType =
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            l_BeginInfo.flags =
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo);

            if (p_Context.debugEnabled) {
              LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
                  p_Context, "Copy buffer - Immediate",
                  Math::Color(0.841f, 0.2249f, 0.2341f, 1.0f));
            }
          }

          VkBufferCopy l_CopyRegion{};
          l_CopyRegion.srcOffset = p_SourceOffset;
          l_CopyRegion.dstOffset = p_DestOffset;
          l_CopyRegion.size = p_Size;
          vkCmdCopyBuffer(l_CommandBuffer, p_SourceBuffer,
                          p_DestBuffer, 1, &l_CopyRegion);

          if (!l_UsingTransferCommandBuffer) {
            vkEndCommandBuffer(l_CommandBuffer);

            VkSubmitInfo l_SubmitInfo{};
            l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            l_SubmitInfo.commandBufferCount = 1;
            l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;

            vkQueueSubmit(p_Context.vk.m_TransferQueue, 1,
                          &l_SubmitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(p_Context.vk.m_TransferQueue);

            vkFreeCommandBuffers(p_Context.vk.m_Device,
                                 p_Context.vk.m_TransferCommandPool,
                                 1, &l_CommandBuffer);
          }

          if (p_Context.debugEnabled) {
            LOW_RENDERER_END_RENDERDOC_SECTION(p_Context);
          }
        }

        VkFormat vkformat_get_depth(Backend::Context &p_Context)
        {
          return Helper::find_supported_format(
              p_Context.vk,
              {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
               VK_FORMAT_D24_UNORM_S8_UINT},
              VK_IMAGE_TILING_OPTIMAL,
              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }
      } // namespace Helper

      namespace ContextHelper {
        Util::List<const char *> g_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"};

        Util::List<const char *> g_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME};

        static bool check_validation_layer_support()
        {
          uint32_t l_LayerCount;
          vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

          Low::Util::List<VkLayerProperties> l_AvailableLayers(
              l_LayerCount);
          vkEnumerateInstanceLayerProperties(
              &l_LayerCount, l_AvailableLayers.data());
          for (const char *i_LayerName : g_ValidationLayers) {
            bool layer_found = false;

            for (const auto &i_LayerProperties : l_AvailableLayers) {
              if (strcmp(i_LayerName, i_LayerProperties.layerName) ==
                  0) {
                layer_found = true;
                break;
              }
            }
            if (!layer_found) {
              return false;
            }
          }
          return true;
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT p_MessageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT p_MessageType,
            const VkDebugUtilsMessengerCallbackDataEXT
                *p_CallbackData,
            void *p_UserData)
        {
          if (p_MessageSeverity ==
                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT &&
              !SKIP_DEBUG_LEVEL) {
            LOW_LOG_DEBUG
                << "Validation layer: " << p_CallbackData->pMessage
                << LOW_LOG_END;
          } else if (p_MessageSeverity ==
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            LOW_LOG_INFO
                << "Validation layer: " << p_CallbackData->pMessage
                << LOW_LOG_END;
          } else if (
              p_MessageSeverity ==
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            LOW_LOG_WARN
                << "Validation layer: " << p_CallbackData->pMessage
                << LOW_LOG_END;
          } else if (p_MessageSeverity ==
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            LOW_LOG_ERROR
                << "Validation layer: " << p_CallbackData->pMessage
                << LOW_LOG_END;
          }

          return VK_FALSE;
        }

        static void populate_debug_messenger_create_info(
            VkDebugUtilsMessengerCreateInfoEXT &p_CreateInfo)
        {
          p_CreateInfo = {};
          p_CreateInfo.sType =
              VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
          p_CreateInfo.messageSeverity =
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
          p_CreateInfo.messageType =
              VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
          p_CreateInfo.pfnUserCallback = debug_callback;
        }

        static VkResult create_debug_utils_messenger_ext(
            VkInstance p_Instance,
            const VkDebugUtilsMessengerCreateInfoEXT *p_CreateInfo,
            const VkAllocationCallbacks *p_Allocator,
            VkDebugUtilsMessengerEXT *p_DebugMessenger)
        {
          auto l_Function = (PFN_vkCreateDebugUtilsMessengerEXT)
              vkGetInstanceProcAddr(p_Instance,
                                    "vkCreateDebugUtilsMessengerEXT");

          if (l_Function != nullptr) {
            return l_Function(p_Instance, p_CreateInfo, p_Allocator,
                              p_DebugMessenger);
          } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
          }
        }

        static void destroy_debug_utils_messenger_ext(
            VkInstance p_Instance,
            VkDebugUtilsMessengerEXT p_DebugMessenger,
            const VkAllocationCallbacks *p_Allocator)
        {
          auto l_Function = (PFN_vkDestroyDebugUtilsMessengerEXT)
              vkGetInstanceProcAddr(
                  p_Instance, "vkDestroyDebugUtilsMessengerEXT");

          if (l_Function != nullptr) {
            l_Function(p_Instance, p_DebugMessenger, p_Allocator);
          }
        }

        static void create_instance(Context &p_Context)
        {
          // Setup appinfo parameter struct
          VkApplicationInfo l_AppInfo{};
          l_AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
          l_AppInfo.pApplicationName = "LoweR Test";
          l_AppInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
          l_AppInfo.pEngineName = "Low";
          l_AppInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
          l_AppInfo.apiVersion = VK_API_VERSION_1_2;

          // Setup createinfo parameter struct
          VkInstanceCreateInfo l_CreateInfo{};
          l_CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
          l_CreateInfo.pApplicationInfo = &l_AppInfo;

          // TL TODO: GLFW hardcoded
          // Load glfw extensions
          uint32_t l_GlfwExtensionCount = 0u;
          const char **l_GlfwExtensions;
          l_GlfwExtensions = glfwGetRequiredInstanceExtensions(
              &l_GlfwExtensionCount);
          Low::Util::List<const char *> l_Extensions(
              l_GlfwExtensions,
              l_GlfwExtensions + l_GlfwExtensionCount);

          if (p_Context.m_ValidationEnabled) {
            l_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
          }

          // Set the glfw extension data in the createinfo struct
          l_CreateInfo.enabledExtensionCount =
              static_cast<uint32_t>(l_Extensions.size());
          l_CreateInfo.ppEnabledExtensionNames = l_Extensions.data();

          VkDebugUtilsMessengerCreateInfoEXT l_DebugCreateInfo{};
          if (p_Context.m_ValidationEnabled) {
            l_CreateInfo.enabledLayerCount =
                static_cast<uint32_t>(g_ValidationLayers.size());
            l_CreateInfo.ppEnabledLayerNames =
                g_ValidationLayers.data();

            populate_debug_messenger_create_info(l_DebugCreateInfo);
            l_CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT
                                      *)&l_DebugCreateInfo;
          } else {
            l_CreateInfo.enabledLayerCount = 0;
            l_CreateInfo.pNext = nullptr;
          }

          VkResult l_Result = vkCreateInstance(&l_CreateInfo, nullptr,
                                               &p_Context.m_Instance);
          LOW_ASSERT(l_Result == VK_SUCCESS,
                     "Vulkan instance creation failed");
        }

        static void setup_debug_messenger(Context &p_Context)
        {
          if (!p_Context.m_ValidationEnabled)
            return;

          VkDebugUtilsMessengerCreateInfoEXT l_CreateInfo;
          populate_debug_messenger_create_info(l_CreateInfo);

          LOW_ASSERT(create_debug_utils_messenger_ext(
                         p_Context.m_Instance, &l_CreateInfo, nullptr,
                         &(p_Context.m_DebugMessenger)) == VK_SUCCESS,
                     "Failed to set up debug messenger");
        }

        static VkCommandBuffer
        get_current_commandbuffer(Backend::Context &p_Context)
        {
          return p_Context.vk
              .m_CommandBuffers[p_Context.currentFrameIndex];
        }

        static void create_surface(Backend::Context &p_Context)
        {
          // TL TODO: glfw hard coded
          LOW_ASSERT(glfwCreateWindowSurface(
                         p_Context.vk.m_Instance,
                         p_Context.window.m_Glfw, nullptr,
                         &(p_Context.vk.m_Surface)) == VK_SUCCESS,
                     "Failed to create window surface");
        }

        static bool
        check_device_extension_support(VkPhysicalDevice p_Device)
        {
          uint32_t l_ExtensionCount;
          vkEnumerateDeviceExtensionProperties(
              p_Device, nullptr, &l_ExtensionCount, nullptr);

          Util::List<VkExtensionProperties> l_AvailableExtensions(
              l_ExtensionCount);
          vkEnumerateDeviceExtensionProperties(
              p_Device, nullptr, &l_ExtensionCount,
              l_AvailableExtensions.data());

          Util::Set<std::string> l_RequiredExtensions(
              g_DeviceExtensions.begin(), g_DeviceExtensions.end());

          // Remove the found extensions from the list of required
          // extensions
          for (const auto &i_Extension : l_AvailableExtensions) {
            l_RequiredExtensions.erase(i_Extension.extensionName);
          }

          return l_RequiredExtensions.empty();
        }

        static int rate_device_suitability(Context &p_Context,
                                           VkPhysicalDevice p_Device)
        {
          VkPhysicalDeviceProperties l_DeviceProperties;
          VkPhysicalDeviceFeatures l_DeviceFeatures;

          vkGetPhysicalDeviceProperties(p_Device,
                                        &l_DeviceProperties);

          int l_Score = 0;

          if (l_DeviceProperties.apiVersion < VK_API_VERSION_1_2) {
            return 0;
          }

          VkPhysicalDeviceVulkan12Features vulkan12Features;
          vulkan12Features.sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
          vulkan12Features.pNext = nullptr;

          VkPhysicalDeviceFeatures2 features2;
          features2.sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
          features2.pNext = &vulkan12Features;

          vkGetPhysicalDeviceFeatures2(p_Device, &features2);

          if (!vulkan12Features.drawIndirectCount) {
            return 0;
          }

          vkGetPhysicalDeviceFeatures(p_Device, &l_DeviceFeatures);

          // Dedicated gpus have way better perfomance
          if (l_DeviceProperties.deviceType ==
              VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            l_Score += 1000;
          }

          // Maximum possible size of textures affects graphics
          // quality
          l_Score += l_DeviceProperties.limits.maxImageDimension2D;

          if (!l_DeviceFeatures.samplerAnisotropy) {
            return 0;
          }

          if (!l_DeviceFeatures.geometryShader) {
            return 0;
          }

          Helper::QueueFamilyIndices l_Indices =
              Helper::find_queue_families(p_Context, p_Device);
          if (!l_Indices.is_complete())
            return 0;

          if (!check_device_extension_support(p_Device))
            return 0;

          Helper::SwapChainSupportDetails l_SwapChainSupport =
              Helper::query_swap_chain_support(p_Context, p_Device);
          if (l_SwapChainSupport.m_Formats.empty() ||
              l_SwapChainSupport.m_PresentModes
                  .empty()) // TODO: Refactor to increase score based
                            // on the amount of modes/formats
            return 0; // Having 0 for either one of the two should
                      // result in a score of 0

          return l_Score;
        }

        static void select_physical_device(Context &p_Context)
        {
          uint32_t l_DeviceCount = 0u;
          vkEnumeratePhysicalDevices(p_Context.m_Instance,
                                     &l_DeviceCount, nullptr);

          LOW_ASSERT(l_DeviceCount > 0, "No physical GPU found");

          Low::Util::List<VkPhysicalDevice> l_Devices(l_DeviceCount);
          vkEnumeratePhysicalDevices(
              p_Context.m_Instance, &l_DeviceCount, l_Devices.data());

          Util::MultiMap<int, VkPhysicalDevice> l_Candidates;
          for (const auto &i_Device : l_Devices) {
            int l_Score =
                rate_device_suitability(p_Context, i_Device);
            l_Candidates.insert(eastl::make_pair(l_Score, i_Device));
          }

          LOW_ASSERT(l_Candidates.rbegin()->first > 0,
                     "No suitable physical GPU found");
          p_Context.m_PhysicalDevice = l_Candidates.rbegin()->second;
        }

        static void create_logical_device(Context &p_Context)
        {
          Helper::QueueFamilyIndices l_Indices =
              Helper::find_queue_families(p_Context,
                                          p_Context.m_PhysicalDevice);

          Low::Util::List<VkDeviceQueueCreateInfo> l_QueueCreateInfos;
          Low::Util::Set<uint32_t> l_UniqueQueueFamilies = {
              l_Indices.m_GraphicsFamily.value(),
              l_Indices.m_PresentFamily.value(),
              l_Indices.m_TransferFamily.value()};

          float l_QueuePriority = 1.f;
          for (uint32_t i_QueueFamily : l_UniqueQueueFamilies) {
            VkDeviceQueueCreateInfo i_QueueCreateInfo{};
            i_QueueCreateInfo.sType =
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            i_QueueCreateInfo.queueFamilyIndex = i_QueueFamily;
            i_QueueCreateInfo.queueCount = 1;
            i_QueueCreateInfo.pQueuePriorities = &l_QueuePriority;
            l_QueueCreateInfos.push_back(i_QueueCreateInfo);
          }

          VkPhysicalDeviceFeatures l_DeviceFeatures{};
          l_DeviceFeatures.samplerAnisotropy = VK_TRUE;
          l_DeviceFeatures.independentBlend = VK_TRUE;
          l_DeviceFeatures.fillModeNonSolid = VK_TRUE;
          l_DeviceFeatures.multiDrawIndirect = VK_TRUE;
          l_DeviceFeatures.wideLines = VK_TRUE;
          VkDeviceCreateInfo l_CreateInfo{};
          l_CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
          l_CreateInfo.pQueueCreateInfos = l_QueueCreateInfos.data();
          l_CreateInfo.queueCreateInfoCount =
              static_cast<uint32_t>(l_QueueCreateInfos.size());
          l_CreateInfo.pEnabledFeatures = &l_DeviceFeatures;
          l_CreateInfo.enabledExtensionCount =
              static_cast<uint32_t>(g_DeviceExtensions.size());
          l_CreateInfo.ppEnabledExtensionNames =
              g_DeviceExtensions.data();

          if (p_Context.m_ValidationEnabled) {
            l_CreateInfo.enabledLayerCount =
                static_cast<uint32_t>(g_ValidationLayers.size());
            l_CreateInfo.ppEnabledLayerNames =
                g_ValidationLayers.data();
          } else {
            l_CreateInfo.enabledLayerCount = 0;
          }

          LOW_ASSERT(vkCreateDevice(p_Context.m_PhysicalDevice,
                                    &l_CreateInfo, nullptr,
                                    &(p_Context.m_Device)) ==
                         VK_SUCCESS,
                     "Failed to create logical device");

          // Create queues
          vkGetDeviceQueue(p_Context.m_Device,
                           l_Indices.m_GraphicsFamily.value(), 0,
                           &(p_Context.m_GraphicsQueue));
          vkGetDeviceQueue(p_Context.m_Device,
                           l_Indices.m_PresentFamily.value(), 0,
                           &(p_Context.m_PresentQueue));
          vkGetDeviceQueue(p_Context.m_Device,
                           l_Indices.m_TransferFamily.value(), 0,
                           &(p_Context.m_TransferQueue));
        }

        static VkExtent2D choose_swap_extent(
            Backend::Context &p_Context,
            const VkSurfaceCapabilitiesKHR &p_Capabilities)
        {
          // If the extents are set to the max value of uint32_t then
          // we need some custom logic to determine the actualy size
          if (p_Capabilities.currentExtent.width != LOW_UINT32_MAX) {
            return p_Capabilities.currentExtent;
          }

          // Query the actualy extents from glfw
          int l_Width, l_Height;
          glfwGetFramebufferSize(p_Context.window.m_Glfw, &l_Width,
                                 &l_Height);

          VkExtent2D l_ActualExtent = {
              static_cast<uint32_t>(l_Width),
              static_cast<uint32_t>(l_Height)};

          // Clamp the extent to the min/max values of capabilities
          l_ActualExtent.width =
              Math::Util::clamp(l_ActualExtent.width,
                                p_Capabilities.minImageExtent.width,
                                p_Capabilities.maxImageExtent.width);
          l_ActualExtent.height =
              Math::Util::clamp(l_ActualExtent.height,
                                p_Capabilities.minImageExtent.height,
                                p_Capabilities.maxImageExtent.height);

          return l_ActualExtent;
        }

        static void
        create_render_targets(Backend::Context &p_Context,
                              Low::Util::List<VkImage> &p_Images,
                              bool p_UpdateExisting)
        {
          Context &l_Context = p_Context.vk;

          if (!p_UpdateExisting) {
            l_Context.m_SwapchainRenderTargets =
                (Backend::ImageResource *)
                    Util::Memory::main_allocator()
                        ->allocate(sizeof(Backend::ImageResource) *
                                   p_Images.size());
          }

          Math::UVector2 l_Dimensions = p_Context.dimensions;

          Backend::ImageResourceCreateParams l_Params;
          l_Params.context = &p_Context;
          l_Params.depth = false;
          l_Params.writable = false;
          l_Params.mip0Dimensions.x = l_Dimensions.x;
          l_Params.mip0Dimensions.y = l_Dimensions.y;
          l_Params.mip0Size = 0;
          l_Params.mip0Data = nullptr;
          l_Params.format = p_Context.imageFormat;
          l_Params.createImage = false;
          l_Params.sampleFilter = Backend::ImageSampleFilter::LINEAR;

          for (size_t i_Iter = 0; i_Iter < p_Images.size();
               i_Iter++) {
            l_Context.m_SwapchainRenderTargets[i_Iter].vk.m_Image =
                p_Images[i_Iter];

            imageresource_create_internal(
                l_Context.m_SwapchainRenderTargets[i_Iter], l_Params,
                !p_UpdateExisting);

            l_Context.m_SwapchainRenderTargets[i_Iter]
                .swapchainImage = true;
          }
        }

        static void create_sync_objects(Backend::Context &p_Context)

        {
          p_Context.vk.m_ImageAvailableSemaphores =
              (VkSemaphore *)Util::Memory::main_allocator()->allocate(
                  p_Context.framesInFlight * sizeof(VkSemaphore));
          p_Context.vk.m_RenderFinishedSemaphores =
              (VkSemaphore *)Util::Memory::main_allocator()->allocate(
                  p_Context.framesInFlight * sizeof(VkSemaphore));
          p_Context.vk.m_InFlightFences =
              (VkFence *)Util::Memory::main_allocator()->allocate(
                  p_Context.framesInFlight * sizeof(VkFence));

          VkSemaphoreCreateInfo l_SemaphoreInfo{};
          l_SemaphoreInfo.sType =
              VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

          VkFenceCreateInfo l_FenceInfo{};
          l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
          l_FenceInfo.flags =
              VK_FENCE_CREATE_SIGNALED_BIT; // Set to resolved
                                            // immidiately to not get
                                            // stuck on frame one

          for (size_t i_Iter = 0; i_Iter < p_Context.framesInFlight;
               i_Iter++) {
            LOW_ASSERT(
                vkCreateSemaphore(
                    p_Context.vk.m_Device, &l_SemaphoreInfo, nullptr,
                    &(p_Context.vk
                          .m_ImageAvailableSemaphores[i_Iter])) ==
                    VK_SUCCESS,
                "Failed to create semaphore");
            LOW_ASSERT(
                vkCreateSemaphore(
                    p_Context.vk.m_Device, &l_SemaphoreInfo, nullptr,
                    &(p_Context.vk
                          .m_RenderFinishedSemaphores[i_Iter])) ==
                    VK_SUCCESS,
                "Failed to create semaphore");
            LOW_ASSERT(
                vkCreateFence(
                    p_Context.vk.m_Device, &l_FenceInfo, nullptr,
                    &(p_Context.vk.m_InFlightFences[i_Iter])) ==
                    VK_SUCCESS,
                "Failed to create fence");
          }
        }

        static void create_staging_buffer(Backend::Context &p_Context)
        {
          VkSemaphoreCreateInfo l_SemaphoreInfo{};
          l_SemaphoreInfo.sType =
              VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

          LOW_ASSERT(vkCreateSemaphore(
                         p_Context.vk.m_Device, &l_SemaphoreInfo,
                         nullptr,
                         &p_Context.vk.m_StagingBufferSemaphore) ==
                         VK_SUCCESS,
                     "Failed to create staging buffer semaphore");

          VkFenceCreateInfo l_FenceInfo{};
          l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
          l_FenceInfo.flags =
              VK_FENCE_CREATE_SIGNALED_BIT; // Set to resolved
                                            // immidiately to not get
                                            // stuck on frame one

          LOW_ASSERT(vkCreateFence(
                         p_Context.vk.m_Device, &l_FenceInfo, nullptr,
                         &p_Context.vk.m_StagingBufferFence) ==
                         VK_SUCCESS,
                     "Failed to create staging buffer fence");

          p_Context.vk.m_StagingBufferUsage = 0u;
          p_Context.vk.m_ReadStagingBufferUsage = 0u;

          uint32_t l_StagingBufferSize = 2 * LOW_MEGABYTE_I;
          uint32_t l_ReadStagingBufferSize = 1 * LOW_KILOBYTE_I;

          p_Context.vk.m_StagingBufferSize = l_StagingBufferSize;
          Helper::create_buffer(
              p_Context, l_StagingBufferSize,
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              p_Context.vk.m_StagingBuffer,
              p_Context.vk.m_StagingBufferMemory);

          p_Context.vk.m_ReadStagingBufferSize =
              l_ReadStagingBufferSize;
          Helper::create_buffer(
              p_Context, l_ReadStagingBufferSize,
              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              p_Context.vk.m_ReadStagingBuffer,
              p_Context.vk.m_ReadStagingBufferMemory);

          VkCommandBufferAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType =
              VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          l_AllocInfo.commandPool =
              p_Context.vk.m_TransferCommandPool;
          l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          l_AllocInfo.commandBufferCount = 1;

          LOW_ASSERT(vkAllocateCommandBuffers(
                         p_Context.vk.m_Device, &l_AllocInfo,
                         &(p_Context.vk.m_TransferCommandBuffer)) ==
                         VK_SUCCESS,
                     "Failed to allocate transfer command buffer");
        }

        static void
        create_command_buffers(Backend::Context &p_Context)
        {
          p_Context.vk.m_CommandBuffers =
              (VkCommandBuffer *)Util::Memory::main_allocator()
                  ->allocate(p_Context.framesInFlight *
                             sizeof(VkCommandBuffer));

          VkCommandBufferAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType =
              VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          l_AllocInfo.commandPool = p_Context.vk.m_CommandPool;
          l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          l_AllocInfo.commandBufferCount = 1;

          for (uint8_t i = 0; i < p_Context.framesInFlight; ++i) {
            LOW_ASSERT(vkAllocateCommandBuffers(
                           p_Context.vk.m_Device, &l_AllocInfo,
                           &(p_Context.vk.m_CommandBuffers[i])) ==
                           VK_SUCCESS,
                       "Failed to allocate command buffer");
          }
        }

        static void create_swapchain(Backend::Context &p_Context,
                                     bool p_UpdateExisting = false)
        {
          Util::List<Math::Color> l_SwapchainClearColors = {
              {0.0f, 1.0f, 0.0f, 1.0f}};
          Context &l_Context = p_Context.vk;

          p_Context.currentFrameIndex = 0;

          Helper::SwapChainSupportDetails l_SwapChainSupportDetails =
              Helper::query_swap_chain_support(
                  l_Context, l_Context.m_PhysicalDevice);

          VkSurfaceFormatKHR l_SurfaceFormat =
              Helper::choose_swap_surface_format(
                  l_SwapChainSupportDetails.m_Formats);
          VkPresentModeKHR l_PresentMode =
              Helper::choose_swap_present_mode(
                  l_SwapChainSupportDetails.m_PresentModes);
          VkExtent2D l_Extent = ContextHelper::choose_swap_extent(
              p_Context, l_SwapChainSupportDetails.m_Capabilities);

          uint32_t l_ImageCount =
              l_SwapChainSupportDetails.m_Capabilities.minImageCount +
              1;

          p_Context.imageCount = l_ImageCount;

          // Clamp the imagecount to not exceed the maximum
          // A set maximum of 0 means that there is no maximum
          if (l_SwapChainSupportDetails.m_Capabilities.maxImageCount >
                  0 &&
              l_ImageCount > l_SwapChainSupportDetails.m_Capabilities
                                 .maxImageCount) {
            l_ImageCount = l_SwapChainSupportDetails.m_Capabilities
                               .maxImageCount;
          }

          VkSwapchainCreateInfoKHR l_CreateInfo{};
          l_CreateInfo.sType =
              VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
          l_CreateInfo.surface = l_Context.m_Surface;
          l_CreateInfo.minImageCount = l_ImageCount;
          l_CreateInfo.imageFormat = l_SurfaceFormat.format;
          l_CreateInfo.imageColorSpace = l_SurfaceFormat.colorSpace;
          l_CreateInfo.imageExtent = l_Extent;
          l_CreateInfo.imageArrayLayers = 1;
          l_CreateInfo.imageUsage =
              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

          Helper::QueueFamilyIndices l_Indices =
              Helper::find_queue_families(l_Context,
                                          l_Context.m_PhysicalDevice);
          uint32_t l_QueueFamilyIndices[] = {
              l_Indices.m_GraphicsFamily.value(),
              l_Indices.m_PresentFamily.value()};

          if (l_Indices.m_GraphicsFamily !=
              l_Indices.m_PresentFamily) {
            l_CreateInfo.imageSharingMode =
                VK_SHARING_MODE_CONCURRENT;
            l_CreateInfo.queueFamilyIndexCount = 2;
            l_CreateInfo.pQueueFamilyIndices = l_QueueFamilyIndices;
          } else {
            l_CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            l_CreateInfo.queueFamilyIndexCount = 0;
            l_CreateInfo.pQueueFamilyIndices = nullptr;
          }

          l_CreateInfo.preTransform =
              l_SwapChainSupportDetails.m_Capabilities
                  .currentTransform;
          l_CreateInfo.compositeAlpha =
              VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
          l_CreateInfo.presentMode = l_PresentMode;
          l_CreateInfo.clipped = VK_TRUE;
          l_CreateInfo.oldSwapchain = VK_NULL_HANDLE;

          // Create swap chain
          LOW_ASSERT(vkCreateSwapchainKHR(
                         l_Context.m_Device, &l_CreateInfo, nullptr,
                         &(l_Context.m_Swapchain)) == VK_SUCCESS,
                     "Could not create swap chain");

          // Retrieve the vkimages of the swapchain
          vkGetSwapchainImagesKHR(l_Context.m_Device,
                                  l_Context.m_Swapchain,
                                  &l_ImageCount, nullptr);

          Low::Util::List<VkImage> l_Images;
          l_Images.resize(l_ImageCount);
          vkGetSwapchainImagesKHR(l_Context.m_Device,
                                  l_Context.m_Swapchain,
                                  &l_ImageCount, l_Images.data());

          p_Context.imageFormat =
              Helper::vkformat_to_imageformat(l_SurfaceFormat.format);

          // Store some swapchain info
          p_Context.dimensions =
              Math::UVector2(l_Extent.width, l_Extent.height);

          ContextHelper::create_render_targets(p_Context, l_Images,
                                               p_UpdateExisting);

          if (!p_UpdateExisting) {
            p_Context.renderpasses =
                (Backend::Renderpass *)Util::Memory::main_allocator()
                    ->allocate(sizeof(Backend::Renderpass) *
                               p_Context.imageCount);
          }

          for (uint8_t i = 0; i < p_Context.imageCount; ++i) {
            Backend::RenderpassCreateParams l_RenderPassParams;
            l_RenderPassParams.context = &p_Context;
            l_RenderPassParams.renderTargetCount = 1;
            l_RenderPassParams.renderTargets =
                &(p_Context.vk.m_SwapchainRenderTargets[i]);
            l_RenderPassParams.clearTargetColor =
                l_SwapchainClearColors.data();
            l_RenderPassParams.useDepth = false;
            l_RenderPassParams.dimensions = p_Context.dimensions;

            if (i > 0) {
              p_Context.renderpasses[i].vk.m_Renderpass =
                  p_Context.renderpasses[0].vk.m_Renderpass;
            }

            renderpass_create_internal(p_Context.renderpasses[i],
                                       l_RenderPassParams, i == 0,
                                       p_UpdateExisting);

            p_Context.renderpasses[i].swapchainRenderpass = true;
          }
        }

        void check_vk_result(VkResult p_Result)
        {
          _LOW_ASSERT(p_Result == VK_SUCCESS);
        }

        static ImVec4 convert_rgb_to_imvec4(int r, int g, int b)
        {
          return ImVec4(((float)r) / 255.0f, ((float)g) / 255.0f,
                        ((float)b) / 255.0f, 1.0f);
        }

        static void setup_imgui(Backend::Context &p_Context)
        {
          // 1: create descriptor pool for IMGUI
          // the size of the pool is very oversize, but it's copied
          // from imgui demo itself.
          VkDescriptorPoolSize pool_sizes[] = {
              {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
              {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
              {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
              {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
              {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
              {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
              {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
              {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
              {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

          VkDescriptorPoolCreateInfo pool_info = {};
          pool_info.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
          pool_info.flags =
              VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
          pool_info.maxSets = 1000;
          pool_info.poolSizeCount = std::size(pool_sizes);
          pool_info.pPoolSizes = pool_sizes;

          LOW_ASSERT(vkCreateDescriptorPool(
                         p_Context.vk.m_Device, &pool_info, nullptr,
                         &(p_Context.vk.m_ImGuiDescriptorPool)) ==
                         VK_SUCCESS,
                     "Could not create imgui descriptorpool");

          IMGUI_CHECKVERSION();

          // this initializes the core structures of imgui
          ImGui::CreateContext();
          ImGuiIO &io = ImGui::GetIO();
          (void)io;
          io.ConfigFlags |=
              ImGuiConfigFlags_DockingEnable; // Enable Docking
          io.ConfigFlags |=
              ImGuiConfigFlags_ViewportsEnable; // Enable
                                                // Multi-Viewport /

          float baseFontSize =
              17.0f; // 13.0f is the size of the default font.
                     // Change to the font size you use.

          Util::String l_BaseInternalFontPath =
              Util::get_project().dataPath +
              "/_internal/assets/fonts/";

          {
            ImFontConfig config;
            config.OversampleH = 2;
            config.OversampleV = 1;
            config.GlyphExtraSpacing.x = 0.7f;

            Util::String l_FontPath =
                l_BaseInternalFontPath + "Roboto-Regular.ttf";
            io.Fonts->AddFontFromFileTTF(l_FontPath.c_str(),
                                         baseFontSize, &config);
          }

          // Icons
          {
            float iconFontSize =
                baseFontSize * 2.0f /
                2.0f; // FontAwesome fonts need to have their sizes
                      // reduced by 2.0f/3.0f in order to align
                      // correctly

            // merge in icons from Font Awesome
            static const ImWchar icons_ranges[] = {ICON_MIN_LC,
                                                   ICON_MAX_16_LC, 0};
            ImFontConfig icons_config;
            icons_config.MergeMode = true;
            icons_config.PixelSnapH = true;
            icons_config.GlyphMinAdvanceX = iconFontSize;
            // icons_config.GlyphExtraSpacing.y = 1.0f;
            icons_config.GlyphOffset.y = 3.0f;

            Util::String l_FontPath =
                l_BaseInternalFontPath + FONT_ICON_FILE_NAME_LC;
            io.Fonts->AddFontFromFileTTF(l_FontPath.c_str(),
                                         iconFontSize, &icons_config,
                                         icons_ranges);
          }

          ImGuiHelper::initialize();

          io.Fonts->Build();

          /*
          io.ConfigFlags |=
              ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard
          Controls
          // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
          Enable
          // Gamepad Controls
                  */

          ImGui::StyleColorsDark();

          // When viewports are enabled we tweak
          // WindowRounding/WindowBg so platform windows can look
          // identical to regular ones.
          ImGuiStyle &style = ImGui::GetStyle();
          if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 2.0f;
            style.ChildRounding = 2.0f;
            style.FrameRounding = 2.0f;
            style.PopupRounding = 2.0f;
            style.ScrollbarRounding = 2.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
          }

          style.ChildRounding = 2.0f;
          style.FrameRounding = 2.0f;
          style.PopupRounding = 2.0f;
          style.ScrollbarRounding = 2.0f;
          style.GrabRounding = 2.0f;
          style.TabRounding = 2.0f;

          style.WindowPadding = ImVec2(8, 8);
          style.FramePadding = ImVec2(5, 3);
          style.ItemSpacing = ImVec2(10, 4);
          style.ItemInnerSpacing = ImVec2(5, 3);

          {
            ImVec4 *colors = ImGui::GetStyle().Colors;
            colors[ImGuiCol_Text] =
                ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_TextDisabled] =
                ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_WindowBg] =
                ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_ChildBg] =
                ImVec4(0.19f, 0.19f, 0.19f, 0.00f);
            colors[ImGuiCol_PopupBg] =
                ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
            colors[ImGuiCol_Border] =
                ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
            colors[ImGuiCol_BorderShadow] =
                ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_FrameBg] =
                ImVec4(0.30f, 0.30f, 0.30f, 0.54f);
            colors[ImGuiCol_FrameBgHovered] =
                ImVec4(0.47f, 0.47f, 0.47f, 0.54f);
            colors[ImGuiCol_FrameBgActive] =
                ImVec4(0.47f, 0.47f, 0.47f, 0.54f);
            colors[ImGuiCol_TitleBg] =
                ImVec4(0.07f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_TitleBgActive] =
                ImVec4(0.07f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed] =
                ImVec4(0.07f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_MenuBarBg] =
                ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_ScrollbarBg] =
                ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_ScrollbarGrab] =
                ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered] =
                ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive] =
                ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_CheckMark] =
                ImVec4(1.00f, 1.00f, 1.00f, 0.54f);
            colors[ImGuiCol_SliderGrab] =
                ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
            colors[ImGuiCol_SliderGrabActive] =
                ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colors[ImGuiCol_Button] =
                ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
            colors[ImGuiCol_ButtonHovered] =
                ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
            colors[ImGuiCol_ButtonActive] =
                ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
            colors[ImGuiCol_Header] =
                ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
            colors[ImGuiCol_HeaderHovered] =
                ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
            colors[ImGuiCol_HeaderActive] =
                ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
            colors[ImGuiCol_Separator] =
                ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
            colors[ImGuiCol_SeparatorHovered] =
                ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colors[ImGuiCol_SeparatorActive] =
                ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colors[ImGuiCol_ResizeGrip] =
                ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
            colors[ImGuiCol_ResizeGripHovered] =
                ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            colors[ImGuiCol_ResizeGripActive] =
                ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            colors[ImGuiCol_Tab] = ImVec4(0.07f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_TabHovered] =
                ImVec4(0.39f, 0.41f, 0.43f, 1.00f);
            colors[ImGuiCol_TabActive] =
                ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_TabUnfocused] =
                ImVec4(0.07f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_TabUnfocusedActive] =
                ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_DockingPreview] =
                ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            colors[ImGuiCol_DockingEmptyBg] =
                ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colors[ImGuiCol_PlotLines] =
                ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered] =
                ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colors[ImGuiCol_PlotHistogram] =
                ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colors[ImGuiCol_PlotHistogramHovered] =
                ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colors[ImGuiCol_TableHeaderBg] =
                ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
            colors[ImGuiCol_TableBorderStrong] =
                ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
            colors[ImGuiCol_TableBorderLight] =
                ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
            colors[ImGuiCol_TableRowBg] =
                ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_TableRowBgAlt] =
                ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            colors[ImGuiCol_TextSelectedBg] =
                ImVec4(0.35f, 0.36f, 0.38f, 0.35f);
            colors[ImGuiCol_DragDropTarget] =
                ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colors[ImGuiCol_NavHighlight] =
                ImVec4(0.35f, 0.39f, 0.45f, 1.00f);
            colors[ImGuiCol_NavWindowingHighlight] =
                ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg] =
                ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colors[ImGuiCol_ModalWindowDimBg] =
                ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
          }
          {
            style.WindowBorderSize = 0.0f;
          }

          // this initializes imgui for glfw
          ImGui_ImplGlfw_InitForVulkan(p_Context.window.m_Glfw, true);

          // this initializes imgui for Vulkan
          ImGui_ImplVulkan_InitInfo init_info = {};
          init_info.Instance = p_Context.vk.m_Instance;
          init_info.PhysicalDevice = p_Context.vk.m_PhysicalDevice;
          init_info.Device = p_Context.vk.m_Device;
          init_info.Queue = p_Context.vk.m_GraphicsQueue;
          init_info.DescriptorPool =
              p_Context.vk.m_ImGuiDescriptorPool;
          init_info.Subpass = 0;
          init_info.Allocator = nullptr;
          init_info.PipelineCache = VK_NULL_HANDLE;
          init_info.MinImageCount = p_Context.imageCount;
          init_info.ImageCount = p_Context.imageCount;
          init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
          init_info.CheckVkResultFn = check_vk_result;

          ImGui_ImplVulkan_Init(
              &init_info, p_Context.renderpasses[0].vk.m_Renderpass);

          LOW_ASSERT(vkResetCommandPool(p_Context.vk.m_Device,
                                        p_Context.vk.m_CommandPool,
                                        0) == VK_SUCCESS,
                     "Failed to reset command pool");

          VkCommandBuffer l_CommandBuffer =
              Helper::begin_single_time_commands(p_Context);
          // execute a gpu command to upload imgui font textures
          ImGui_ImplVulkan_CreateFontsTexture(/* l_CommandBuffer*/);

          Helper::end_single_time_commands(p_Context,
                                           l_CommandBuffer);

          // ImGui_ImplVulkan_DestroyFontUploadObjects();
        }

        uint32_t
        staging_buffer_get_free_block(Backend::Context &p_Context,
                                      uint32_t p_Size)
        {
          if (p_Context.vk.m_StagingBufferSize <=
              p_Context.vk.m_StagingBufferUsage + p_Size) {

            LOW_ASSERT(
                p_Size <= p_Context.vk.m_StagingBufferSize,
                "Requested data does not fit in staging buffer");

            LOW_ASSERT(
                vkEndCommandBuffer(
                    p_Context.vk.m_TransferCommandBuffer) ==
                    VK_SUCCESS,
                "Failed to stop recording transfer command buffer");

            VkSubmitInfo l_TransferSubmitInfo{};
            l_TransferSubmitInfo.sType =
                VK_STRUCTURE_TYPE_SUBMIT_INFO;

            // VkSemaphore l_WaitSemaphores[] = {};
            VkPipelineStageFlags l_WaitStages[] = {
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
            l_TransferSubmitInfo.waitSemaphoreCount = 0;
            l_TransferSubmitInfo.pWaitSemaphores = nullptr;
            l_TransferSubmitInfo.pWaitDstStageMask = nullptr;
            l_TransferSubmitInfo.commandBufferCount = 1;
            l_TransferSubmitInfo.pCommandBuffers =
                &p_Context.vk.m_TransferCommandBuffer;

            l_TransferSubmitInfo.signalSemaphoreCount = 0;
            l_TransferSubmitInfo.pSignalSemaphores = nullptr;

            VkResult l_SubmitResult =
                vkQueueSubmit(p_Context.vk.m_TransferQueue, 1,
                              &l_TransferSubmitInfo,
                              p_Context.vk.m_StagingBufferFence);

            LOW_ASSERT(l_SubmitResult == VK_SUCCESS,
                       "Failed to submit transfer command buffer");

            vkWaitForFences(p_Context.vk.m_Device, 1,
                            &p_Context.vk.m_StagingBufferFence,
                            VK_TRUE, UINT64_MAX);

            vkResetFences(p_Context.vk.m_Device, 1,
                          &p_Context.vk.m_StagingBufferFence);

            p_Context.vk.m_StagingBufferUsage = 0;

            VkCommandBufferBeginInfo l_BeginInfo{};
            l_BeginInfo.sType =
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            l_BeginInfo.flags = 0;
            l_BeginInfo.pInheritanceInfo = nullptr;

            LOW_ASSERT(
                vkBeginCommandBuffer(
                    p_Context.vk.m_TransferCommandBuffer,
                    &l_BeginInfo) == VK_SUCCESS,
                "Failed to begin recording transfer command buffer");
          }

          uint32_t l_StartHolder = p_Context.vk.m_StagingBufferUsage;
          p_Context.vk.m_StagingBufferUsage += p_Size;

          return l_StartHolder;
        }

        uint32_t read_staging_buffer_get_free_block(
            Backend::Context &p_Context, uint32_t p_Size)
        {
          LOW_ASSERT(p_Context.vk.m_StagingBufferSize >
                         p_Context.vk.m_StagingBufferUsage + p_Size,
                     "Read staging buffer blown");

          uint32_t l_StartHolder =
              p_Context.vk.m_ReadStagingBufferUsage;
          p_Context.vk.m_ReadStagingBufferUsage += p_Size;

          return l_StartHolder;
        }

      } // namespace ContextHelper

      namespace BarrierHelper {
        void
        transition_image_barrier(VkCommandBuffer p_CommandBuffer,
                                 Backend::ImageResource &p_Image2D,
                                 VkImageLayout p_OldLayout,
                                 VkImageLayout p_NewLayout,
                                 VkAccessFlags p_SrcAccessMask,
                                 VkAccessFlags p_DstAccessMask,
                                 VkPipelineStageFlags p_SourceStage,
                                 VkPipelineStageFlags p_DstStage)
        {
          VkImageMemoryBarrier l_Barrier{};
          l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
          l_Barrier.oldLayout = p_OldLayout;
          l_Barrier.newLayout = p_NewLayout;
          l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barrier.image = p_Image2D.vk.m_Image;
          if (p_Image2D.depth) {
            l_Barrier.subresourceRange.aspectMask =
                VK_IMAGE_ASPECT_DEPTH_BIT;
          } else {
            l_Barrier.subresourceRange.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;
          }
          l_Barrier.subresourceRange.baseMipLevel = 0;
          l_Barrier.subresourceRange.levelCount = 1;
          l_Barrier.subresourceRange.baseArrayLayer = 0;
          l_Barrier.subresourceRange.layerCount = 1;
          l_Barrier.srcAccessMask = p_SrcAccessMask;
          l_Barrier.dstAccessMask = p_DstAccessMask;
          VkPipelineStageFlags l_SourceStage = p_SourceStage;
          VkPipelineStageFlags l_DestinationStage = p_DstStage;

          vkCmdPipelineBarrier(p_CommandBuffer, l_SourceStage,
                               l_DestinationStage, 0, 0, nullptr, 0,
                               nullptr, 1, &l_Barrier);
        }

        static void perform_resource_barrier_rendertarget(
            Backend::Context &p_Context, Resource::Image p_Image)
        {
          if (!p_Image.is_alive()) {
            return;
          }

          VkCommandBuffer l_CommandBuffer =
              ContextHelper::get_current_commandbuffer(p_Context);

          if (p_Image.get_image().vk.m_State ==
              ImageState::UNDEFINED) {
            if (!p_Image.get_image().depth) {
              transition_image_barrier(
                  l_CommandBuffer, p_Image.get_image(),
                  VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
              p_Image.get_image().vk.m_State = ImageState::UNDEFINED;
            }
            return;
          } else if (p_Image.get_image().vk.m_State ==
                         ImageState::DEPTH_STENCIL_ATTACHMENT &&
                     p_Image.get_image().depth) {
            return;
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::SHADER_READ_ONLY_OPTIMAL) {
            if (p_Image.get_image().depth) {
              transition_image_barrier(
                  l_CommandBuffer, p_Image.get_image(),
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                  VK_ACCESS_SHADER_READ_BIT, 0,
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
              p_Image.get_image().vk.m_State =
                  ImageState::DEPTH_STENCIL_ATTACHMENT;
            } else {
              transition_image_barrier(
                  l_CommandBuffer, p_Image.get_image(),
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                  VK_ACCESS_SHADER_READ_BIT, 0,
                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
            }
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::GENERAL) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_ACCESS_SHADER_WRITE_BIT, 0,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
            p_Image.get_image().vk.m_State = ImageState::UNDEFINED;
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::PRESENT_SRC) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 0,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
            p_Image.get_image().vk.m_State = ImageState::UNDEFINED;
          } else {
            LOW_ASSERT_WARN(false,
                            "Unsupported image state for transition");
          }
        }

        static void
        perform_resource_barrier_sampler(Backend::Context &p_Context,
                                         Resource::Image p_Image)
        {
          if (!p_Image.is_alive()) {
            return;
          }

          VkCommandBuffer l_CommandBuffer =
              ContextHelper::get_current_commandbuffer(p_Context);

          if (p_Image.get_image().vk.m_State ==
              ImageState::SHADER_READ_ONLY_OPTIMAL) {
            return;
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::UNDEFINED) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::DESTINATION_OPTIMAL) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::GENERAL) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::PRESENT_SRC) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::DEPTH_STENCIL_ATTACHMENT) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
          } else {
            LOW_ASSERT_WARN(false,
                            "Unsupported image state for transition");
          }
          p_Image.get_image().vk.m_State =
              ImageState::SHADER_READ_ONLY_OPTIMAL;
        }

        static void
        perform_resource_barrier_image(Backend::Context &p_Context,
                                       Resource::Image p_Image)
        {
          if (!p_Image.is_alive()) {
            return;
          }

          VkCommandBuffer l_CommandBuffer =
              ContextHelper::get_current_commandbuffer(p_Context);

          if (p_Image.get_image().vk.m_State == ImageState::GENERAL) {
            return;
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::UNDEFINED) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
          } else if (p_Image.get_image().vk.m_State ==
                     ImageState::SHADER_READ_ONLY_OPTIMAL) {
            transition_image_barrier(
                l_CommandBuffer, p_Image.get_image(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
          } else {
            LOW_ASSERT_WARN(false,
                            "Unsupported image state for transition");
          }
          p_Image.get_image().vk.m_State = ImageState::GENERAL;
        }
      } // namespace BarrierHelper

      void vk_context_create(Backend::Context &p_Context,
                             Backend::ContextCreateParams &p_Params)
      {
        p_Context.framesInFlight = p_Params.framesInFlight;
        p_Context.vk.m_PipelineResourceSignatureIndex = 0u;
        p_Context.vk.m_PipelineResourceSignatures =
            (PipelineResourceSignatureInternal *)
                Util::Memory::main_allocator()
                    ->allocate(
                        sizeof(PipelineResourceSignatureInternal) *
                        MAX_PRS);
        p_Context.vk.m_CommittedPipelineResourceSignatures =
            (uint32_t *)Util::Memory::main_allocator()->allocate(
                sizeof(uint32_t) *
                LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS);

        {
          // Check for validation layer support
          if (p_Params.validation_enabled) {
            LOW_ASSERT(
                ContextHelper::check_validation_layer_support(),
                "Validation layers requested, but not available");
            LOW_LOG_DEBUG << "Validation enabled" << LOW_LOG_END;
          }

          p_Context.vk.m_ValidationEnabled =
              p_Params.validation_enabled;
          p_Context.debugEnabled = p_Params.validation_enabled;
          p_Context.window = *(p_Params.window);

          ContextHelper::create_instance(p_Context.vk);

          ContextHelper::setup_debug_messenger(p_Context.vk);

          ContextHelper::create_surface(p_Context);

          ContextHelper::select_physical_device(p_Context.vk);

          ContextHelper::create_logical_device(p_Context.vk);
        }
        Helper::QueueFamilyIndices l_QueueFamilyIndices =
            Helper::find_queue_families(
                p_Context.vk, p_Context.vk.m_PhysicalDevice);

        VmaAllocatorCreateInfo l_AllocCreateInfo = {};
        l_AllocCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        l_AllocCreateInfo.physicalDevice =
            p_Context.vk.m_PhysicalDevice;
        l_AllocCreateInfo.device = p_Context.vk.m_Device;
        l_AllocCreateInfo.instance = p_Context.vk.m_Instance;

        vmaCreateAllocator(&l_AllocCreateInfo,
                           &(p_Context.vk.m_Alloc));

        // Create command pool
        {
          VkCommandPoolCreateInfo l_PoolInfo{};
          l_PoolInfo.sType =
              VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
          l_PoolInfo.flags =
              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
          l_PoolInfo.queueFamilyIndex =
              l_QueueFamilyIndices.m_GraphicsFamily.value();

          LOW_ASSERT(vkCreateCommandPool(
                         p_Context.vk.m_Device, &l_PoolInfo, nullptr,
                         &(p_Context.vk.m_CommandPool)) == VK_SUCCESS,
                     "Failed to create command pool");
        }

        {
          VkCommandPoolCreateInfo l_PoolInfo{};
          l_PoolInfo.sType =
              VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
          l_PoolInfo.flags =
              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
          l_PoolInfo.queueFamilyIndex =
              l_QueueFamilyIndices.m_TransferFamily.value();

          LOW_ASSERT(vkCreateCommandPool(
                         p_Context.vk.m_Device, &l_PoolInfo, nullptr,
                         &(p_Context.vk.m_TransferCommandPool)) ==
                         VK_SUCCESS,
                     "Failed to create transfer command pool");
        }

        ContextHelper::create_swapchain(p_Context);

        ContextHelper::create_sync_objects(p_Context);
        ContextHelper::create_staging_buffer(p_Context);
        ContextHelper::create_command_buffers(p_Context);

        {
          Util::List<VkDescriptorPoolSize> l_PoolSizes;
          {
            VkDescriptorPoolSize l_Size;
            l_Size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            l_Size.descriptorCount = 100;
            l_PoolSizes.push_back(l_Size);
          }
          {
            VkDescriptorPoolSize l_Size;
            l_Size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            l_Size.descriptorCount = 100;
            l_PoolSizes.push_back(l_Size);
          }
          {
            VkDescriptorPoolSize l_Size;
            l_Size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            l_Size.descriptorCount = 100;
            l_PoolSizes.push_back(l_Size);
          }
          {
            VkDescriptorPoolSize l_Size;
            l_Size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            l_Size.descriptorCount = 100;
            l_PoolSizes.push_back(l_Size);
          }

          VkDescriptorPoolCreateInfo l_PoolInfo{};
          l_PoolInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
          l_PoolInfo.poolSizeCount =
              static_cast<uint32_t>(l_PoolSizes.size());
          l_PoolInfo.pPoolSizes = l_PoolSizes.data();
          l_PoolInfo.maxSets = 100;

          LOW_ASSERT(vkCreateDescriptorPool(
                         p_Context.vk.m_Device, &l_PoolInfo, nullptr,
                         &(p_Context.vk.m_DescriptorPool)) ==
                         VK_SUCCESS,
                     "Failed to create descriptor pools");
        }

        p_Context.state = Backend::ContextState::SUCCESS;

        ContextHelper::setup_imgui(p_Context);
      }

      void vk_context_cleanup(Backend::Context &p_Context)
      {
        Context l_Context = p_Context.vk;

        for (uint32_t i = 0u; i < p_Context.framesInFlight; ++i) {
          vkDestroySemaphore(l_Context.m_Device,
                             l_Context.m_ImageAvailableSemaphores[i],
                             nullptr);
          vkDestroySemaphore(l_Context.m_Device,
                             l_Context.m_RenderFinishedSemaphores[i],
                             nullptr);
          vkDestroyFence(l_Context.m_Device,
                         l_Context.m_InFlightFences[i], nullptr);
        }

        Util::Memory::main_allocator()->deallocate(
            l_Context.m_ImageAvailableSemaphores);
        Util::Memory::main_allocator()->deallocate(
            l_Context.m_RenderFinishedSemaphores);
        Util::Memory::main_allocator()->deallocate(
            l_Context.m_InFlightFences);

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        vkDestroyDescriptorPool(l_Context.m_Device,
                                l_Context.m_ImGuiDescriptorPool,
                                nullptr);

        for (uint32_t i = 0u; i < p_Context.imageCount; ++i) {
          renderpass_cleanup_internal(p_Context.renderpasses[i],
                                      i == p_Context.imageCount - 1);
          vk_imageresource_cleanup(
              p_Context.vk.m_SwapchainRenderTargets[i]);
        }

        Util::Memory::main_allocator()->deallocate(
            p_Context.vk.m_SwapchainRenderTargets);
        Util::Memory::main_allocator()->deallocate(
            p_Context.renderpasses);

        vkDestroySwapchainKHR(l_Context.m_Device,
                              l_Context.m_Swapchain, nullptr);

        Util::Memory::main_allocator()->deallocate(
            p_Context.vk.m_CommittedPipelineResourceSignatures);

        for (uint32_t i = 0u;
             i < p_Context.vk.m_PipelineResourceSignatureIndex; ++i) {
          vkDestroyDescriptorSetLayout(
              l_Context.m_Device,
              l_Context.m_PipelineResourceSignatures[i]
                  .m_DescriptorSetLayout,
              nullptr);
          Util::Memory::main_allocator()->deallocate(
              p_Context.vk.m_PipelineResourceSignatures[i]
                  .m_DescriptorSets);
          if (p_Context.vk.m_PipelineResourceSignatures[i]
                  .m_BindingCount) {
            Util::Memory::main_allocator()->deallocate(
                p_Context.vk.m_PipelineResourceSignatures[i]
                    .m_Bindings);
          }
        }
        Util::Memory::main_allocator()->deallocate(
            p_Context.vk.m_PipelineResourceSignatures);

        Util::Memory::main_allocator()->deallocate(
            l_Context.m_CommandBuffers);

        {
          vkDestroyDescriptorPool(p_Context.vk.m_Device,
                                  p_Context.vk.m_DescriptorPool,
                                  nullptr);
          vkDestroyCommandPool(p_Context.vk.m_Device,
                               p_Context.vk.m_CommandPool, nullptr);
          vkDestroyCommandPool(p_Context.vk.m_Device,
                               p_Context.vk.m_TransferCommandPool,
                               nullptr);
        }

        vkDestroyBuffer(p_Context.vk.m_Device,
                        p_Context.vk.m_StagingBuffer, nullptr);
        vkFreeMemory(p_Context.vk.m_Device,
                     p_Context.vk.m_StagingBufferMemory, nullptr);

        vkDestroyBuffer(p_Context.vk.m_Device,
                        p_Context.vk.m_ReadStagingBuffer, nullptr);
        vkFreeMemory(p_Context.vk.m_Device,
                     p_Context.vk.m_ReadStagingBufferMemory, nullptr);

        vkDestroySemaphore(p_Context.vk.m_Device,
                           p_Context.vk.m_StagingBufferSemaphore,
                           nullptr);
        vkDestroyFence(p_Context.vk.m_Device,
                       p_Context.vk.m_StagingBufferFence, nullptr);

        vmaDestroyAllocator(p_Context.vk.m_Alloc);

        // Clean devices and instance
        {
          vkDestroyDevice(l_Context.m_Device, nullptr);

          vkDestroySurfaceKHR(l_Context.m_Instance,
                              l_Context.m_Surface, nullptr);

          if (l_Context.m_ValidationEnabled) {
            ContextHelper::destroy_debug_utils_messenger_ext(
                l_Context.m_Instance, l_Context.m_DebugMessenger,
                nullptr);
          }

          vkDestroyInstance(l_Context.m_Instance, nullptr);
        }
      }

      void vk_context_wait_idle(Backend::Context &p_Context)
      {
        vkDeviceWaitIdle(p_Context.vk.m_Device);
      }

      void vk_context_update_dimensions(Backend::Context &p_Context)
      {
        vk_context_wait_idle(p_Context);

        for (uint8_t i = 0u; i < p_Context.imageCount; ++i) {
          vkDestroyFramebuffer(
              p_Context.vk.m_Device,
              p_Context.renderpasses[i].vk.m_Framebuffer, nullptr);
          vkDestroyImageView(
              p_Context.vk.m_Device,
              p_Context.vk.m_SwapchainRenderTargets[i].vk.m_ImageView,
              nullptr);
        }

        vkDestroyRenderPass(p_Context.vk.m_Device,
                            p_Context.renderpasses[0].vk.m_Renderpass,
                            nullptr);

        vkDestroySwapchainKHR(p_Context.vk.m_Device,
                              p_Context.vk.m_Swapchain, nullptr);

        ContextHelper::create_swapchain(p_Context, true);
      }

      uint8_t vk_frame_prepare(Backend::Context &p_Context)
      {
        if (p_Context.debugEnabled) {
          LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
              p_Context, "Vulkan::PrepareFrame",
              Math::Color(0.4578f, 0.2478f, 0.9947f, 1.0f));
        }

        VkFence l_Fences[] = {
            p_Context.vk
                .m_InFlightFences[p_Context.currentFrameIndex],
            p_Context.vk.m_StagingBufferFence};
        {
          LOW_PROFILE_CPU("Renderer", "Wait fences");

          vkWaitForFences(p_Context.vk.m_Device, 2, l_Fences, VK_TRUE,
                          UINT64_MAX);
        }

        p_Context.vk.m_StagingBufferUsage = 0u;
        p_Context.vk.m_ReadStagingBufferUsage = 0u;

        p_Context.running = false;

        uint32_t l_CurrentImage;

        VkResult l_Result = vkAcquireNextImageKHR(
            p_Context.vk.m_Device, p_Context.vk.m_Swapchain,
            UINT64_MAX,
            p_Context.vk.m_ImageAvailableSemaphores
                [p_Context.currentFrameIndex],
            VK_NULL_HANDLE, &l_CurrentImage);

        p_Context.currentImageIndex = l_CurrentImage;

        // Handle window resize
        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR) {
          p_Context.state = Backend::ContextState::OUT_OF_DATE;
          return p_Context.state;
        }

        LOW_ASSERT(l_Result == VK_SUCCESS ||
                       l_Result == VK_SUBOPTIMAL_KHR,
                   "Failed to acquire swapchain image");

        p_Context.running = true;

        vkResetFences(p_Context.vk.m_Device, 2, l_Fences);

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = 0;
        l_BeginInfo.pInheritanceInfo = nullptr;

        if (p_Context.debugEnabled) {
          LOW_RENDERER_END_RENDERDOC_SECTION(p_Context);
        }

        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(p_Context);
        LOW_ASSERT(vkBeginCommandBuffer(l_CommandBuffer,
                                        &l_BeginInfo) == VK_SUCCESS,
                   "Failed to begin recording command buffer");

        LOW_ASSERT(
            vkBeginCommandBuffer(p_Context.vk.m_TransferCommandBuffer,
                                 &l_BeginInfo) == VK_SUCCESS,
            "Failed to begin recording transfer command buffer");

        p_Context.state = Backend::ContextState::SUCCESS;
        return p_Context.state;
      }

      void vk_imgui_prepare_frame(Backend::Context &p_Context)
      {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
      }

      void vk_imgui_render(Backend::Context &p_Context)
      {
        ImGui::Render();
        GLFWwindow *backup_current_context = glfwGetCurrentContext();

        ImGui_ImplVulkan_RenderDrawData(
            ImGui::GetDrawData(),
            ContextHelper::get_current_commandbuffer(p_Context));

        ImGui::UpdatePlatformWindows();

        ImGui::RenderPlatformWindowsDefault();

        glfwMakeContextCurrent(backup_current_context);
      }

      void vk_frame_render(Backend::Context &p_Context)
      {
        LOW_PROFILE_CPU("Renderer", "Vulkan::RenderFrame");
        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(p_Context);

        LOW_ASSERT(vkEndCommandBuffer(l_CommandBuffer) == VK_SUCCESS,
                   "Failed to stop recording the command buffer");

        {

          LOW_ASSERT(
              vkEndCommandBuffer(
                  p_Context.vk.m_TransferCommandBuffer) == VK_SUCCESS,
              "Failed to stop recording transfer command buffer");

          VkSubmitInfo l_TransferSubmitInfo{};
          l_TransferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

          // VkSemaphore l_WaitSemaphores[] = {};
          VkPipelineStageFlags l_WaitStages[] = {
              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
          l_TransferSubmitInfo.waitSemaphoreCount = 0;
          l_TransferSubmitInfo.pWaitSemaphores = nullptr;
          l_TransferSubmitInfo.pWaitDstStageMask = nullptr;
          l_TransferSubmitInfo.commandBufferCount = 1;
          l_TransferSubmitInfo.pCommandBuffers =
              &p_Context.vk.m_TransferCommandBuffer;

          VkSemaphore l_SignalSemaphores[] = {
              p_Context.vk.m_StagingBufferSemaphore};
          l_TransferSubmitInfo.signalSemaphoreCount = 1;
          l_TransferSubmitInfo.pSignalSemaphores = l_SignalSemaphores;

          VkResult l_SubmitResult = vkQueueSubmit(
              p_Context.vk.m_TransferQueue, 1, &l_TransferSubmitInfo,
              p_Context.vk.m_StagingBufferFence);

          LOW_ASSERT(l_SubmitResult == VK_SUCCESS,
                     "Failed to submit transfer command buffer");
        }

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore l_WaitSemaphores[] = {
            p_Context.vk.m_ImageAvailableSemaphores
                [p_Context.currentFrameIndex],
            p_Context.vk.m_StagingBufferSemaphore};
        VkPipelineStageFlags l_WaitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        l_SubmitInfo.waitSemaphoreCount = 2;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;

        VkSemaphore l_SignalSemaphores[] = {
            p_Context.vk.m_RenderFinishedSemaphores
                [p_Context.currentFrameIndex]};
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

        VkResult l_SubmitResult = vkQueueSubmit(
            p_Context.vk.m_GraphicsQueue, 1, &l_SubmitInfo,
            p_Context.vk
                .m_InFlightFences[p_Context.currentFrameIndex]);

        LOW_ASSERT(l_SubmitResult == VK_SUCCESS,
                   "Failed to submit draw command buffer");

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = l_SignalSemaphores;

        VkSwapchainKHR l_Swapchains[] = {p_Context.vk.m_Swapchain};
        l_PresentInfo.swapchainCount = 1;
        l_PresentInfo.pSwapchains = l_Swapchains;
        uint32_t l_ImageIndex = p_Context.currentImageIndex;
        l_PresentInfo.pImageIndices = &l_ImageIndex;
        l_PresentInfo.pResults = nullptr;

        VkResult l_Result = vkQueuePresentKHR(
            p_Context.vk.m_PresentQueue, &l_PresentInfo);

        // Handle window resize
        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR ||
            l_Result == VK_SUBOPTIMAL_KHR ||
            /*g_FramebufferResized*/ false) {
          // g_FramebufferResized = false;
          // recreate_swapchain();
          // TODO: Handle reconfigure renderer
          // UPDATE: I don't think we need to do anything actually...
          // it has been working like this for a while...
        } else {
          LOW_ASSERT(l_Result == VK_SUCCESS,
                     "Failed to present swapchain image");
        }
        p_Context.running = false;

        p_Context.currentFrameIndex =
            (p_Context.currentFrameIndex + 1) %
            p_Context.framesInFlight;

        p_Context.vk.m_BoundPipelineLayout = VK_NULL_HANDLE;
      }

      void renderpass_create_internal(
          Backend::Renderpass &p_Renderpass,
          Backend::RenderpassCreateParams &p_Params,
          bool p_CreateVulkanRenderpass, bool p_UpdateExisting)
      {
        p_Renderpass.context = p_Params.context;
        p_Renderpass.dimensions = p_Params.dimensions;
        p_Renderpass.clearDepthColor = p_Params.clearDepthColor;
        p_Renderpass.renderTargetCount = p_Params.renderTargetCount;
        p_Renderpass.useDepth = p_Params.useDepth;
        if (!p_UpdateExisting) {
          p_Renderpass.renderTargets = nullptr;
          p_Renderpass.clearTargetColor = nullptr;
          if (p_Params.renderTargetCount > 0) {
            p_Renderpass.renderTargets =
                (Backend::ImageResource *)
                    Util::Memory::main_allocator()
                        ->allocate(p_Params.renderTargetCount *
                                   sizeof(Backend::ImageResource));

            p_Renderpass.clearTargetColor =
                (Math::Color *)Util::Memory::main_allocator()
                    ->allocate(p_Params.renderTargetCount *
                               sizeof(Math::Color));
          }
        }

        Low::Util::List<VkAttachmentDescription> l_Attachments;
        Low::Util::List<VkAttachmentReference> l_ColorAttachmentRefs;

        VkAttachmentDescription l_DepthAttachment{};
        VkAttachmentReference l_DepthAttachmentRef{};

        VkSubpassDescription l_Subpass{};
        l_Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        for (uint32_t i = 0u; i < p_Params.renderTargetCount; ++i) {
          p_Renderpass.clearTargetColor[i] =
              p_Params.clearTargetColor[i];
          p_Renderpass.renderTargets[i] = p_Params.renderTargets[i];

          bool i_Clear = p_Params.clearTargetColor[i].a > 0.0f;

          VkAttachmentDescription l_ColorAttachment{};
          l_ColorAttachment.format = Helper::imageformat_to_vkformat(
              p_Renderpass.renderTargets[i].format);
          l_ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
          l_ColorAttachment.loadOp = i_Clear
                                         ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                         : VK_ATTACHMENT_LOAD_OP_LOAD;
          l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          l_ColorAttachment.stencilLoadOp =
              VK_ATTACHMENT_LOAD_OP_LOAD;
          l_ColorAttachment.stencilStoreOp =
              VK_ATTACHMENT_STORE_OP_STORE;
          l_ColorAttachment.initialLayout =
              i_Clear ? VK_IMAGE_LAYOUT_UNDEFINED
                      : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
          l_ColorAttachment.finalLayout =
              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

          l_Attachments.push_back(l_ColorAttachment);

          VkAttachmentReference l_ColorAttachmentRef{};
          l_ColorAttachmentRef.attachment = i;
          l_ColorAttachmentRef.layout =
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

          l_ColorAttachmentRefs.push_back(l_ColorAttachmentRef);
        }

        if (p_Params.useDepth) {
          p_Renderpass.depthRenderTargetHandleId =
              p_Params.depthRenderTarget->handleId;
          l_DepthAttachment.format =
              Helper::vkformat_get_depth(*p_Params.context);
          l_DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

          bool l_ClearDepth = p_Params.clearDepthColor.g > 0.0f;

          l_DepthAttachment.loadOp = l_ClearDepth
                                         ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                         : VK_ATTACHMENT_LOAD_OP_LOAD;
          l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          l_DepthAttachment.stencilLoadOp =
              VK_ATTACHMENT_LOAD_OP_LOAD;
          l_DepthAttachment.stencilStoreOp =
              VK_ATTACHMENT_STORE_OP_STORE;
          l_DepthAttachment.initialLayout =
              l_ClearDepth
                  ? VK_IMAGE_LAYOUT_UNDEFINED
                  : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
          l_DepthAttachment.finalLayout =
              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

          l_DepthAttachmentRef.attachment =
              p_Params.renderTargetCount;
          l_DepthAttachmentRef.layout =
              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

          l_Attachments.push_back(l_DepthAttachment);

          l_Subpass.pDepthStencilAttachment = &l_DepthAttachmentRef;
        } else {
          l_Subpass.pDepthStencilAttachment = nullptr;
        }

        l_Subpass.colorAttachmentCount =
            static_cast<uint32_t>(l_ColorAttachmentRefs.size());
        l_Subpass.pColorAttachments = l_ColorAttachmentRefs.data();

        Util::List<VkSubpassDependency> l_Dependencies;

        {
          VkSubpassDependency l_Dependency{};
          l_Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
          l_Dependency.dstSubpass = 0;
          if (p_Params.useDepth) {
            l_Dependency.srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
          } else {
            l_Dependency.srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
          }
          l_Dependency.srcAccessMask = 0;
          if (p_Params.useDepth) {
            l_Dependency.dstStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            l_Dependency.dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
          } else {
            l_Dependency.dstStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            l_Dependency.dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
          }
          l_Dependencies.push_back(l_Dependency);
        }

        VkRenderPassCreateInfo l_RenderpassInfo{};
        l_RenderpassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        l_RenderpassInfo.attachmentCount =
            static_cast<uint32_t>(l_Attachments.size());
        l_RenderpassInfo.pAttachments = l_Attachments.data();
        l_RenderpassInfo.subpassCount = 1;
        l_RenderpassInfo.pSubpasses = &l_Subpass;
        l_RenderpassInfo.dependencyCount = l_Dependencies.size();
        l_RenderpassInfo.pDependencies = l_Dependencies.data();

        if (p_CreateVulkanRenderpass) {
          LOW_ASSERT(
              vkCreateRenderPass(p_Params.context->vk.m_Device,
                                 &l_RenderpassInfo, nullptr,
                                 &(p_Renderpass.vk.m_Renderpass)) ==
                  VK_SUCCESS,
              "Failed to create render pass");
        }

        {
          Util::List<VkImageView> l_Attachments;
          l_Attachments.resize(p_Params.renderTargetCount +
                               (p_Params.useDepth ? 1 : 0));

          for (int i_Iter = 0; i_Iter < p_Params.renderTargetCount;
               i_Iter++) {
            l_Attachments[i_Iter] =
                p_Params.renderTargets[i_Iter].vk.m_ImageView;
          }

          if (p_Params.useDepth) {
            l_Attachments[p_Params.renderTargetCount] =
                p_Params.depthRenderTarget->vk.m_ImageView;
          }

          VkFramebufferCreateInfo l_FramebufferInfo{};
          l_FramebufferInfo.sType =
              VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
          l_FramebufferInfo.renderPass = p_Renderpass.vk.m_Renderpass;
          l_FramebufferInfo.attachmentCount =
              static_cast<uint32_t>(l_Attachments.size());
          l_FramebufferInfo.pAttachments = l_Attachments.data();
          l_FramebufferInfo.width = p_Params.dimensions.x;
          l_FramebufferInfo.height = p_Params.dimensions.y;
          l_FramebufferInfo.layers = 1;

          LOW_ASSERT(
              vkCreateFramebuffer(p_Params.context->vk.m_Device,
                                  &l_FramebufferInfo, nullptr,
                                  &(p_Renderpass.vk.m_Framebuffer)) ==
                  VK_SUCCESS,
              "Failed to create framebuffer");
        }

        p_Renderpass.swapchainRenderpass = false;
      }

      void
      vk_renderpass_create(Backend::Renderpass &p_Renderpass,
                           Backend::RenderpassCreateParams &p_Params)
      {
        renderpass_create_internal(p_Renderpass, p_Params, true,
                                   false);
      }

      void
      renderpass_cleanup_internal(Backend::Renderpass &p_Renderpass,
                                  bool p_ClearVulkanRenderpass)
      {
        vkDestroyFramebuffer(p_Renderpass.context->vk.m_Device,
                             p_Renderpass.vk.m_Framebuffer, nullptr);

        if (p_ClearVulkanRenderpass) {
          vkDestroyRenderPass(p_Renderpass.context->vk.m_Device,
                              p_Renderpass.vk.m_Renderpass, nullptr);
        }

        Util::Memory::main_allocator()->deallocate(
            p_Renderpass.clearTargetColor);
        Util::Memory::main_allocator()->deallocate(
            p_Renderpass.renderTargets);
      }

      void vk_renderpass_cleanup(Backend::Renderpass &p_Renderpass)
      {
        renderpass_cleanup_internal(p_Renderpass, true);
      }

      static void vk_renderpass_perform_barriers(
          Backend::Renderpass &p_Renderpass)
      {
        vk_perform_barriers(*p_Renderpass.context);

        if (p_Renderpass.useDepth) {
          Resource::Image l_Image =
              p_Renderpass.depthRenderTargetHandleId;
          BarrierHelper::perform_resource_barrier_rendertarget(
              *p_Renderpass.context, l_Image);
        }

        for (uint32_t i = 0u; i < p_Renderpass.renderTargetCount;
             ++i) {
          BarrierHelper::perform_resource_barrier_rendertarget(
              *p_Renderpass.context,
              p_Renderpass.renderTargets[i].handleId);
        }
      }

      void vk_renderpass_begin(Backend::Renderpass &p_Renderpass)
      {
        LOW_PROFILE_CPU("Renderer", "Vulkan::RenderpassBegin");

        vk_renderpass_perform_barriers(p_Renderpass);

        if (p_Renderpass.swapchainRenderpass) {
          for (Interface::ImGuiImage i_ImGuiImage :
               Interface::ImGuiImage::ms_LivingInstances) {
            BarrierHelper::perform_resource_barrier_sampler(
                *p_Renderpass.context, i_ImGuiImage.get_image());
          }
        }

        VkRenderPassBeginInfo l_RenderpassInfo{};
        l_RenderpassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        l_RenderpassInfo.renderPass = p_Renderpass.vk.m_Renderpass;
        l_RenderpassInfo.framebuffer = p_Renderpass.vk.m_Framebuffer;
        l_RenderpassInfo.renderArea.offset = {0, 0};

        VkExtent2D l_ActualExtent = {
            static_cast<uint32_t>(p_Renderpass.dimensions.x),
            static_cast<uint32_t>(p_Renderpass.dimensions.y)};

        l_RenderpassInfo.renderArea.extent = l_ActualExtent;

        Low::Util::List<VkClearValue> l_ClearValues;

        for (uint32_t i = 0u; i < p_Renderpass.renderTargetCount;
             ++i) {
          VkClearValue l_ClearColor = {
              {{p_Renderpass.clearTargetColor[i].r,
                p_Renderpass.clearTargetColor[i].g,
                p_Renderpass.clearTargetColor[i].b,
                p_Renderpass.clearTargetColor[i].a}}};
          l_ClearValues.push_back(l_ClearColor);

          Resource::Image i_Image(
              p_Renderpass.renderTargets[i].handleId);

          if (i_Image.is_alive()) {
            i_Image.get_image().vk.m_State = ImageState::PRESENT_SRC;
          }
        }
        if (p_Renderpass.clearDepthColor.y > 0.0f) {
          l_ClearValues.push_back({p_Renderpass.clearDepthColor.r,
                                   p_Renderpass.clearDepthColor.y});
        }

        l_RenderpassInfo.clearValueCount =
            static_cast<uint32_t>(l_ClearValues.size());
        l_RenderpassInfo.pClearValues = l_ClearValues.data();

        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(
                *p_Renderpass.context);

        if (p_Renderpass.useDepth) {
          Resource::Image l_Image(
              p_Renderpass.depthRenderTargetHandleId);

          if (l_Image.is_alive()) {
            l_Image.get_image().vk.m_State =
                ImageState::DEPTH_STENCIL_ATTACHMENT;
          }
        }

        {
          LOW_PROFILE_CPU("Renderer", "Vulkan::RecordCommands");

          vkCmdBeginRenderPass(l_CommandBuffer, &l_RenderpassInfo,
                               VK_SUBPASS_CONTENTS_INLINE);

          VkViewport l_Viewport{};
          l_Viewport.x = 0.f;
          l_Viewport.y = 0.f;
          l_Viewport.width = static_cast<float>(l_ActualExtent.width);
          l_Viewport.height =
              static_cast<float>(l_ActualExtent.height);
          l_Viewport.minDepth = 0.f;
          l_Viewport.maxDepth = 1.f;
          vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);

          VkRect2D l_Scissor{};
          l_Scissor.offset = {0, 0};
          l_Scissor.extent = l_ActualExtent;
          vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);
        }
      }

      void vk_renderpass_end(Backend::Renderpass &p_Renderpass)
      {
        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(
                *p_Renderpass.context);
        vkCmdEndRenderPass(l_CommandBuffer);
      }

      namespace ImageHelper {
        static void
        create_image_view(Backend::Context &p_Context,
                          Backend::ImageResource &p_Image2d,
                          uint8_t &p_Format, bool p_Depth)
        {
          VkImageViewCreateInfo l_CreateInfo{};
          l_CreateInfo.sType =
              VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
          l_CreateInfo.image = p_Image2d.vk.m_Image;
          l_CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
          l_CreateInfo.format =
              Helper::imageformat_to_vkformat(p_Format);

          l_CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
          l_CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
          l_CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
          l_CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

          l_CreateInfo.subresourceRange.aspectMask =
              p_Depth ? VK_IMAGE_ASPECT_DEPTH_BIT
                      : VK_IMAGE_ASPECT_COLOR_BIT;
          l_CreateInfo.subresourceRange.baseMipLevel = 0;
          l_CreateInfo.subresourceRange.levelCount = 1;
          l_CreateInfo.subresourceRange.baseArrayLayer = 0;
          l_CreateInfo.subresourceRange.layerCount = 1;

          LOW_ASSERT(vkCreateImageView(p_Context.vk.m_Device,
                                       &l_CreateInfo, nullptr,
                                       &(p_Image2d.vk.m_ImageView)) ==
                         VK_SUCCESS,
                     "Could not create image view");
        }

        static void create_2d_sampler(Backend::Context &p_Context,
                                      Backend::ImageResource &p_Image)
        {
          VkSamplerCreateInfo l_SamplerInfo{};
          l_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
          if (p_Image.sampleFilter ==
              Backend::ImageSampleFilter::LINEAR) {
            l_SamplerInfo.magFilter = VK_FILTER_LINEAR;
            l_SamplerInfo.minFilter = VK_FILTER_LINEAR;
          } else if (p_Image.sampleFilter ==
                     Backend::ImageSampleFilter::CUBIC) {
            l_SamplerInfo.magFilter = VK_FILTER_CUBIC_IMG;
            l_SamplerInfo.minFilter = VK_FILTER_CUBIC_IMG;
          } else {
            LOW_ASSERT(false, "Unknown image sample filter");
          }
#if 0
          l_SamplerInfo.addressModeU =
              VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
          l_SamplerInfo.addressModeV =
              VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
          l_SamplerInfo.addressModeW =
              VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
#else
          l_SamplerInfo.addressModeU =
              VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
          l_SamplerInfo.addressModeV =
              VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
          l_SamplerInfo.addressModeW =
              VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
#endif

          // l_SamplerInfo.addressModeU =
          // VK_SAMPLER_ADDRESS_MODE_REPEAT;
          // l_SamplerInfo.addressModeV =
          // VK_SAMPLER_ADDRESS_MODE_REPEAT;
          // l_SamplerInfo.addressModeW =
          // VK_SAMPLER_ADDRESS_MODE_REPEAT;
          l_SamplerInfo.anisotropyEnable = VK_TRUE;

          VkPhysicalDeviceProperties l_Properties{};
          vkGetPhysicalDeviceProperties(p_Context.vk.m_PhysicalDevice,
                                        &l_Properties);

          l_SamplerInfo.maxAnisotropy =
              l_Properties.limits.maxSamplerAnisotropy;
          l_SamplerInfo.borderColor =
              VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
          l_SamplerInfo.unnormalizedCoordinates = VK_FALSE;
          l_SamplerInfo.compareEnable = VK_TRUE;
          l_SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
          l_SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
          l_SamplerInfo.mipLodBias = 0.f;
          l_SamplerInfo.minLod = 0.f;
          l_SamplerInfo.maxLod = 0.f;

          LOW_ASSERT(vkCreateSampler(p_Context.vk.m_Device,
                                     &l_SamplerInfo, nullptr,
                                     &(p_Image.vk.m_Sampler)) ==
                         VK_SUCCESS,
                     "Failed to create image2d sampler");
        }

        void copy_buffer_to_image(Backend::ImageResource &p_Image,
                                  VkBuffer p_Buffer, uint32_t p_Width,
                                  uint32_t p_Height)
        {
          VkCommandBuffer l_CommandBuffer =
              Helper::begin_single_time_commands(*p_Image.context);

          VkBufferImageCopy l_Region{};
          l_Region.bufferOffset = 0;
          l_Region.bufferRowLength = 0;
          l_Region.bufferImageHeight = 0;

          l_Region.imageSubresource.aspectMask =
              VK_IMAGE_ASPECT_COLOR_BIT;
          l_Region.imageSubresource.mipLevel = 0;
          l_Region.imageSubresource.baseArrayLayer = 0;
          l_Region.imageSubresource.layerCount = 1;

          l_Region.imageOffset = {0, 0, 0};
          l_Region.imageExtent = {p_Width, p_Height, 1};

          vkCmdCopyBufferToImage(
              l_CommandBuffer, p_Buffer, p_Image.vk.m_Image,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Region);

          Helper::end_single_time_commands(*p_Image.context,
                                           l_CommandBuffer);
        }

        static void create_image(Backend::ImageResource &p_Image,
                                 uint32_t p_Width, uint32_t p_Height,
                                 VkFormat p_Format,
                                 VkImageTiling p_Tiling,
                                 VkImageUsageFlags p_Usage,
                                 VkMemoryPropertyFlags p_Properties,
                                 void *p_ImageData,
                                 size_t p_ImageDataSize)
        {
          VkBuffer l_StagingBuffer;
          VkDeviceMemory l_StagingBufferMemory;

          if (p_ImageData) {
            Helper::create_buffer(
                *p_Image.context, p_ImageDataSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                l_StagingBuffer, l_StagingBufferMemory);

            void *l_Data;
            vkMapMemory(p_Image.context->vk.m_Device,
                        l_StagingBufferMemory, 0, p_ImageDataSize, 0,
                        &l_Data);
            memcpy(l_Data, p_ImageData, p_ImageDataSize);
            vkUnmapMemory(p_Image.context->vk.m_Device,
                          l_StagingBufferMemory);
          }

          VkImageCreateInfo l_ImageInfo{};
          l_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
          l_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
          l_ImageInfo.extent.width = p_Width;
          l_ImageInfo.extent.height = p_Height;
          l_ImageInfo.extent.depth = 1;
          l_ImageInfo.mipLevels = 1;
          l_ImageInfo.arrayLayers = 1;
          l_ImageInfo.format = p_Format;
          l_ImageInfo.tiling = p_Tiling;
          l_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          l_ImageInfo.usage = p_Usage;
          l_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
          l_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
          l_ImageInfo.flags = 0;

          LOW_ASSERT(vkCreateImage(p_Image.context->vk.m_Device,
                                   &l_ImageInfo, nullptr,
                                   &(p_Image.vk.m_Image)) ==
                         VK_SUCCESS,
                     "Failed to create image");

          VkMemoryRequirements l_MemRequirements;
          vkGetImageMemoryRequirements(p_Image.context->vk.m_Device,
                                       p_Image.vk.m_Image,
                                       &l_MemRequirements);

          VkMemoryAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
          l_AllocInfo.allocationSize = l_MemRequirements.size;
          l_AllocInfo.memoryTypeIndex = Helper::find_memory_type(
              p_Image.context->vk, l_MemRequirements.memoryTypeBits,
              p_Properties);

          LOW_ASSERT(vkAllocateMemory(p_Image.context->vk.m_Device,
                                      &l_AllocInfo, nullptr,
                                      &(p_Image.vk.m_Memory)) ==
                         VK_SUCCESS,
                     "Failed to allocate image memory");

          vkBindImageMemory(p_Image.context->vk.m_Device,
                            p_Image.vk.m_Image, p_Image.vk.m_Memory,
                            0);

          if (p_ImageData) {
            VkCommandBuffer l_CommandBuffer =
                Helper::begin_single_time_commands(*p_Image.context);

            {
              BarrierHelper::transition_image_barrier(
                  l_CommandBuffer, p_Image, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                  VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT);
              p_Image.vk.m_State = ImageState::DESTINATION_OPTIMAL;
            }
            Helper::end_single_time_commands(*p_Image.context,
                                             l_CommandBuffer);

            copy_buffer_to_image(p_Image, l_StagingBuffer,
                                 p_Image.dimensions.x,
                                 p_Image.dimensions.y);

            vkDestroyBuffer(p_Image.context->vk.m_Device,
                            l_StagingBuffer, nullptr);
            vkFreeMemory(p_Image.context->vk.m_Device,
                         l_StagingBufferMemory, nullptr);
          }
        }
      } // namespace ImageHelper

      void imageresource_create_internal(
          Backend::ImageResource &p_Image,
          Backend::ImageResourceCreateParams &p_Params,
          bool p_CreateSampler)
      {
        p_Image.context = p_Params.context;
        p_Image.format = p_Params.format;
        p_Image.sampleFilter = p_Params.sampleFilter;
        p_Image.dimensions = p_Params.mip0Dimensions;
        p_Image.depth = false;
        p_Image.vk.m_State = ImageState::UNDEFINED;

        p_Image.handleId = 0ull;

        VkImageUsageFlags l_UsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (p_Params.depth) {
          l_UsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                         VK_IMAGE_USAGE_SAMPLED_BIT |
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT;
          p_Image.depth = true;
        }

        if (p_Params.writable && !p_Params.depth) {
          l_UsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if (p_Params.createImage) {
          // MARK
          ImageHelper::create_image(
              p_Image, p_Params.mip0Dimensions.x,
              p_Params.mip0Dimensions.y,
              Helper::imageformat_to_vkformat(p_Params.format),
              VK_IMAGE_TILING_OPTIMAL, l_UsageFlags,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_Params.mip0Data,
              p_Params.mip0Size);
        }

        ImageHelper::create_image_view(*p_Params.context, p_Image,
                                       p_Image.format,
                                       p_Params.depth);

        if (p_CreateSampler) {
          ImageHelper::create_2d_sampler(*p_Params.context, p_Image);
        }

        p_Image.swapchainImage = false;
      }

      void vk_imageresource_create(
          Backend::ImageResource &p_Image,
          Backend::ImageResourceCreateParams &p_Params)
      {
        imageresource_create_internal(p_Image, p_Params, true);
      }

      void vk_imageresource_cleanup(Backend::ImageResource &p_Image)
      {
        Context &l_Context = p_Image.context->vk;
        ImageResource &l_Image = p_Image.vk;

        vkDestroySampler(l_Context.m_Device, l_Image.m_Sampler,
                         nullptr);
        vkDestroyImageView(l_Context.m_Device, l_Image.m_ImageView,
                           nullptr);

        if (!p_Image.swapchainImage) {
          // TL TODO: Switch to vma
          vkDestroyImage(l_Context.m_Device, l_Image.m_Image,
                         nullptr);
          vkFreeMemory(l_Context.m_Device, l_Image.m_Memory, nullptr);
        }
      }

      void vk_pipeline_resource_signature_create(
          Backend::PipelineResourceSignature &p_Signature,
          Backend::PipelineResourceSignatureCreateParams &p_Params)
      {
        LOW_ASSERT(p_Params.binding <
                       LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS,
                   "Signature binding index too high");
        p_Signature.binding = p_Params.binding;
        p_Signature.vk.m_Index =
            p_Params.context->vk.m_PipelineResourceSignatureIndex++;
        p_Signature.context = p_Params.context;
        p_Signature.resourceCount = p_Params.resourceDescriptionCount;
        p_Signature.resources = nullptr;
        if (p_Signature.resourceCount) {
          p_Signature.resources =
              (Backend::PipelineResourceBinding *)
                  Util::Memory::main_allocator()
                      ->allocate(
                          p_Signature.resourceCount *
                          sizeof(Backend::PipelineResourceBinding));
        }

        {
          Util::List<VkDescriptorSetLayoutBinding> l_Bindings;

          for (uint32_t i = 0u; i < p_Params.resourceDescriptionCount;
               ++i) {
            Backend::PipelineResourceDescription &i_Resource =
                p_Params.resourceDescriptions[i];

            p_Signature.resources[i].description = i_Resource;
            p_Signature.resources[i].boundResourceHandleId =
                (uint64_t *)Util::Memory::main_allocator()->allocate(
                    i_Resource.arraySize * sizeof(uint64_t));

            VkDescriptorSetLayoutBinding i_Binding;
            i_Binding.binding = i;

            i_Binding.descriptorCount = i_Resource.arraySize;

            if (i_Resource.step ==
                Backend::ResourcePipelineStep::COMPUTE) {
              i_Binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            } else if (i_Resource.step ==
                       Backend::ResourcePipelineStep::VERTEX) {
              i_Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            } else if (i_Resource.step ==
                       Backend::ResourcePipelineStep::FRAGMENT) {
              i_Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            } else if (i_Resource.step ==
                       Backend::ResourcePipelineStep::GRAPHICS) {
              i_Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                     VK_SHADER_STAGE_FRAGMENT_BIT;
            } else if (i_Resource.step ==
                       Backend::ResourcePipelineStep::ALL) {
              i_Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                     VK_SHADER_STAGE_FRAGMENT_BIT |
                                     VK_SHADER_STAGE_COMPUTE_BIT;
            } else {
              LOW_ASSERT(false, "Unknown pipeline step");
            }

            if (i_Resource.type ==
                Backend::ResourceType::CONSTANT_BUFFER) {
              i_Binding.descriptorType =
                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            } else if (i_Resource.type ==
                       Backend::ResourceType::BUFFER) {
              i_Binding.descriptorType =
                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            } else if (i_Resource.type ==
                       Backend::ResourceType::IMAGE) {
              i_Binding.descriptorType =
                  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            } else if (i_Resource.type ==
                       Backend::ResourceType::SAMPLER) {
              i_Binding.descriptorType =
                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            } else if (i_Resource.type ==
                       Backend::ResourceType::UNBOUND_SAMPLER) {
              i_Binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            } else if (i_Resource.type ==
                       Backend::ResourceType::TEXTURE2D) {
              i_Binding.descriptorType =
                  VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            } else if (i_Resource.type ==
                       Backend::ResourceType::IMAGE) {
              i_Binding.descriptorType =
                  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            } else {
              LOW_ASSERT(false, "Unknown uniform interface type");
            }

            i_Binding.pImmutableSamplers = nullptr;

            l_Bindings.push_back(i_Binding);
          }

          VkDescriptorSetLayoutCreateInfo l_LayoutInfo{};
          l_LayoutInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          l_LayoutInfo.bindingCount =
              static_cast<uint32_t>(l_Bindings.size());
          l_LayoutInfo.pBindings = l_Bindings.data();

          LOW_ASSERT(
              vkCreateDescriptorSetLayout(
                  p_Params.context->vk.m_Device, &l_LayoutInfo,
                  nullptr,
                  &(p_Signature.context->vk
                        .m_PipelineResourceSignatures[p_Signature.vk
                                                          .m_Index]
                        .m_DescriptorSetLayout)) == VK_SUCCESS,
              "Failed to create descriptor set layout");
        }

        {
          PipelineResourceSignatureInternal &l_InternalSignature =
              p_Signature.context->vk.m_PipelineResourceSignatures
                  [p_Signature.vk.m_Index];

          l_InternalSignature.m_BindingCount =
              p_Params.resourceDescriptionCount;
          if (l_InternalSignature.m_BindingCount) {
            l_InternalSignature.m_Bindings =
                (Backend::PipelineResourceBinding *)
                    Util::Memory::main_allocator()
                        ->allocate(
                            sizeof(Backend::PipelineResourceBinding) *
                            p_Params.resourceDescriptionCount);
          }

          for (uint32_t i = 0u; i < p_Params.resourceDescriptionCount;
               ++i) {
            l_InternalSignature.m_Bindings[i].boundResourceHandleId =
                (uint64_t *)Util::Memory::main_allocator()->allocate(
                    sizeof(uint64_t) *
                    p_Params.resourceDescriptions[i].arraySize);
            l_InternalSignature.m_Bindings[i].description =
                p_Params.resourceDescriptions[i];
          }

          l_InternalSignature.m_DescriptorSets =
              (VkDescriptorSet *)Util::Memory::main_allocator()
                  ->allocate(sizeof(VkDescriptorSet) *
                             p_Signature.context->framesInFlight);

          Util::List<VkDescriptorSetLayout> l_Layouts(
              p_Params.context->framesInFlight,
              l_InternalSignature.m_DescriptorSetLayout);

          VkDescriptorSetAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          l_AllocInfo.descriptorPool =
              p_Params.context->vk.m_DescriptorPool;
          l_AllocInfo.descriptorSetCount =
              static_cast<uint32_t>(p_Params.context->framesInFlight);
          l_AllocInfo.pSetLayouts = l_Layouts.data();

          LOW_ASSERT(vkAllocateDescriptorSets(
                         p_Params.context->vk.m_Device, &l_AllocInfo,
                         l_InternalSignature.m_DescriptorSets) ==
                         VK_SUCCESS,
                     "Failed to allocate descriptor sets");
        }
      }

      void vk_pipeline_resource_signature_commit(
          Backend::PipelineResourceSignature &p_Signature)
      {
        p_Signature.context->vk.m_CommittedPipelineResourceSignatures
            [p_Signature.binding] = p_Signature.vk.m_Index;
        for (uint32_t i = p_Signature.binding + 1;
             i < LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS; ++i) {
          p_Signature.context->vk
              .m_CommittedPipelineResourceSignatures[i] = ~0u;
        }
      }

      void vk_pipeline_resource_signature_commit_clear(
          Backend::Context &p_Context)
      {
        for (uint32_t i = 0;
             i < LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS; ++i) {
          p_Context.vk.m_CommittedPipelineResourceSignatures[i] = ~0u;
        }
      }

      uint32_t vk_pipeline_resource_signature_get_binding(
          Backend::PipelineResourceSignature &p_Signature,
          Util::Name p_Name, uint8_t p_PreferredType)
      {
        uint32_t l_Index = ~0u;

        for (uint32_t i = 0u; i < p_Signature.resourceCount; ++i) {
          if (p_Signature.resources[i].description.name == p_Name) {
            if (p_Signature.resources[i].description.type ==
                p_PreferredType) {
              return i;
            }
            l_Index = i;
          }
        }
        LOW_ASSERT(l_Index != ~0u, "Unable to find resource binding");
        return l_Index;
      }

      void vk_pipeline_resource_signature_set_buffer(
          Backend::PipelineResourceSignature &p_Signature,
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Buffer p_BufferResource)
      {
        uint32_t l_ResourceIndex =
            vk_pipeline_resource_signature_get_binding(
                p_Signature, p_Name, Backend::ResourceType::BUFFER);
        Backend::PipelineResourceBinding &l_Resource =
            p_Signature.resources[l_ResourceIndex];

        LOW_ASSERT(l_Resource.description.type ==
                       Backend::ResourceType::BUFFER,
                   "Expected buffer resource type");
        LOW_ASSERT(p_ArrayIndex < l_Resource.description.arraySize,
                   "Resource array index out of bounds");

        l_Resource.boundResourceHandleId[p_ArrayIndex] =
            p_BufferResource.get_id();
        p_Signature.context->vk
            .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
            .m_Bindings[l_ResourceIndex]
            .boundResourceHandleId[p_ArrayIndex] =
            p_BufferResource.get_id();

        for (uint8_t j = 0u; j < p_Signature.context->framesInFlight;
             ++j) {
          VkDescriptorBufferInfo i_BufferInfo;
          i_BufferInfo.buffer =
              p_BufferResource.get_buffer().vk.m_Buffer;
          i_BufferInfo.offset = 0;
          i_BufferInfo.range =
              p_BufferResource.get_buffer().bufferSize;

          VkWriteDescriptorSet i_DescriptorWrite;

          i_DescriptorWrite.sType =
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          i_DescriptorWrite.dstSet =
              p_Signature.context->vk
                  .m_PipelineResourceSignatures[p_Signature.vk
                                                    .m_Index]
                  .m_DescriptorSets[j];
          i_DescriptorWrite.dstBinding = l_ResourceIndex;
          i_DescriptorWrite.dstArrayElement = p_ArrayIndex;
          i_DescriptorWrite.pNext = nullptr;
          i_DescriptorWrite.descriptorType =
              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          i_DescriptorWrite.descriptorCount = 1;
          i_DescriptorWrite.pBufferInfo = &i_BufferInfo;
          i_DescriptorWrite.pImageInfo = nullptr;
          i_DescriptorWrite.pTexelBufferView = nullptr;

          vkUpdateDescriptorSets(p_Signature.context->vk.m_Device, 1,
                                 &i_DescriptorWrite, 0, nullptr);
        }
      }

      void vk_pipeline_resource_signature_set_constant_buffer(
          Backend::PipelineResourceSignature &p_Signature,
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Buffer p_BufferResource)
      {
        uint32_t l_ResourceIndex =
            vk_pipeline_resource_signature_get_binding(
                p_Signature, p_Name,
                Backend::ResourceType::CONSTANT_BUFFER);
        Backend::PipelineResourceBinding &l_Resource =
            p_Signature.resources[l_ResourceIndex];

        LOW_ASSERT(l_Resource.description.type ==
                       Backend::ResourceType::CONSTANT_BUFFER,
                   "Expected constant buffer resource type");
        LOW_ASSERT(p_ArrayIndex < l_Resource.description.arraySize,
                   "Resource array index out of bounds");

        l_Resource.boundResourceHandleId[p_ArrayIndex] =
            p_BufferResource.get_id();
        p_Signature.context->vk
            .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
            .m_Bindings[l_ResourceIndex]
            .boundResourceHandleId[p_ArrayIndex] =
            p_BufferResource.get_id();

        for (uint8_t j = 0u; j < p_Signature.context->framesInFlight;
             ++j) {
          VkDescriptorBufferInfo i_BufferInfo;
          i_BufferInfo.buffer =
              p_BufferResource.get_buffer().vk.m_Buffer;
          i_BufferInfo.offset = 0;
          i_BufferInfo.range =
              p_BufferResource.get_buffer().bufferSize;

          VkWriteDescriptorSet i_DescriptorWrite;

          i_DescriptorWrite.sType =
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          i_DescriptorWrite.dstSet =
              p_Signature.context->vk
                  .m_PipelineResourceSignatures[p_Signature.vk
                                                    .m_Index]
                  .m_DescriptorSets[j];
          i_DescriptorWrite.dstBinding = l_ResourceIndex;
          i_DescriptorWrite.dstArrayElement = p_ArrayIndex;
          i_DescriptorWrite.pNext = nullptr;
          i_DescriptorWrite.descriptorType =
              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
          i_DescriptorWrite.descriptorCount = 1;
          i_DescriptorWrite.pBufferInfo = &i_BufferInfo;
          i_DescriptorWrite.pImageInfo = nullptr;
          i_DescriptorWrite.pTexelBufferView = nullptr;

          vkUpdateDescriptorSets(p_Signature.context->vk.m_Device, 1,
                                 &i_DescriptorWrite, 0, nullptr);
        }
      }

      void vk_pipeline_resource_signature_set_sampler(
          Backend::PipelineResourceSignature &p_Signature,
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Image p_ImageResource)
      {
        uint32_t l_ResourceIndex =
            vk_pipeline_resource_signature_get_binding(
                p_Signature, p_Name, Backend::ResourceType::SAMPLER);
        Backend::PipelineResourceBinding &l_Resource =
            p_Signature.resources[l_ResourceIndex];

        LOW_ASSERT(l_Resource.description.type ==
                       Backend::ResourceType::SAMPLER,
                   "Expected image resource type");
        LOW_ASSERT(p_ArrayIndex < l_Resource.description.arraySize,
                   "Resource array index out of bounds");

        l_Resource.boundResourceHandleId[p_ArrayIndex] =
            p_ImageResource.get_id();
        p_Signature.context->vk
            .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
            .m_Bindings[l_ResourceIndex]
            .boundResourceHandleId[p_ArrayIndex] =
            p_ImageResource.get_id();

        for (uint8_t j = 0u; j < p_Signature.context->framesInFlight;
             ++j) {
          VkDescriptorImageInfo i_ImageInfo;
          i_ImageInfo.imageLayout =
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          i_ImageInfo.imageView =
              p_ImageResource.get_image().vk.m_ImageView;
          i_ImageInfo.sampler =
              p_ImageResource.get_image().vk.m_Sampler;

          VkWriteDescriptorSet i_DescriptorWrite;

          i_DescriptorWrite.sType =
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          i_DescriptorWrite.dstSet =
              p_Signature.context->vk
                  .m_PipelineResourceSignatures[p_Signature.vk
                                                    .m_Index]
                  .m_DescriptorSets[j];
          i_DescriptorWrite.dstBinding = l_ResourceIndex;
          i_DescriptorWrite.dstArrayElement = p_ArrayIndex;
          i_DescriptorWrite.pNext = nullptr;
          i_DescriptorWrite.descriptorType =
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          i_DescriptorWrite.descriptorCount = 1;
          i_DescriptorWrite.pBufferInfo = nullptr;
          i_DescriptorWrite.pImageInfo = &(i_ImageInfo);
          i_DescriptorWrite.pTexelBufferView = nullptr;

          vkUpdateDescriptorSets(p_Signature.context->vk.m_Device, 1,
                                 &i_DescriptorWrite, 0, nullptr);
        }
      }

      void vk_pipeline_resource_signature_set_unbound_sampler(
          Backend::PipelineResourceSignature &p_Signature,
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Image p_ImageResource)
      {
        uint32_t l_ResourceIndex =
            vk_pipeline_resource_signature_get_binding(
                p_Signature, p_Name,
                Backend::ResourceType::UNBOUND_SAMPLER);
        Backend::PipelineResourceBinding &l_Resource =
            p_Signature.resources[l_ResourceIndex];

        LOW_ASSERT(l_Resource.description.type ==
                       Backend::ResourceType::UNBOUND_SAMPLER,
                   "Expected unbound_sampler resource type");
        LOW_ASSERT(p_ArrayIndex < l_Resource.description.arraySize,
                   "Resource array index out of bounds");

        l_Resource.boundResourceHandleId[p_ArrayIndex] =
            p_ImageResource.get_id();
        p_Signature.context->vk
            .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
            .m_Bindings[l_ResourceIndex]
            .boundResourceHandleId[p_ArrayIndex] =
            p_ImageResource.get_id();

        for (uint8_t j = 0u; j < p_Signature.context->framesInFlight;
             ++j) {
          VkDescriptorImageInfo i_ImageInfo;
          i_ImageInfo.imageLayout =
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          i_ImageInfo.imageView =
              p_ImageResource.get_image().vk.m_ImageView;
          i_ImageInfo.sampler =
              p_ImageResource.get_image().vk.m_Sampler;

          VkWriteDescriptorSet i_DescriptorWrite;

          i_DescriptorWrite.sType =
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          i_DescriptorWrite.dstSet =
              p_Signature.context->vk
                  .m_PipelineResourceSignatures[p_Signature.vk
                                                    .m_Index]
                  .m_DescriptorSets[j];
          i_DescriptorWrite.dstBinding = l_ResourceIndex;
          i_DescriptorWrite.dstArrayElement = p_ArrayIndex;
          i_DescriptorWrite.pNext = nullptr;
          i_DescriptorWrite.descriptorType =
              VK_DESCRIPTOR_TYPE_SAMPLER;
          i_DescriptorWrite.descriptorCount = 1;
          i_DescriptorWrite.pBufferInfo = nullptr;
          i_DescriptorWrite.pImageInfo = &(i_ImageInfo);
          i_DescriptorWrite.pTexelBufferView = nullptr;

          vkUpdateDescriptorSets(p_Signature.context->vk.m_Device, 1,
                                 &i_DescriptorWrite, 0, nullptr);
        }
      }

      void vk_pipeline_resource_signature_set_texture2d(
          Backend::PipelineResourceSignature &p_Signature,
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Image p_ImageResource)
      {
        uint32_t l_ResourceIndex =
            vk_pipeline_resource_signature_get_binding(
                p_Signature, p_Name,
                Backend::ResourceType::TEXTURE2D);
        Backend::PipelineResourceBinding &l_Resource =
            p_Signature.resources[l_ResourceIndex];

        LOW_ASSERT(l_Resource.description.type ==
                       Backend::ResourceType::TEXTURE2D,
                   "Expected texture2d resource type");
        LOW_ASSERT(p_ArrayIndex < l_Resource.description.arraySize,
                   "Resource array index out of bounds");

        l_Resource.boundResourceHandleId[p_ArrayIndex] =
            p_ImageResource.get_id();
        p_Signature.context->vk
            .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
            .m_Bindings[l_ResourceIndex]
            .boundResourceHandleId[p_ArrayIndex] =
            p_ImageResource.get_id();

        for (uint8_t j = 0u; j < p_Signature.context->framesInFlight;
             ++j) {
          VkDescriptorImageInfo i_ImageInfo;
          i_ImageInfo.imageLayout =
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          i_ImageInfo.imageView =
              p_ImageResource.get_image().vk.m_ImageView;
          i_ImageInfo.sampler =
              p_ImageResource.get_image().vk.m_Sampler;

          VkWriteDescriptorSet i_DescriptorWrite;

          i_DescriptorWrite.sType =
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          i_DescriptorWrite.dstSet =
              p_Signature.context->vk
                  .m_PipelineResourceSignatures[p_Signature.vk
                                                    .m_Index]
                  .m_DescriptorSets[j];
          i_DescriptorWrite.dstBinding = l_ResourceIndex;
          i_DescriptorWrite.dstArrayElement = p_ArrayIndex;
          i_DescriptorWrite.pNext = nullptr;
          i_DescriptorWrite.descriptorType =
              VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
          i_DescriptorWrite.descriptorCount = 1;
          i_DescriptorWrite.pBufferInfo = nullptr;
          i_DescriptorWrite.pImageInfo = &(i_ImageInfo);
          i_DescriptorWrite.pTexelBufferView = nullptr;

          vkUpdateDescriptorSets(p_Signature.context->vk.m_Device, 1,
                                 &i_DescriptorWrite, 0, nullptr);
        }
      }

      void vk_pipeline_resource_signature_set_image(
          Backend::PipelineResourceSignature &p_Signature,
          Util::Name p_Name, uint32_t p_ArrayIndex,
          Resource::Image p_ImageResource)
      {
        uint32_t l_ResourceIndex =
            vk_pipeline_resource_signature_get_binding(
                p_Signature, p_Name, Backend::ResourceType::IMAGE);
        Backend::PipelineResourceBinding &l_Resource =
            p_Signature.resources[l_ResourceIndex];

        LOW_ASSERT(l_Resource.description.type ==
                       Backend::ResourceType::IMAGE,
                   "Expected image resource type");
        LOW_ASSERT(p_ArrayIndex < l_Resource.description.arraySize,
                   "Resource array index out of bounds");

        l_Resource.boundResourceHandleId[p_ArrayIndex] =
            p_ImageResource.get_id();
        p_Signature.context->vk
            .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
            .m_Bindings[l_ResourceIndex]
            .boundResourceHandleId[p_ArrayIndex] =
            p_ImageResource.get_id();

        for (uint8_t j = 0u; j < p_Signature.context->framesInFlight;
             ++j) {
          VkDescriptorImageInfo i_ImageInfo;
          i_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
          i_ImageInfo.imageView =
              p_ImageResource.get_image().vk.m_ImageView;
          i_ImageInfo.sampler =
              p_ImageResource.get_image().vk.m_Sampler;

          VkWriteDescriptorSet i_DescriptorWrite;

          i_DescriptorWrite.sType =
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          i_DescriptorWrite.dstSet =
              p_Signature.context->vk
                  .m_PipelineResourceSignatures[p_Signature.vk
                                                    .m_Index]
                  .m_DescriptorSets[j];
          i_DescriptorWrite.dstBinding = l_ResourceIndex;
          i_DescriptorWrite.dstArrayElement = p_ArrayIndex;
          i_DescriptorWrite.pNext = nullptr;
          i_DescriptorWrite.descriptorType =
              VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
          i_DescriptorWrite.descriptorCount = 1;
          i_DescriptorWrite.pBufferInfo = nullptr;
          i_DescriptorWrite.pImageInfo = &(i_ImageInfo);
          i_DescriptorWrite.pTexelBufferView = nullptr;

          vkUpdateDescriptorSets(p_Signature.context->vk.m_Device, 1,
                                 &i_DescriptorWrite, 0, nullptr);
        }
      }

      namespace PipelineHelper {
        VkShaderModule
        create_shader_module(Backend::Context &p_Context,
                             const Util::List<char> &p_Code)
        {
          VkShaderModuleCreateInfo l_CreateInfo{};
          l_CreateInfo.sType =
              VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
          l_CreateInfo.codeSize =
              p_Code.size() - 1; // Remove \0 terminator
          l_CreateInfo.pCode =
              reinterpret_cast<const uint32_t *>(p_Code.data());

          VkShaderModule l_ShaderModule;
          LOW_ASSERT(vkCreateShaderModule(
                         p_Context.vk.m_Device, &l_CreateInfo,
                         nullptr, &l_ShaderModule) == VK_SUCCESS,
                     "Failed to create shader module");

          return l_ShaderModule;
        }

        Util::List<char> read_shader_file(const char *p_Filename)
        {
          Util::FileIO::File l_File = Util::FileIO::open(
              p_Filename, Util::FileIO::FileMode::READ_BYTES);

          size_t l_Filesize = Util::FileIO::size_sync(l_File);
          Util::List<char> l_ContentBuffer(
              l_Filesize + 1); // Add 1 because of \0 terminator

          Util::FileIO::read_sync(l_File, l_ContentBuffer.data());

          return l_ContentBuffer;
        }

        void pipeline_layout_create(
            Backend::Context &p_Context,
            Backend::Pipeline &p_Pipeline,
            Backend::PipelineResourceSignature *p_Signatures,
            uint8_t p_SignatureCount,
            Backend::PipelineConstantCreateParams
                *p_PipelineConstants,
            uint8_t p_PipelineConstantCount)
        {

          Pipeline &l_Pipeline = p_Pipeline.vk;

          {
            Util::List<VkDescriptorSetLayout> l_DescriptorSetLayouts;
            l_DescriptorSetLayouts.resize(p_SignatureCount);
            for (uint32_t i = 0u; i < p_SignatureCount; ++i) {
              l_DescriptorSetLayouts[i] =
                  p_Context.vk
                      .m_PipelineResourceSignatures[p_Signatures[i]
                                                        .vk.m_Index]
                      .m_DescriptorSetLayout;
            }

            VkPipelineLayoutCreateInfo l_PipelineLayoutInfo{};
            l_PipelineLayoutInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            if (p_SignatureCount == 0) {
              l_PipelineLayoutInfo.setLayoutCount = 0;
              l_PipelineLayoutInfo.pSetLayouts = nullptr;
            } else {
              l_PipelineLayoutInfo.setLayoutCount = p_SignatureCount;
              l_PipelineLayoutInfo.pSetLayouts =
                  l_DescriptorSetLayouts.data();
            }
            l_PipelineLayoutInfo.pushConstantRangeCount = 0;
            l_PipelineLayoutInfo.pPushConstantRanges = nullptr;

            l_Pipeline.m_PipelineConstants = nullptr;
            l_Pipeline.m_PipelineConstantCount = 0;

            Util::List<VkPushConstantRange> l_PushConstantRanges;

            if (p_PipelineConstantCount > 0) {
              l_PushConstantRanges.resize(p_PipelineConstantCount);

              l_Pipeline.m_PipelineConstantCount =
                  p_PipelineConstantCount;

              l_Pipeline.m_PipelineConstants =
                  (PipelineConstant *)Util::Memory::main_allocator()
                      ->allocate(p_PipelineConstantCount *
                                 sizeof(PipelineConstant));

              uint32_t l_Offset = 0;
              for (uint32_t i = 0; i < p_PipelineConstantCount; ++i) {
                l_Pipeline.m_PipelineConstants[i].name =
                    p_PipelineConstants[i].name;
                l_Pipeline.m_PipelineConstants[i].size =
                    p_PipelineConstants[i].size;
                l_Pipeline.m_PipelineConstants[i].offset = l_Offset;

                l_PushConstantRanges[i].size =
                    p_PipelineConstants[i].size;
                l_PushConstantRanges[i].offset = l_Offset;

                if (p_Pipeline.type ==
                    Backend::PipelineType::COMPUTE) {
                  l_PushConstantRanges[i].stageFlags =
                      VK_SHADER_STAGE_COMPUTE_BIT;
                } else if (p_Pipeline.type ==
                           Backend::PipelineType::GRAPHICS) {
                  LOW_ASSERT(
                      false,
                      "Pipeline constants are not supported for "
                      "graphics pipelines");
                } else {
                  LOW_ASSERT(false, "Unknown pipeline type");
                }

                l_Offset += p_PipelineConstants[i].size;
              }

              l_PipelineLayoutInfo.pPushConstantRanges =
                  l_PushConstantRanges.data();
              l_PipelineLayoutInfo.pushConstantRangeCount =
                  l_PushConstantRanges.size();
            }

            LOW_ASSERT(vkCreatePipelineLayout(
                           p_Context.vk.m_Device,
                           &l_PipelineLayoutInfo, nullptr,
                           &l_Pipeline.m_PipelineLayout) ==
                           VK_SUCCESS,
                       "Failed to create pipeline layout");
          }
        }
      } // namespace PipelineHelper

      void vk_pipeline_compute_create(
          Backend::Pipeline &p_Pipeline,
          Backend::PipelineComputeCreateParams &p_Params)
      {
        p_Pipeline.context = p_Params.context;

        p_Pipeline.type = Backend::PipelineType::COMPUTE;

        PipelineHelper::pipeline_layout_create(
            *p_Pipeline.context, p_Pipeline, p_Params.signatures,
            p_Params.signatureCount, p_Params.pipelineConstants,
            p_Params.pipelineConstantCount);

        {
          VkShaderModule l_ShaderModule =
              PipelineHelper::create_shader_module(
                  *p_Params.context, PipelineHelper::read_shader_file(
                                         p_Params.shaderPath));

          VkComputePipelineCreateInfo l_ComputePipelineInfo = {
              VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
          l_ComputePipelineInfo.stage.sType =
              VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
          l_ComputePipelineInfo.stage.stage =
              VK_SHADER_STAGE_COMPUTE_BIT;
          l_ComputePipelineInfo.stage.module = l_ShaderModule;
          l_ComputePipelineInfo.stage.pName = "main";
          l_ComputePipelineInfo.layout =
              p_Pipeline.vk.m_PipelineLayout;

          LOW_ASSERT(vkCreateComputePipelines(
                         p_Params.context->vk.m_Device,
                         VK_NULL_HANDLE, 1, &l_ComputePipelineInfo,
                         nullptr,
                         &(p_Pipeline.vk.m_Pipeline)) == VK_SUCCESS,
                     "Failed to create compute pipeline");

          vkDestroyShaderModule(p_Params.context->vk.m_Device,
                                l_ShaderModule, nullptr);
        }
      }

      void vk_pipeline_graphics_create(
          Backend::Pipeline &p_Pipeline,
          Backend::PipelineGraphicsCreateParams &p_Params)
      {
        p_Pipeline.context = p_Params.context;
        p_Pipeline.type = Backend::PipelineType::GRAPHICS;

        PipelineHelper::pipeline_layout_create(
            *p_Pipeline.context, p_Pipeline, p_Params.signatures,
            p_Params.signatureCount, nullptr, 0);

        auto l_VertexShaderCode = PipelineHelper::read_shader_file(
            p_Params.vertexShaderPath);
        auto l_FragmentShaderCode = PipelineHelper::read_shader_file(
            p_Params.fragmentShaderPath);

        VkShaderModule l_VertexShaderModule =
            PipelineHelper::create_shader_module(*p_Params.context,
                                                 l_VertexShaderCode);
        VkShaderModule l_FragmentShaderModule =
            PipelineHelper::create_shader_module(
                *p_Params.context, l_FragmentShaderCode);

        VkPipelineShaderStageCreateInfo l_VertexShaderStageInfo{};
        l_VertexShaderStageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_VertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        l_VertexShaderStageInfo.module = l_VertexShaderModule;
        l_VertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo l_FragmentShaderStageInfo{};
        l_FragmentShaderStageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_FragmentShaderStageInfo.stage =
            VK_SHADER_STAGE_FRAGMENT_BIT;
        l_FragmentShaderStageInfo.module = l_FragmentShaderModule;
        l_FragmentShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo l_ShaderStages[] = {
            l_VertexShaderStageInfo, l_FragmentShaderStageInfo};

        Low::Util::List<VkDynamicState> l_DynamicStages = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo l_DynamicState{};
        l_DynamicState.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_DynamicState.dynamicStateCount =
            static_cast<uint32_t>(l_DynamicStages.size());
        l_DynamicState.pDynamicStates = l_DynamicStages.data();

        VkPipelineVertexInputStateCreateInfo l_VertexInputInfo{};

        Low::Util::List<VkVertexInputAttributeDescription>
            l_AttributeDescriptions;
        l_AttributeDescriptions.resize(
            p_Params.vertexDataAttributeCount);

        VkVertexInputBindingDescription l_BindingDescription;
        l_BindingDescription.binding = 0;
        l_BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        uint32_t l_Stride = 0u;

        for (uint8_t i = 0u; i < p_Params.vertexDataAttributeCount;
             ++i) {
          l_AttributeDescriptions[i].binding = 0;
          l_AttributeDescriptions[i].location = i;
          l_AttributeDescriptions[i].offset = l_Stride;

          if (p_Params.vertexDataAttributesType[i] ==
              Backend::VertexAttributeType::VECTOR2) {
            l_AttributeDescriptions[i].format =
                VK_FORMAT_R32G32_SFLOAT;
            l_Stride += sizeof(Math::Vector2);
          } else if (p_Params.vertexDataAttributesType[i] ==
                     Backend::VertexAttributeType::VECTOR3) {
            l_AttributeDescriptions[i].format =
                VK_FORMAT_R32G32B32_SFLOAT;
            l_Stride += sizeof(Math::Vector3);
          } else {
            LOW_ASSERT(false, "Unknown vertex attribute type");
          }
        }

        l_BindingDescription.stride = l_Stride;

        l_VertexInputInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        if (p_Params.vertexDataAttributeCount > 0) {
          l_VertexInputInfo.vertexBindingDescriptionCount = 1;
          l_VertexInputInfo.pVertexBindingDescriptions =
              &l_BindingDescription;
        } else {
          l_VertexInputInfo.vertexBindingDescriptionCount = 0;
          l_VertexInputInfo.pVertexBindingDescriptions = nullptr;
        }
        l_VertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(l_AttributeDescriptions.size());
        l_VertexInputInfo.pVertexAttributeDescriptions =
            l_AttributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
        l_InputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_InputAssembly.topology =
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        l_InputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport l_Viewport{};
        l_Viewport.x = 0.f;
        l_Viewport.y = 0.f;
        l_Viewport.width = (float)p_Params.dimensions.x;
        l_Viewport.height = (float)p_Params.dimensions.y;
        l_Viewport.minDepth = 0.f;
        l_Viewport.maxDepth = 1.f;

        VkExtent2D l_Extent{p_Params.dimensions.x,
                            p_Params.dimensions.y};

        VkRect2D l_Scissor{};
        l_Scissor.offset = {0, 0};
        l_Scissor.extent = l_Extent;

        VkPipelineViewportStateCreateInfo l_ViewportState{};
        l_ViewportState.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        l_ViewportState.viewportCount = 1;
        l_ViewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo l_Rasterizer{};
        l_Rasterizer.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        l_Rasterizer.depthClampEnable = VK_FALSE;
        l_Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        if (p_Params.polygonMode ==
            Backend::PipelineRasterizerPolygonMode::FILL) {
          l_Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        } else if (p_Params.polygonMode ==
                   Backend::PipelineRasterizerPolygonMode::LINE) {
          l_Rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        } else {
          LOW_ASSERT(false, "Unknown rasterizer polygon mode");
        }

        // Fills the polygons "LINE" would be
        // wireframe
        // Setting this to any other mode but FILL requires
        // ~l_Rasterizer.lineWidth~ so be set to some float. If the
        // float is larger than 1 then a special GPU feature
        // ~wideLines~ has to be enabled
        l_Rasterizer.lineWidth = 2.f;
        if (p_Params.cullMode ==
            Backend::PipelineRasterizerCullMode::FRONT) {
          l_Rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
        } else if (p_Params.cullMode ==
                   Backend::PipelineRasterizerCullMode::BACK) {
          l_Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        } else if (p_Params.cullMode ==
                   Backend::PipelineRasterizerCullMode::NONE) {
          l_Rasterizer.cullMode = VK_CULL_MODE_NONE;
        } else {
          LOW_ASSERT(false, "Unknown pipeline rasterizer cull mode");
        }

        if (p_Params.frontFace ==
            Backend::PipelineRasterizerFrontFace::CLOCKWISE) {
          l_Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        } else if (p_Params.frontFace ==
                   Backend::PipelineRasterizerFrontFace::
                       COUNTER_CLOCKWISE) {
          l_Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        } else {
          LOW_ASSERT(false, "Unknown rasterizer frontface mode");
        }

        l_Rasterizer.depthBiasEnable = VK_FALSE;
        l_Rasterizer.depthBiasConstantFactor = 0.f;
        l_Rasterizer.depthBiasClamp = 0.f;

        VkPipelineMultisampleStateCreateInfo l_Multisampling{};
        l_Multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_Multisampling.sampleShadingEnable = VK_FALSE;
        l_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        l_Multisampling.minSampleShading = 1.f;
        l_Multisampling.pSampleMask = nullptr;
        l_Multisampling.alphaToCoverageEnable = VK_FALSE;
        l_Multisampling.alphaToOneEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
        l_DepthStencil.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        l_DepthStencil.depthTestEnable =
            p_Params.depthTest ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthWriteEnable =
            p_Params.depthWrite ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        if (p_Params.depthTest || p_Params.depthWrite) {
          switch (p_Params.depthCompareOperation) {
          case Backend::CompareOperation::LESS:
            l_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            break;
          case Backend::CompareOperation::EQUAL:
            l_DepthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;
            break;
          default:
            LOW_ASSERT(false, "Unknown compare operation");
            break;
          }
        }
        l_DepthStencil.depthBoundsTestEnable = VK_FALSE;
        l_DepthStencil.minDepthBounds = 0.0f;
        l_DepthStencil.maxDepthBounds = 1.0f;
        l_DepthStencil.stencilTestEnable = VK_FALSE;
        l_DepthStencil.front = {};
        l_DepthStencil.back = {};

        Low::Util::List<VkPipelineColorBlendAttachmentState>
            l_ColorBlendAttachments;

        for (uint32_t i = 0u; i < p_Params.colorTargetCount; ++i) {
          Backend::GraphicsPipelineColorTarget &i_Target =
              p_Params.colorTargets[i];

          int test =
              i_Target.wirteMask & LOW_RENDERER_COLOR_WRITE_BIT_RED;

          VkColorComponentFlags l_WriteMask = 0;
          if ((i_Target.wirteMask &
               LOW_RENDERER_COLOR_WRITE_BIT_RED) ==
              LOW_RENDERER_COLOR_WRITE_BIT_RED) {
            l_WriteMask |= VK_COLOR_COMPONENT_R_BIT;
          }
          if ((i_Target.wirteMask &
               LOW_RENDERER_COLOR_WRITE_BIT_GREEN) ==
              LOW_RENDERER_COLOR_WRITE_BIT_GREEN) {
            l_WriteMask |= VK_COLOR_COMPONENT_G_BIT;
          }
          if ((i_Target.wirteMask &
               LOW_RENDERER_COLOR_WRITE_BIT_BLUE) ==
              LOW_RENDERER_COLOR_WRITE_BIT_BLUE) {
            l_WriteMask |= VK_COLOR_COMPONENT_B_BIT;
          }
          if ((i_Target.wirteMask &
               LOW_RENDERER_COLOR_WRITE_BIT_ALPHA) ==
              LOW_RENDERER_COLOR_WRITE_BIT_ALPHA) {
            // l_WriteMask |= VK_COLOR_COMPONENT_A_BIT;
            //  TODO: This is currently disabled because transparency
            //  lead to problems. This should be fixed on a higher
            //  level I guess.
          }

          VkPipelineColorBlendAttachmentState
              l_ColorBlendAttachment{};
          l_ColorBlendAttachment.colorWriteMask = l_WriteMask;

          if (i_Target.blendEnable) {
            l_ColorBlendAttachment.blendEnable = VK_TRUE;
          } else {
            l_ColorBlendAttachment.blendEnable = VK_FALSE;
          }

          l_ColorBlendAttachment.srcColorBlendFactor =
              VK_BLEND_FACTOR_SRC_ALPHA;
          l_ColorBlendAttachment.dstColorBlendFactor =
              VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
          l_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
          l_ColorBlendAttachment.srcAlphaBlendFactor =
              VK_BLEND_FACTOR_ONE;
          l_ColorBlendAttachment.dstAlphaBlendFactor =
              VK_BLEND_FACTOR_ZERO;
          l_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

          l_ColorBlendAttachments.push_back(l_ColorBlendAttachment);
        }

        VkPipelineColorBlendStateCreateInfo l_ColorBlending{};
        l_ColorBlending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_ColorBlending.logicOpEnable = VK_FALSE;
        l_ColorBlending.logicOp = VK_LOGIC_OP_COPY;
        l_ColorBlending.attachmentCount =
            static_cast<uint32_t>(l_ColorBlendAttachments.size());
        l_ColorBlending.pAttachments = l_ColorBlendAttachments.data();
        l_ColorBlending.blendConstants[0] = 0.f;
        l_ColorBlending.blendConstants[1] = 0.f;
        l_ColorBlending.blendConstants[2] = 0.f;
        l_ColorBlending.blendConstants[3] = 0.f;

        VkGraphicsPipelineCreateInfo l_PipelineInfo{};
        l_PipelineInfo.sType =
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        l_PipelineInfo.stageCount = 2;
        l_PipelineInfo.pStages = l_ShaderStages;
        l_PipelineInfo.pVertexInputState = &l_VertexInputInfo;
        l_PipelineInfo.pInputAssemblyState = &l_InputAssembly;
        l_PipelineInfo.pViewportState = &l_ViewportState;
        l_PipelineInfo.pRasterizationState = &l_Rasterizer;
        l_PipelineInfo.pMultisampleState = &l_Multisampling;
        l_PipelineInfo.pDepthStencilState = &l_DepthStencil;
        l_PipelineInfo.pColorBlendState = &l_ColorBlending;
        l_PipelineInfo.pDynamicState = &l_DynamicState;
        l_PipelineInfo.layout = p_Pipeline.vk.m_PipelineLayout;
        l_PipelineInfo.renderPass =
            p_Params.renderpass->vk.m_Renderpass;
        l_PipelineInfo.subpass = 0;
        l_PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        l_PipelineInfo.basePipelineIndex = -1;

        LOW_ASSERT(vkCreateGraphicsPipelines(
                       p_Params.context->vk.m_Device, VK_NULL_HANDLE,
                       1, &l_PipelineInfo, nullptr,
                       &(p_Pipeline.vk.m_Pipeline)) == VK_SUCCESS,
                   "Failed to create graphics pipeline");

        vkDestroyShaderModule(p_Params.context->vk.m_Device,
                              l_FragmentShaderModule, nullptr);
        vkDestroyShaderModule(p_Params.context->vk.m_Device,
                              l_VertexShaderModule, nullptr);
      }

      void vk_pipeline_cleanup(Backend::Pipeline &p_Pipeline)
      {
        vkDestroyPipeline(p_Pipeline.context->vk.m_Device,
                          p_Pipeline.vk.m_Pipeline, nullptr);
        vkDestroyPipelineLayout(p_Pipeline.context->vk.m_Device,
                                p_Pipeline.vk.m_PipelineLayout,
                                nullptr);

        if (p_Pipeline.vk.m_PipelineConstantCount > 0) {
          Util::Memory::main_allocator()->deallocate(
              p_Pipeline.vk.m_PipelineConstants);
        }
      }

      void vk_pipeline_bind(Backend::Pipeline &p_Pipeline)
      {
        LOW_PROFILE_CPU("Renderer", "Vulkan::BindPipeline");
        if (p_Pipeline.type == Backend::PipelineType::GRAPHICS) {
          p_Pipeline.context->vk.m_BoundPipelineLayout =
              p_Pipeline.vk.m_PipelineLayout;
          vk_bind_resources(*p_Pipeline.context,
                            VK_PIPELINE_BIND_POINT_GRAPHICS);
          vkCmdBindPipeline(ContextHelper::get_current_commandbuffer(
                                *p_Pipeline.context),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            p_Pipeline.vk.m_Pipeline);
        } else if (p_Pipeline.type ==
                   Backend::PipelineType::COMPUTE) {
          p_Pipeline.context->vk.m_BoundPipelineLayout =
              p_Pipeline.vk.m_PipelineLayout;
          vkCmdBindPipeline(ContextHelper::get_current_commandbuffer(
                                *p_Pipeline.context),
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            p_Pipeline.vk.m_Pipeline);
        } else {
          LOW_ASSERT(false, "Unknown pipeline type");
        }
      }

      void vk_pipeline_set_constant(Backend::Pipeline &p_Pipeline,
                                    Util::Name p_ConstantName,
                                    void *p_Value)
      {
        PipelineConstant l_PipelineConstant;
        bool l_Found = false;

        for (uint32_t i = 0u;
             i < p_Pipeline.vk.m_PipelineConstantCount; ++i) {
          if (p_Pipeline.vk.m_PipelineConstants[i].name ==
              p_ConstantName) {
            l_Found = true;
            l_PipelineConstant = p_Pipeline.vk.m_PipelineConstants[i];
          }
        }

        LOW_ASSERT(l_Found, "Could not find pipeline constant");

        VkShaderStageFlags l_ShaderFlags = 0;

        if (p_Pipeline.type == Backend::PipelineType::COMPUTE) {
          l_ShaderFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        } else if (p_Pipeline.type ==
                   Backend::PipelineType::GRAPHICS) {
          LOW_ASSERT(false, "Pipeline constants are not supported "
                            "for graphics pipelines");
        } else {
          LOW_ASSERT(false, "Unknown pipeline type");
        }

        vkCmdPushConstants(ContextHelper::get_current_commandbuffer(
                               *p_Pipeline.context),
                           p_Pipeline.vk.m_PipelineLayout,
                           l_ShaderFlags, l_PipelineConstant.offset,
                           l_PipelineConstant.size, p_Value);
      }

      static void vk_perform_barriers(Backend::Context &p_Context)
      {
        LOW_PROFILE_CPU("Renderer", "Vulkan::PerformBarriers");

        for (uint32_t i = 0u;
             i < LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS; ++i) {
          uint32_t i_Index =
              p_Context.vk.m_CommittedPipelineResourceSignatures[i];
          if (i_Index > MAX_PRS) {
            break;
          }

          PipelineResourceSignatureInternal &l_InternalSignature =
              p_Context.vk.m_PipelineResourceSignatures[i_Index];

          for (uint32_t j = 0u;
               j < l_InternalSignature.m_BindingCount; ++j) {
            Backend::PipelineResourceBinding &i_Binding =
                l_InternalSignature.m_Bindings[j];
            if (i_Binding.description.type ==
                Backend::ResourceType::IMAGE) {
              for (uint32_t i = 0u;
                   i < i_Binding.description.arraySize; ++i) {
                BarrierHelper::perform_resource_barrier_image(
                    p_Context, i_Binding.boundResourceHandleId[i]);
              }
            } else if (i_Binding.description.type ==
                           Backend::ResourceType::SAMPLER ||
                       i_Binding.description.type ==
                           Backend::ResourceType::UNBOUND_SAMPLER) {
              for (uint32_t i = 0u;
                   i < i_Binding.description.arraySize; ++i) {
                BarrierHelper::perform_resource_barrier_sampler(
                    p_Context, i_Binding.boundResourceHandleId[i]);
              }
            }
          }
        }
      }

      static void
      vk_bind_descriptor_sets(Backend::Context &p_Context,
                              VkPipelineBindPoint p_BindPoint)
      {
        Util::List<VkDescriptorSet> l_DescriptorSets;

        for (uint32_t i = 0u;
             i < LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS; ++i) {
          uint32_t i_Index =
              p_Context.vk.m_CommittedPipelineResourceSignatures[i];
          if (i_Index > MAX_PRS) {
            break;
          }

          PipelineResourceSignatureInternal &l_InternalSignature =
              p_Context.vk.m_PipelineResourceSignatures[i_Index];
          l_DescriptorSets.push_back(
              l_InternalSignature
                  .m_DescriptorSets[p_Context.currentFrameIndex]);
        }

        if (l_DescriptorSets.size() == 0) {
          return;
        }

        vkCmdBindDescriptorSets(
            ContextHelper::get_current_commandbuffer(p_Context),
            p_BindPoint, p_Context.vk.m_BoundPipelineLayout, 0,
            l_DescriptorSets.size(), l_DescriptorSets.data(), 0,
            nullptr);
      }

      void vk_bind_resources(Backend::Context &p_Context,
                             VkPipelineBindPoint p_BindPoint)
      {
        vk_bind_descriptor_sets(p_Context, p_BindPoint);
      }

      void vk_compute_dispatch(Backend::Context &p_Context,
                               Math::UVector3 p_Dimensions)
      {
        vk_perform_barriers(p_Context);
        vk_bind_resources(p_Context, VK_PIPELINE_BIND_POINT_COMPUTE);
        vkCmdDispatch(
            ContextHelper::get_current_commandbuffer(p_Context),
            p_Dimensions.x, p_Dimensions.y, p_Dimensions.z);

        Util::Name l_GlobalName = N(LOW_RENDERER_COMPUTEDISPATCH);
        Util::Globals::set(l_GlobalName,
                           ((int)Util::Globals::get(l_GlobalName)) +
                               1);
      }

      uint32_t vk_get_draw_indexed_indirect_info_size()
      {
        return sizeof(DrawIndexedIndirectInfo);
      }

      void vk_draw_indexed_indirect_info_fill(
          Backend::DrawIndexedIndirectInfo &p_Info,
          uint32_t p_IndexCount, uint32_t p_FirstIndex,
          int32_t p_VertexBufferOffset, uint32_t p_InstanceCount,
          uint32_t p_FirstInstance)
      {
        p_Info.vk.indexCount = p_IndexCount;
        p_Info.vk.firstIndex = p_FirstIndex;
        p_Info.vk.vertexOffset = p_VertexBufferOffset;
        p_Info.vk.instanceCount = p_InstanceCount;
        p_Info.vk.firstInstance = p_FirstInstance;
      }

      void vk_draw(Backend::DrawParams &p_Params)
      {
        vkCmdDraw(ContextHelper::get_current_commandbuffer(
                      *p_Params.context),
                  p_Params.vertexCount, 1, p_Params.firstVertex, 0);

        Util::Name l_GlobalName = N(LOW_RENDERER_DRAWCALLS);
        Util::Globals::set(l_GlobalName,
                           ((int)Util::Globals::get(l_GlobalName)) +
                               1);
      }

      void vk_draw_indexed(Backend::DrawIndexedParams &p_Params)
      {
        LOW_PROFILE_CPU("Renderer", "Vulkan::DrawIndexed");
        vkCmdDrawIndexed(ContextHelper::get_current_commandbuffer(
                             *p_Params.context),
                         p_Params.indexCount, p_Params.instanceCount,
                         p_Params.firstIndex, p_Params.vertexOffset,
                         p_Params.firstInstance);

        static Util::Name l_GlobalName = N(LOW_RENDERER_DRAWCALLS);
        Util::Globals::set(l_GlobalName,
                           ((int)Util::Globals::get(l_GlobalName)) +
                               1);
      }

      void vk_draw_indexed_indirect_count(
          Backend::Context &p_Context, Backend::Buffer &p_DrawBuffer,
          uint32_t p_DrawBufferOffset, Backend::Buffer &p_CountBuffer,
          uint32_t p_CountBufferOffset, uint32_t p_MaxDrawCount,
          uint32_t p_Stride)
      {
        vkCmdDrawIndexedIndirectCount(
            ContextHelper::get_current_commandbuffer(p_Context),
            p_DrawBuffer.vk.m_Buffer, p_DrawBufferOffset,
            p_CountBuffer.vk.m_Buffer, p_CountBufferOffset,
            p_MaxDrawCount, p_Stride);

        Util::Name l_GlobalName = N(LOW_RENDERER_DRAWCALLS);
        Util::Globals::set(l_GlobalName,
                           ((int)Util::Globals::get(l_GlobalName)) +
                               1);
      }

      void vk_buffer_create(Backend::Buffer &p_Buffer,
                            Backend::BufferCreateParams &p_Params)
      {
        p_Buffer.context = p_Params.context;
        p_Buffer.usageFlags = p_Params.usageFlags;

        VkDeviceSize l_BufferSize = p_Params.bufferSize;
        p_Buffer.bufferSize = p_Params.bufferSize;

        VkBuffer l_StagingBuffer;
        VkDeviceMemory l_StagingBufferMemory;

        if (p_Params.data) {
          Helper::create_buffer(
              *p_Params.context, l_BufferSize,
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              l_StagingBuffer, l_StagingBufferMemory);

          void *l_Data;
          vkMapMemory(p_Params.context->vk.m_Device,
                      l_StagingBufferMemory, 0, l_BufferSize, 0,
                      &l_Data);
          memcpy(l_Data, p_Params.data, (size_t)l_BufferSize);
          vkUnmapMemory(p_Params.context->vk.m_Device,
                        l_StagingBufferMemory);
        }

        VkBufferUsageFlags l_UsageFlage =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if ((p_Params.usageFlags &
             LOW_RENDERER_BUFFER_USAGE_VERTEX) ==
            LOW_RENDERER_BUFFER_USAGE_VERTEX) {
          l_UsageFlage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if ((p_Params.usageFlags & LOW_RENDERER_BUFFER_USAGE_INDEX) ==
            LOW_RENDERER_BUFFER_USAGE_INDEX) {
          l_UsageFlage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if ((p_Params.usageFlags &
             LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT) ==
            LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT) {
          l_UsageFlage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if ((p_Params.usageFlags &
             LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER) ==
            LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER) {
          l_UsageFlage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if ((p_Params.usageFlags &
             LOW_RENDERER_BUFFER_USAGE_INDIRECT) ==
            LOW_RENDERER_BUFFER_USAGE_INDIRECT) {
          l_UsageFlage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        VkMemoryPropertyFlags l_MemoryFlags;
        if (p_Params.memoryType ==
            Backend::BufferMemoryType::DEFAULT) {
          l_MemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        } else if (p_Params.memoryType ==
                   Backend::BufferMemoryType::HOST_VISIBLE) {
          l_MemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        } else {
          LOW_ASSERT(false, "Unknown buffer memory property");
        }

        Helper::create_buffer(*p_Params.context, l_BufferSize,
                              l_UsageFlage, l_MemoryFlags,
                              p_Buffer.vk.m_Buffer,
                              p_Buffer.vk.m_Memory);

        if (p_Params.data) {
          Helper::copy_buffer(*p_Params.context, l_StagingBuffer,
                              p_Buffer.vk.m_Buffer, l_BufferSize);

          vkDestroyBuffer(p_Params.context->vk.m_Device,
                          l_StagingBuffer, nullptr);
          vkFreeMemory(p_Params.context->vk.m_Device,
                       l_StagingBufferMemory, nullptr);
        }
      }

      void vk_buffer_cleanup(Backend::Buffer &p_Buffer)
      {
        vkDestroyBuffer(p_Buffer.context->vk.m_Device,
                        p_Buffer.vk.m_Buffer, nullptr);
        vkFreeMemory(p_Buffer.context->vk.m_Device,
                     p_Buffer.vk.m_Memory, nullptr);
      }

      void vk_buffer_read(Backend::Buffer &p_Buffer, void *p_Data,
                          uint32_t p_DataSize, uint32_t p_Start)
      {

        VkBuffer l_StagingBuffer;
        VkDeviceMemory l_StagingBufferMemory;
        void *l_Data;
        uint32_t l_StagingBufferOffset = 0;
        uint32_t l_ReadOffset = 0;

        bool l_Running = p_Buffer.context->running;

        if (l_Running) {

          l_StagingBuffer = p_Buffer.context->vk.m_ReadStagingBuffer;
          l_StagingBufferMemory =
              p_Buffer.context->vk.m_ReadStagingBufferMemory;

          l_StagingBufferOffset =
              ContextHelper::read_staging_buffer_get_free_block(
                  *p_Buffer.context, p_DataSize);

          l_ReadOffset = p_Buffer.vk.m_StagingBufferReadOffset;
        } else {
          Helper::create_buffer(
              *p_Buffer.context, p_DataSize,
              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              l_StagingBuffer, l_StagingBufferMemory);

          l_StagingBufferOffset = 0u;
          l_ReadOffset = 0u;
        }

        vkMapMemory(p_Buffer.context->vk.m_Device,
                    l_StagingBufferMemory, l_ReadOffset, p_DataSize,
                    0, &l_Data);

        Helper::copy_buffer(*p_Buffer.context, p_Buffer.vk.m_Buffer,
                            l_StagingBuffer, p_DataSize, p_Start,
                            l_StagingBufferOffset);

        memcpy(p_Data, l_Data, (size_t)p_DataSize);
        vkUnmapMemory(p_Buffer.context->vk.m_Device,
                      l_StagingBufferMemory);

        if (l_Running) {
          p_Buffer.vk.m_StagingBufferReadOffset =
              l_StagingBufferOffset;
        } else {
          vkDestroyBuffer(p_Buffer.context->vk.m_Device,
                          l_StagingBuffer, nullptr);
          vkFreeMemory(p_Buffer.context->vk.m_Device,
                       l_StagingBufferMemory, nullptr);
        }
      }

      void vk_buffer_write(Backend::Buffer &p_Buffer, void *p_Data,
                           uint32_t p_DataSize, uint32_t p_Start)
      {

        VkBuffer l_StagingBuffer;
        VkDeviceMemory l_StagingBufferMemory;
        void *l_Data;
        uint32_t l_StagingBufferOffset = 0;
        Backend::Context *l_Context = p_Buffer.context;

        if (p_Buffer.context->running) {
          if (l_Context->debugEnabled) {
            LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
                *l_Context, "Write buffer - Transfer",
                Math::Color(0.641f, 0.2249f, 0.4341f, 1.0f));
          }
          l_StagingBuffer = p_Buffer.context->vk.m_StagingBuffer;
          l_StagingBufferMemory =
              p_Buffer.context->vk.m_StagingBufferMemory;

          l_StagingBufferOffset =
              ContextHelper::staging_buffer_get_free_block(
                  *p_Buffer.context, p_DataSize);

          vkMapMemory(p_Buffer.context->vk.m_Device,
                      l_StagingBufferMemory, l_StagingBufferOffset,
                      p_DataSize, 0, &l_Data);

        } else {
          if (l_Context->debugEnabled) {
            LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
                *l_Context, "Write buffer - Immediate",
                Math::Color(0.641f, 0.2249f, 0.4341f, 1.0f));
          }
          Helper::create_buffer(
              *p_Buffer.context, p_DataSize,
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              l_StagingBuffer, l_StagingBufferMemory);

          vkMapMemory(p_Buffer.context->vk.m_Device,
                      l_StagingBufferMemory, 0, p_DataSize, 0,
                      &l_Data);

          l_StagingBufferOffset = 0;
        }

        memcpy(l_Data, p_Data, (size_t)p_DataSize);
        vkUnmapMemory(p_Buffer.context->vk.m_Device,
                      l_StagingBufferMemory);

        Helper::copy_buffer(*p_Buffer.context, l_StagingBuffer,
                            p_Buffer.vk.m_Buffer, p_DataSize,
                            l_StagingBufferOffset, p_Start);

        if (!p_Buffer.context->running) {
          vkDestroyBuffer(p_Buffer.context->vk.m_Device,
                          l_StagingBuffer, nullptr);
          vkFreeMemory(p_Buffer.context->vk.m_Device,
                       l_StagingBufferMemory, nullptr);
        }
        if (l_Context->debugEnabled) {
          LOW_RENDERER_END_RENDERDOC_SECTION(*l_Context);
        }
      }

      void vk_buffer_set(Backend::Buffer &p_Buffer, void *p_Data)
      {
        vk_buffer_write(p_Buffer, p_Data, p_Buffer.bufferSize, 0);
      }

      void vk_buffer_bind_vertex(Backend::Buffer &p_Buffer)
      {
        LOW_ASSERT((p_Buffer.usageFlags &
                    LOW_RENDERER_BUFFER_USAGE_VERTEX) ==
                       LOW_RENDERER_BUFFER_USAGE_VERTEX,
                   "Tried to bind buffer without proper usage flag "
                   "as vertex "
                   "buffer");

        VkDeviceSize l_Offsets[] = {0};

        vkCmdBindVertexBuffers(
            ContextHelper::get_current_commandbuffer(
                *p_Buffer.context),
            0, 1, &(p_Buffer.vk.m_Buffer), l_Offsets);
      }

      void vk_buffer_bind_index(Backend::Buffer &p_Buffer,
                                uint8_t p_IndexBufferType)
      {
        LOW_ASSERT(
            (p_Buffer.usageFlags & LOW_RENDERER_BUFFER_USAGE_INDEX) ==
                LOW_RENDERER_BUFFER_USAGE_INDEX,
            "Tried to bind buffer without proper usage flag as index "
            "buffer");
        VkIndexType l_IndexType = VK_INDEX_TYPE_UINT16;

        if (p_IndexBufferType == Backend::IndexBufferType::UINT8) {
          l_IndexType = VK_INDEX_TYPE_UINT8_EXT;
        } else if (p_IndexBufferType ==
                   Backend::IndexBufferType::UINT16) {
          l_IndexType = VK_INDEX_TYPE_UINT16;
        } else if (p_IndexBufferType ==
                   Backend::IndexBufferType::UINT32) {
          l_IndexType = VK_INDEX_TYPE_UINT32;
        } else {
          LOW_ASSERT(false, "Unknown index type");
        }

        vkCmdBindIndexBuffer(ContextHelper::get_current_commandbuffer(
                                 *p_Buffer.context),
                             p_Buffer.vk.m_Buffer, 0, l_IndexType);
      }

      Util::String get_source_path_vk_glsl(Util::String p_Path)
      {
        return Util::get_project().dataPath + "/shader/src/vk_glsl/" +
               p_Path;
      }

      static Util::String compile_vk_glsl_to_spv(Util::String p_Path)
      {
        Util::String l_OutPath = Util::get_project().dataPath +
                                 "/shader/dst/spv/" + p_Path + ".spv";

#if LOW_RENDERER_COMPILE_SHADERS
        Util::String l_SourcePath = get_source_path_vk_glsl(p_Path);

        Util::String l_Command =
            "glslc " + l_SourcePath + " -o " + l_OutPath;

        Util::String l_Notice = "Compiling shader " + l_SourcePath;

        LOW_LOG_DEBUG << l_Notice << LOW_LOG_END;
        system(l_Command.c_str());
#endif

        return l_OutPath;
      }

      void vk_renderdoc_section_begin(Backend::Context &p_Context,
                                      Util::String p_SectionLabel,
                                      Math::Color p_Color)
      {
        if (!p_Context.vk.m_ValidationEnabled) {
          return;
        }

        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(p_Context);

        VkDebugUtilsLabelEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        markerInfo.pLabelName = p_SectionLabel.c_str();
        markerInfo.color[0] = p_Color.r;
        markerInfo.color[1] = p_Color.g;
        markerInfo.color[2] = p_Color.b;
        markerInfo.color[3] = p_Color.a;

        auto l_Function =
            (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
                p_Context.vk.m_Instance,
                "vkCmdBeginDebugUtilsLabelEXT");

        if (l_Function != nullptr) {
          l_Function(l_CommandBuffer, &markerInfo);
        }
      }

      void vk_renderdoc_section_end(Backend::Context &p_Context)
      {
        if (!p_Context.vk.m_ValidationEnabled) {
          return;
        }

        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(p_Context);

        auto l_Function =
            (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
                p_Context.vk.m_Instance,
                "vkCmdEndDebugUtilsLabelEXT");

        if (l_Function != nullptr) {
          l_Function(l_CommandBuffer);
        }
      }

      void vk_imgui_image_create(Backend::ImGuiImage &p_ImGuiImage,
                                 Backend::ImageResource &p_Image)
      {
        uint32_t l_FramesInFlight = p_Image.context->framesInFlight;
        p_ImGuiImage.context = p_Image.context;

        p_ImGuiImage.vk.m_DescriptorSets =
            (VkDescriptorSet *)Util::Memory::main_allocator()
                ->allocate(sizeof(VkDescriptorSet) *
                           l_FramesInFlight);

        // TODO: Transition imag eto correct layout after resize

        for (uint32_t i = 0u; i < l_FramesInFlight; ++i) {
          p_ImGuiImage.vk.m_DescriptorSets[i] =
              ImGui_ImplVulkan_AddTexture(
                  p_Image.vk.m_Sampler, p_Image.vk.m_ImageView,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
      }

      void vk_imgui_image_cleanup(Backend::ImGuiImage &p_ImGuiImage)
      {
        Util::Memory::main_allocator()->deallocate(
            p_ImGuiImage.vk.m_DescriptorSets);
      }

      void vk_imgui_image_render(Backend::ImGuiImage &p_ImGuiImage,
                                 Math::UVector2 &p_Dimensions)
      {
        ImGui::Image(
            p_ImGuiImage.vk.m_DescriptorSets[p_ImGuiImage.context
                                                 ->currentFrameIndex],
            ImVec2{(float)p_Dimensions.x, (float)p_Dimensions.y});
      }

      void
      initialize_callback(Backend::ApiBackendCallback &p_Callbacks)
      {
        p_Callbacks.context_create = &vk_context_create;
        p_Callbacks.context_cleanup = &vk_context_cleanup;
        p_Callbacks.context_wait_idle = &vk_context_wait_idle;
        p_Callbacks.context_update_dimensions =
            &vk_context_update_dimensions;

        p_Callbacks.frame_prepare = &vk_frame_prepare;
        p_Callbacks.frame_render = &vk_frame_render;
        p_Callbacks.imgui_prepare_frame = &vk_imgui_prepare_frame;
        p_Callbacks.imgui_render = &vk_imgui_render;

        p_Callbacks.renderpass_create = &vk_renderpass_create;
        p_Callbacks.renderpass_cleanup = &vk_renderpass_cleanup;
        p_Callbacks.renderpass_begin = &vk_renderpass_begin;
        p_Callbacks.renderpass_end = &vk_renderpass_end;

        p_Callbacks.pipeline_resource_signature_create =
            &vk_pipeline_resource_signature_create;
        p_Callbacks.pipeline_resource_signature_set_constant_buffer =
            &vk_pipeline_resource_signature_set_constant_buffer;
        p_Callbacks.pipeline_resource_signature_set_buffer =
            &vk_pipeline_resource_signature_set_buffer;
        p_Callbacks.pipeline_resource_signature_set_image =
            &vk_pipeline_resource_signature_set_image;
        p_Callbacks.pipeline_resource_signature_set_sampler =
            &vk_pipeline_resource_signature_set_sampler;
        p_Callbacks.pipeline_resource_signature_set_unbound_sampler =
            &vk_pipeline_resource_signature_set_unbound_sampler;
        p_Callbacks.pipeline_resource_signature_set_texture2d =
            &vk_pipeline_resource_signature_set_texture2d;
        p_Callbacks.pipeline_resource_signature_commit =
            &vk_pipeline_resource_signature_commit;
        p_Callbacks.pipeline_resource_signature_commit_clear =
            &vk_pipeline_resource_signature_commit_clear;

        p_Callbacks.pipeline_compute_create =
            &vk_pipeline_compute_create;
        p_Callbacks.pipeline_graphics_create =
            &vk_pipeline_graphics_create;
        p_Callbacks.pipeline_cleanup = &vk_pipeline_cleanup;
        p_Callbacks.pipeline_bind = &vk_pipeline_bind;
        p_Callbacks.pipeline_set_constant = &vk_pipeline_set_constant;

        p_Callbacks.compute_dispatch = &vk_compute_dispatch;
        p_Callbacks.draw = &vk_draw;
        p_Callbacks.draw_indexed = &vk_draw_indexed;
        p_Callbacks.get_draw_indexed_indirect_info_size =
            &vk_get_draw_indexed_indirect_info_size;
        p_Callbacks.draw_indexed_indirect_info_fill =
            &vk_draw_indexed_indirect_info_fill;
        p_Callbacks.draw_indexed_indirect_count =
            &vk_draw_indexed_indirect_count;

        p_Callbacks.imageresource_create = &vk_imageresource_create;
        p_Callbacks.imageresource_cleanup = &vk_imageresource_cleanup;

        p_Callbacks.buffer_create = &vk_buffer_create;
        p_Callbacks.buffer_cleanup = &vk_buffer_cleanup;
        p_Callbacks.buffer_set = &vk_buffer_set;
        p_Callbacks.buffer_write = &vk_buffer_write;
        p_Callbacks.buffer_read = &vk_buffer_read;
        p_Callbacks.buffer_bind_vertex = &vk_buffer_bind_vertex;
        p_Callbacks.buffer_bind_index = &vk_buffer_bind_index;

        p_Callbacks.compile = &compile_vk_glsl_to_spv;
        p_Callbacks.get_shader_source_path = &get_source_path_vk_glsl;

        p_Callbacks.renderdoc_section_begin =
            &vk_renderdoc_section_begin;
        p_Callbacks.renderdoc_section_end = &vk_renderdoc_section_end;

        p_Callbacks.imgui_image_create = &vk_imgui_image_create;
        p_Callbacks.imgui_image_cleanup = &vk_imgui_image_cleanup;
        p_Callbacks.imgui_image_render = &vk_imgui_image_render;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low

#undef SKIP_DEBUG_LEVEL
