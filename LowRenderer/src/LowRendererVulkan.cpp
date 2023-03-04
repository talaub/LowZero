#include "vulkan/vulkan_core.h"
#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000
#include "../../LowDependencies/VulkanMemoryAllocator/include/vk_mem_alloc.h"

#include "LowRendererVulkan.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilContainers.h"
#include "LowUtilFileIO.h"

#include "LowMath.h"

#include "LowRendererBackend.h"
#include "LowRendererImage.h"

#include <GLFW/glfw3.h>

#include <corecrt_malloc.h>
#include <stdint.h>
#include <string>

#define SKIP_DEBUG_LEVEL true

#define MAX_PRS 255

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      void vk_renderpass_create(Backend::Renderpass &p_Renderpass,
                                Backend::RenderpassCreateParams &p_Params);
      void vk_renderpass_cleanup(Backend::Renderpass &p_Renderpass);
      void
      vk_imageresource_create(Backend::ImageResource &p_Image,
                              Backend::ImageResourceCreateParams &p_Params);
      void vk_imageresource_cleanup(Backend::ImageResource &p_Image);

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

          bool is_complete()
          {
            return m_GraphicsFamily.has_value() && m_PresentFamily.has_value();
          }
        };

        uint32_t find_memory_type(Context &p_Context, uint32_t p_TypeFilter,
                                  VkMemoryPropertyFlags p_Properties)
        {
          VkPhysicalDeviceMemoryProperties l_MemProperties;
          vkGetPhysicalDeviceMemoryProperties(p_Context.m_PhysicalDevice,
                                              &l_MemProperties);

          for (uint32_t i_Iter = 0; i_Iter < l_MemProperties.memoryTypeCount;
               i_Iter++) {
            if ((p_TypeFilter & (1 << i_Iter)) &&
                (l_MemProperties.memoryTypes[i_Iter].propertyFlags &
                 p_Properties) == p_Properties) {
              return i_Iter;
            }
          }
          LOW_ASSERT(false, "Failed to find suitable memory type");
        }

        static VkSurfaceFormatKHR choose_swap_surface_format(
            const Low::Util::List<VkSurfaceFormatKHR> &p_AvailableFormats)
        {
          for (const auto &i_AvailableFormat : p_AvailableFormats) {
            if (i_AvailableFormat.format == VK_FORMAT_B8G8R8A8_SRGB) {
              return i_AvailableFormat;
            }
          }
          return p_AvailableFormats[0];
        }

        static SwapChainSupportDetails
        query_swap_chain_support(Context &p_Context, VkPhysicalDevice p_Device)
        {
          SwapChainSupportDetails l_Details;

          // Query device surface capabilities
          vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
              p_Device, p_Context.m_Surface, &l_Details.m_Capabilities);

          // Query supported surface formats
          uint32_t l_FormatCount;
          vkGetPhysicalDeviceSurfaceFormatsKHR(p_Device, p_Context.m_Surface,
                                               &l_FormatCount, nullptr);

          if (l_FormatCount != 0) {
            l_Details.m_Formats.resize(l_FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(p_Device, p_Context.m_Surface,
                                                 &l_FormatCount,
                                                 l_Details.m_Formats.data());
          }

          // Query supported presentations modes
          uint32_t l_PresentModeCount;
          vkGetPhysicalDeviceSurfacePresentModesKHR(
              p_Device, p_Context.m_Surface, &l_PresentModeCount, nullptr);

          if (l_PresentModeCount != 0) {
            l_Details.m_PresentModes.resize(l_PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                p_Device, p_Context.m_Surface, &l_PresentModeCount,
                l_Details.m_PresentModes.data());
          }

          return l_Details;
        }

        static VkPresentModeKHR choose_swap_present_mode(
            const Low::Util::List<VkPresentModeKHR> &p_AvailablePresentModes)
        {
          // We look for our prefered present mode
          for (const auto &i_AvalablePresentMode : p_AvailablePresentModes) {
            if (i_AvalablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
              return i_AvalablePresentMode;
            }
          }

          // The FIFO_KHR mode is the only mode that is guaranteed to be
          // supported so this will be our fallback
          return VK_PRESENT_MODE_FIFO_KHR;
        }

        static Helper::QueueFamilyIndices
        find_queue_families(Context &p_Context, VkPhysicalDevice p_Device)
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
            vkGetPhysicalDeviceSurfaceSupportKHR(
                p_Device, l_Iterator, p_Context.m_Surface, &i_PresentSupport);
            if (i_PresentSupport) {
              l_Indices.m_PresentFamily = l_Iterator;
            }

            if (i_QueueFamiliy.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
              l_Indices.m_GraphicsFamily = l_Iterator;
            }
            l_Iterator++;
          }

          return l_Indices;
        }

        static VkFormat find_supported_format(
            Context &p_Context, const Low::Util::List<VkFormat> &p_Candidates,
            VkImageTiling p_Tiling, VkFormatFeatureFlags p_Features)
        {
          for (VkFormat i_Format : p_Candidates) {
            VkFormatProperties i_Properties;
            vkGetPhysicalDeviceFormatProperties(p_Context.m_PhysicalDevice,
                                                i_Format, &i_Properties);

            if (p_Tiling == VK_IMAGE_TILING_LINEAR &&
                (i_Properties.linearTilingFeatures & p_Features) ==
                    p_Features) {
              return i_Format;
            } else if (p_Tiling == VK_IMAGE_TILING_OPTIMAL &&
                       (i_Properties.optimalTilingFeatures & p_Features) ==
                           p_Features) {
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
          case VK_FORMAT_R32G32B32A32_SFLOAT:
            return Backend::ImageFormat::RGBA32_SFLOAT;
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
          case Backend::ImageFormat::RGBA32_SFLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
          default:
            LOW_ASSERT(false, "Unknown image format");
            return VK_FORMAT_A2B10G10R10_SINT_PACK32;
          }
        }

        static VkCommandBuffer
        begin_single_time_commands(Backend::Context &p_Context)
        {
          VkCommandBufferAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          l_AllocInfo.commandPool = p_Context.vk.m_CommandPool;
          l_AllocInfo.commandBufferCount = 1;

          VkCommandBuffer l_CommandBuffer;
          vkAllocateCommandBuffers(p_Context.vk.m_Device, &l_AllocInfo,
                                   &l_CommandBuffer);

          VkCommandBufferBeginInfo l_BeginInfo{};
          l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
          l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

          LOW_ASSERT(vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo) ==
                         VK_SUCCESS,
                     "Failed to begine one time command buffer");

          return l_CommandBuffer;
        }

        static void end_single_time_commands(Backend::Context &p_Context,
                                             VkCommandBuffer p_CommandBuffer)
        {
          vkEndCommandBuffer(p_CommandBuffer);

          VkSubmitInfo l_SubmitInfo{};
          l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
          l_SubmitInfo.commandBufferCount = 1;
          l_SubmitInfo.pCommandBuffers = &p_CommandBuffer;

          vkQueueSubmit(p_Context.vk.m_GraphicsQueue, 1, &l_SubmitInfo,
                        VK_NULL_HANDLE);
          vkQueueWaitIdle(p_Context.vk.m_GraphicsQueue);
          vkDeviceWaitIdle(p_Context.vk.m_Device);

          vkFreeCommandBuffers(p_Context.vk.m_Device,
                               p_Context.vk.m_CommandPool, 1, &p_CommandBuffer);
        }

        void create_buffer(Backend::Context &p_Context, VkDeviceSize p_Size,
                           VkBufferUsageFlags p_Usage,
                           VkMemoryPropertyFlags p_Properties,
                           VkBuffer &p_Buffer, VkDeviceMemory &p_BufferMemory)
        {
          VkBufferCreateInfo l_BufferInfo{};
          l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
          l_BufferInfo.size = p_Size;
          l_BufferInfo.usage = p_Usage;
          l_BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

          LOW_ASSERT(vkCreateBuffer(p_Context.vk.m_Device, &l_BufferInfo,
                                    nullptr, &p_Buffer) == VK_SUCCESS,
                     "Failed to create buffer");

          VkMemoryRequirements l_MemoryRequirements;
          vkGetBufferMemoryRequirements(p_Context.vk.m_Device, p_Buffer,
                                        &l_MemoryRequirements);

          VkMemoryAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
          l_AllocInfo.allocationSize = l_MemoryRequirements.size;
          l_AllocInfo.memoryTypeIndex = find_memory_type(
              p_Context.vk, l_MemoryRequirements.memoryTypeBits, p_Properties);

          LOW_ASSERT(vkAllocateMemory(p_Context.vk.m_Device, &l_AllocInfo,
                                      nullptr, &p_BufferMemory) == VK_SUCCESS,
                     "Failed to allocate buffer memor");

          vkBindBufferMemory(p_Context.vk.m_Device, p_Buffer, p_BufferMemory,
                             0);
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
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        static bool check_validation_layer_support()
        {
          uint32_t l_LayerCount;
          vkEnumerateInstanceLayerProperties(&l_LayerCount, nullptr);

          Low::Util::List<VkLayerProperties> l_AvailableLayers(l_LayerCount);
          vkEnumerateInstanceLayerProperties(&l_LayerCount,
                                             l_AvailableLayers.data());
          for (const char *i_LayerName : g_ValidationLayers) {
            bool layer_found = false;

            for (const auto &i_LayerProperties : l_AvailableLayers) {
              if (strcmp(i_LayerName, i_LayerProperties.layerName) == 0) {
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
            const VkDebugUtilsMessengerCallbackDataEXT *p_CallbackData,
            void *p_UserData)
        {

          if (p_MessageSeverity ==
                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT &&
              !SKIP_DEBUG_LEVEL) {
            LOW_LOG_DEBUG((std::string("Validation layer: ") +
                           std::string(p_CallbackData->pMessage))
                              .c_str());
          } else if (p_MessageSeverity ==
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            LOW_LOG_INFO((std::string("Validation layer: ") +
                          std::string(p_CallbackData->pMessage))
                             .c_str());
          } else if (p_MessageSeverity ==
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            LOW_LOG_WARN((std::string("Validation layer: ") +
                          std::string(p_CallbackData->pMessage))
                             .c_str());
          } else if (p_MessageSeverity ==
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            LOW_LOG_ERROR((std::string("Validation layer: ") +
                           std::string(p_CallbackData->pMessage))
                              .c_str());
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
          auto l_Function =
              (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                  p_Instance, "vkCreateDebugUtilsMessengerEXT");

          if (l_Function != nullptr) {
            return l_Function(p_Instance, p_CreateInfo, p_Allocator,
                              p_DebugMessenger);
          } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
          }
        }

        static void destroy_debug_utils_messenger_ext(
            VkInstance p_Instance, VkDebugUtilsMessengerEXT p_DebugMessenger,
            const VkAllocationCallbacks *p_Allocator)
        {
          auto l_Function =
              (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
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
          l_AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
          l_AppInfo.pEngineName = "Low";
          l_AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
          l_AppInfo.apiVersion = VK_API_VERSION_1_0;

          // Setup createinfo parameter struct
          VkInstanceCreateInfo l_CreateInfo{};
          l_CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
          l_CreateInfo.pApplicationInfo = &l_AppInfo;

          // TL TODO: GLFW hardcoded
          // Load glfw extensions
          uint32_t l_GlfwExtensionCount = 0u;
          const char **l_GlfwExtensions;
          l_GlfwExtensions =
              glfwGetRequiredInstanceExtensions(&l_GlfwExtensionCount);
          Low::Util::List<const char *> l_Extensions(
              l_GlfwExtensions, l_GlfwExtensions + l_GlfwExtensionCount);

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
            l_CreateInfo.ppEnabledLayerNames = g_ValidationLayers.data();

            populate_debug_messenger_create_info(l_DebugCreateInfo);
            l_CreateInfo.pNext =
                (VkDebugUtilsMessengerCreateInfoEXT *)&l_DebugCreateInfo;
          } else {
            l_CreateInfo.enabledLayerCount = 0;
            l_CreateInfo.pNext = nullptr;
          }

          VkResult l_Result =
              vkCreateInstance(&l_CreateInfo, nullptr, &p_Context.m_Instance);
          LOW_ASSERT(l_Result == VK_SUCCESS, "Vulkan instance creation failed");
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
          return p_Context.vk.m_CommandBuffers[p_Context.currentFrameIndex];
        }

        static void create_surface(Backend::Context &p_Context)
        {
          // TL TODO: glfw hard coded
          LOW_ASSERT(glfwCreateWindowSurface(
                         p_Context.vk.m_Instance, p_Context.window.m_Glfw,
                         nullptr, &(p_Context.vk.m_Surface)) == VK_SUCCESS,
                     "Failed to create window surface");
        }

        static bool check_device_extension_support(VkPhysicalDevice p_Device)
        {
          uint32_t l_ExtensionCount;
          vkEnumerateDeviceExtensionProperties(p_Device, nullptr,
                                               &l_ExtensionCount, nullptr);

          Util::List<VkExtensionProperties> l_AvailableExtensions(
              l_ExtensionCount);
          vkEnumerateDeviceExtensionProperties(p_Device, nullptr,
                                               &l_ExtensionCount,
                                               l_AvailableExtensions.data());

          Util::Set<std::string> l_RequiredExtensions(
              g_DeviceExtensions.begin(), g_DeviceExtensions.end());

          // Remove the found extensions from the list of required extensions
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

          vkGetPhysicalDeviceProperties(p_Device, &l_DeviceProperties);
          vkGetPhysicalDeviceFeatures(p_Device, &l_DeviceFeatures);

          int l_Score = 0;

          // Dedicated gpus have way better perfomance
          if (l_DeviceProperties.deviceType ==
              VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            l_Score += 1000;
          }

          // Maximum possible size of textures affects graphics quality
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
                  .empty()) // TODO: Refactor to increase score based on the
                            // amount of modes/formats
            return 0; // Having 0 for either one of the two should result in a
                      // score of 0

          return l_Score;
        }

        static void select_physical_device(Context &p_Context)
        {
          uint32_t l_DeviceCount = 0u;
          vkEnumeratePhysicalDevices(p_Context.m_Instance, &l_DeviceCount,
                                     nullptr);

          LOW_ASSERT(l_DeviceCount > 0, "No physical GPU found");

          Low::Util::List<VkPhysicalDevice> l_Devices(l_DeviceCount);
          vkEnumeratePhysicalDevices(p_Context.m_Instance, &l_DeviceCount,
                                     l_Devices.data());

          Util::MultiMap<int, VkPhysicalDevice> l_Candidates;
          for (const auto &i_Device : l_Devices) {
            int l_Score = rate_device_suitability(p_Context, i_Device);
            l_Candidates.insert(eastl::make_pair(l_Score, i_Device));
          }

          LOW_ASSERT(l_Candidates.rbegin()->first > 0,
                     "No suitable physical GPU found");
          p_Context.m_PhysicalDevice = l_Candidates.rbegin()->second;

          LOW_LOG_DEBUG("Physical device selected");
        }

        static void create_logical_device(Context &p_Context)
        {
          Helper::QueueFamilyIndices l_Indices = Helper::find_queue_families(
              p_Context, p_Context.m_PhysicalDevice);

          Low::Util::List<VkDeviceQueueCreateInfo> l_QueueCreateInfos;
          Low::Util::Set<uint32_t> l_UniqueQueueFamilies = {
              l_Indices.m_GraphicsFamily.value(),
              l_Indices.m_PresentFamily.value()};

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
          VkDeviceCreateInfo l_CreateInfo{};
          l_CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
          l_CreateInfo.pQueueCreateInfos = l_QueueCreateInfos.data();
          l_CreateInfo.queueCreateInfoCount =
              static_cast<uint32_t>(l_QueueCreateInfos.size());
          l_CreateInfo.pEnabledFeatures = &l_DeviceFeatures;
          l_CreateInfo.enabledExtensionCount =
              static_cast<uint32_t>(g_DeviceExtensions.size());
          l_CreateInfo.ppEnabledExtensionNames = g_DeviceExtensions.data();

          if (p_Context.m_ValidationEnabled) {
            l_CreateInfo.enabledLayerCount =
                static_cast<uint32_t>(g_ValidationLayers.size());
            l_CreateInfo.ppEnabledLayerNames = g_ValidationLayers.data();
          } else {
            l_CreateInfo.enabledLayerCount = 0;
          }

          LOW_ASSERT(vkCreateDevice(p_Context.m_PhysicalDevice, &l_CreateInfo,
                                    nullptr,
                                    &(p_Context.m_Device)) == VK_SUCCESS,
                     "Failed to create logical device");

          LOW_LOG_DEBUG("Logical device created");

          // Create queues
          vkGetDeviceQueue(p_Context.m_Device,
                           l_Indices.m_GraphicsFamily.value(), 0,
                           &(p_Context.m_GraphicsQueue));
          vkGetDeviceQueue(p_Context.m_Device,
                           l_Indices.m_PresentFamily.value(), 0,
                           &(p_Context.m_PresentQueue));

          LOW_LOG_DEBUG("Queues created");
        }

        static VkExtent2D
        choose_swap_extent(Backend::Context &p_Context,
                           const VkSurfaceCapabilitiesKHR &p_Capabilities)
        {
          // If the extents are set to the max value of uint32_t then we need
          // some custom logic to determine the actualy size
          if (p_Capabilities.currentExtent.width != LOW_UINT32_MAX) {
            return p_Capabilities.currentExtent;
          }

          // Query the actualy extents from glfw
          int l_Width, l_Height;
          glfwGetFramebufferSize(p_Context.window.m_Glfw, &l_Width, &l_Height);

          VkExtent2D l_ActualExtent = {static_cast<uint32_t>(l_Width),
                                       static_cast<uint32_t>(l_Height)};

          // Clamp the extent to the min/max values of capabilities
          l_ActualExtent.width = Math::Util::clamp(
              l_ActualExtent.width, p_Capabilities.minImageExtent.width,
              p_Capabilities.maxImageExtent.width);
          l_ActualExtent.height = Math::Util::clamp(
              l_ActualExtent.height, p_Capabilities.minImageExtent.height,
              p_Capabilities.maxImageExtent.height);

          return l_ActualExtent;
        }

        static void create_render_targets(Backend::Context &p_Context,
                                          Low::Util::List<VkImage> &p_Images)
        {
          Context &l_Context = p_Context.vk;

          l_Context.m_SwapchainRenderTargets =
              (Backend::ImageResource *)Util::Memory::main_allocator()
                  ->allocate(sizeof(Backend::ImageResource) * p_Images.size());

          Math::UVector2 l_Dimensions = p_Context.dimensions;

          Backend::ImageResourceCreateParams l_Params;
          l_Params.context = &p_Context;
          l_Params.depth = false;
          l_Params.writable = false;
          l_Params.dimensions.x = l_Dimensions.x;
          l_Params.dimensions.y = l_Dimensions.y;
          l_Params.imageDataSize = 0;
          l_Params.imageData = nullptr;
          l_Params.format = p_Context.imageFormat;
          l_Params.createImage = false;

          for (size_t i_Iter = 0; i_Iter < p_Images.size(); i_Iter++) {
            l_Context.m_SwapchainRenderTargets[i_Iter].vk.m_Image =
                p_Images[i_Iter];

            vk_imageresource_create(l_Context.m_SwapchainRenderTargets[i_Iter],
                                    l_Params);

            l_Context.m_SwapchainRenderTargets[i_Iter].swapchainImage = true;
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
          l_SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

          VkFenceCreateInfo l_FenceInfo{};
          l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
          l_FenceInfo.flags =
              VK_FENCE_CREATE_SIGNALED_BIT; // Set to resolved immidiately to
                                            // not get stuck on frame one

          for (size_t i_Iter = 0; i_Iter < p_Context.framesInFlight; i_Iter++) {
            LOW_ASSERT(
                vkCreateSemaphore(
                    p_Context.vk.m_Device, &l_SemaphoreInfo, nullptr,
                    &(p_Context.vk.m_ImageAvailableSemaphores[i_Iter])) ==
                    VK_SUCCESS,
                "Failed to create semaphore");
            LOW_ASSERT(
                vkCreateSemaphore(
                    p_Context.vk.m_Device, &l_SemaphoreInfo, nullptr,
                    &(p_Context.vk.m_RenderFinishedSemaphores[i_Iter])) ==
                    VK_SUCCESS,
                "Failed to create semaphore");
            LOW_ASSERT(
                vkCreateFence(p_Context.vk.m_Device, &l_FenceInfo, nullptr,
                              &(p_Context.vk.m_InFlightFences[i_Iter])) ==
                    VK_SUCCESS,
                "Failed to create fence");
          }
        }

        static void create_command_buffers(Backend::Context &p_Context)
        {
          p_Context.vk.m_CommandBuffers =
              (VkCommandBuffer *)Util::Memory::main_allocator()->allocate(
                  p_Context.framesInFlight * sizeof(VkCommandBuffer));

          VkCommandBufferAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          l_AllocInfo.commandPool = p_Context.vk.m_CommandPool;
          l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          l_AllocInfo.commandBufferCount = 1;

          for (uint8_t i = 0; i < p_Context.framesInFlight; ++i) {
            LOW_ASSERT(vkAllocateCommandBuffers(
                           p_Context.vk.m_Device, &l_AllocInfo,
                           &(p_Context.vk.m_CommandBuffers[i])) == VK_SUCCESS,
                       "Failed to allocate command buffer");
          }
        }

        Util::List<Math::Color> g_SwapchainClearColors = {
            {0.0f, 1.0f, 0.0f, 1.0f}};
      } // namespace ContextHelper

      void vk_context_create(Backend::Context &p_Context,
                             Backend::ContextCreateParams &p_Params)
      {
        p_Context.framesInFlight = p_Params.framesInFlight;
        p_Context.vk.m_PipelineResourceSignatureIndex = 0u;
        p_Context.vk.m_PipelineResourceSignatures =
            (PipelineResourceSignatureInternal *)Util::Memory::main_allocator()
                ->allocate(sizeof(PipelineResourceSignatureInternal) * MAX_PRS);
        p_Context.vk.m_CommittedPipelineResourceSignatures =
            (uint32_t *)Util::Memory::main_allocator()->allocate(
                sizeof(uint32_t) * LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS);

        {
          // Check for validation layer support
          if (p_Params.validation_enabled) {
            LOW_ASSERT(ContextHelper::check_validation_layer_support(),
                       "Validation layers requested, but not available");
            LOW_LOG_DEBUG("Validation layers enabled");
          }

          p_Context.vk.m_ValidationEnabled = p_Params.validation_enabled;
          p_Context.window = *(p_Params.window);

          ContextHelper::create_instance(p_Context.vk);

          ContextHelper::setup_debug_messenger(p_Context.vk);

          ContextHelper::create_surface(p_Context);

          ContextHelper::select_physical_device(p_Context.vk);

          ContextHelper::create_logical_device(p_Context.vk);
        }
        Helper::QueueFamilyIndices l_QueueFamilyIndices =
            Helper::find_queue_families(p_Context.vk,
                                        p_Context.vk.m_PhysicalDevice);

        VmaAllocatorCreateInfo l_AllocCreateInfo = {};
        l_AllocCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        l_AllocCreateInfo.physicalDevice = p_Context.vk.m_PhysicalDevice;
        l_AllocCreateInfo.device = p_Context.vk.m_Device;
        l_AllocCreateInfo.instance = p_Context.vk.m_Instance;

        vmaCreateAllocator(&l_AllocCreateInfo, &(p_Context.vk.m_Alloc));

        // Create command pool
        {
          VkCommandPoolCreateInfo l_PoolInfo{};
          l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
          l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
          l_PoolInfo.queueFamilyIndex =
              l_QueueFamilyIndices.m_GraphicsFamily.value();

          LOW_ASSERT(
              vkCreateCommandPool(p_Context.vk.m_Device, &l_PoolInfo, nullptr,
                                  &(p_Context.vk.m_CommandPool)) == VK_SUCCESS,
              "Failed to create command pool");
        }

        {
          Context &l_Context = p_Context.vk;

          p_Context.currentFrameIndex = 0;

          Helper::SwapChainSupportDetails l_SwapChainSupportDetails =
              Helper::query_swap_chain_support(l_Context,
                                               l_Context.m_PhysicalDevice);

          VkSurfaceFormatKHR l_SurfaceFormat =
              Helper::choose_swap_surface_format(
                  l_SwapChainSupportDetails.m_Formats);
          VkPresentModeKHR l_PresentMode = Helper::choose_swap_present_mode(
              l_SwapChainSupportDetails.m_PresentModes);
          VkExtent2D l_Extent = ContextHelper::choose_swap_extent(
              p_Context, l_SwapChainSupportDetails.m_Capabilities);

          uint32_t l_ImageCount =
              l_SwapChainSupportDetails.m_Capabilities.minImageCount + 1;

          p_Context.imageCount = l_ImageCount;

          // Clamp the imagecount to not exceed the maximum
          // A set maximum of 0 means that there is no maximum
          if (l_SwapChainSupportDetails.m_Capabilities.maxImageCount > 0 &&
              l_ImageCount >
                  l_SwapChainSupportDetails.m_Capabilities.maxImageCount) {
            l_ImageCount =
                l_SwapChainSupportDetails.m_Capabilities.maxImageCount;
          }

          VkSwapchainCreateInfoKHR l_CreateInfo{};
          l_CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
          l_CreateInfo.surface = l_Context.m_Surface;
          l_CreateInfo.minImageCount = l_ImageCount;
          l_CreateInfo.imageFormat = l_SurfaceFormat.format;
          l_CreateInfo.imageColorSpace = l_SurfaceFormat.colorSpace;
          l_CreateInfo.imageExtent = l_Extent;
          l_CreateInfo.imageArrayLayers = 1;
          l_CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

          Helper::QueueFamilyIndices l_Indices = Helper::find_queue_families(
              l_Context, l_Context.m_PhysicalDevice);
          uint32_t l_QueueFamilyIndices[] = {l_Indices.m_GraphicsFamily.value(),
                                             l_Indices.m_PresentFamily.value()};

          if (l_Indices.m_GraphicsFamily != l_Indices.m_PresentFamily) {
            l_CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            l_CreateInfo.queueFamilyIndexCount = 2;
            l_CreateInfo.pQueueFamilyIndices = l_QueueFamilyIndices;
          } else {
            l_CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            l_CreateInfo.queueFamilyIndexCount = 0;
            l_CreateInfo.pQueueFamilyIndices = nullptr;
          }

          l_CreateInfo.preTransform =
              l_SwapChainSupportDetails.m_Capabilities.currentTransform;
          l_CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
          l_CreateInfo.presentMode = l_PresentMode;
          l_CreateInfo.clipped = VK_TRUE;
          l_CreateInfo.oldSwapchain = VK_NULL_HANDLE;

          // Create swap chain
          LOW_ASSERT(vkCreateSwapchainKHR(l_Context.m_Device, &l_CreateInfo,
                                          nullptr, &(l_Context.m_Swapchain)) ==
                         VK_SUCCESS,
                     "Could not create swap chain");
          LOW_LOG_DEBUG("Swapchain created");

          // Retrieve the vkimages of the swapchain
          vkGetSwapchainImagesKHR(l_Context.m_Device, l_Context.m_Swapchain,
                                  &l_ImageCount, nullptr);

          Low::Util::List<VkImage> l_Images;
          l_Images.resize(l_ImageCount);
          vkGetSwapchainImagesKHR(l_Context.m_Device, l_Context.m_Swapchain,
                                  &l_ImageCount, l_Images.data());

          p_Context.imageFormat =
              Helper::vkformat_to_imageformat(l_SurfaceFormat.format);

          // Store some swapchain info
          p_Context.dimensions =
              Math::UVector2(l_Extent.width, l_Extent.height);

          ContextHelper::create_render_targets(p_Context, l_Images);

          p_Context.renderpasses =
              (Backend::Renderpass *)Util::Memory::main_allocator()->allocate(
                  sizeof(Backend::Renderpass) * p_Context.imageCount);

          for (uint8_t i = 0; i < p_Context.imageCount; ++i) {
            Backend::RenderpassCreateParams l_RenderPassParams;
            l_RenderPassParams.context = &p_Context;
            l_RenderPassParams.renderTargetCount = 1;
            l_RenderPassParams.renderTargets =
                &(p_Context.vk.m_SwapchainRenderTargets[i]);
            l_RenderPassParams.clearTargetColor =
                ContextHelper::g_SwapchainClearColors.data();
            l_RenderPassParams.useDepth = false;
            l_RenderPassParams.dimensions = p_Context.dimensions;

            vk_renderpass_create(p_Context.renderpasses[i], l_RenderPassParams);
          }

          ContextHelper::create_sync_objects(p_Context);
          ContextHelper::create_command_buffers(p_Context);
        }
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
          l_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
          l_PoolInfo.poolSizeCount = static_cast<uint32_t>(l_PoolSizes.size());
          l_PoolInfo.pPoolSizes = l_PoolSizes.data();
          l_PoolInfo.maxSets = 100;

          LOW_ASSERT(vkCreateDescriptorPool(
                         p_Context.vk.m_Device, &l_PoolInfo, nullptr,
                         &(p_Context.vk.m_DescriptorPool)) == VK_SUCCESS,
                     "Failed to create descriptor pools");
        }
      }

      void vk_context_cleanup(Backend::Context &p_Context)
      {
        Context l_Context = p_Context.vk;

        for (uint32_t i = 0u; i < p_Context.framesInFlight; ++i) {
          vkDestroySemaphore(l_Context.m_Device,
                             l_Context.m_ImageAvailableSemaphores[i], nullptr);
          vkDestroySemaphore(l_Context.m_Device,
                             l_Context.m_RenderFinishedSemaphores[i], nullptr);
          vkDestroyFence(l_Context.m_Device, l_Context.m_InFlightFences[i],
                         nullptr);
        }

        Util::Memory::main_allocator()->deallocate(
            l_Context.m_ImageAvailableSemaphores);
        Util::Memory::main_allocator()->deallocate(
            l_Context.m_RenderFinishedSemaphores);
        Util::Memory::main_allocator()->deallocate(l_Context.m_InFlightFences);

        for (uint32_t i = 0u; i < p_Context.imageCount; ++i) {
          vk_renderpass_cleanup(p_Context.renderpasses[i]);
          vk_imageresource_cleanup(p_Context.vk.m_SwapchainRenderTargets[i]);
        }

        Util::Memory::main_allocator()->deallocate(
            p_Context.vk.m_SwapchainRenderTargets);
        Util::Memory::main_allocator()->deallocate(p_Context.renderpasses);

        vkDestroySwapchainKHR(l_Context.m_Device, l_Context.m_Swapchain,
                              nullptr);

        Util::Memory::main_allocator()->deallocate(
            p_Context.vk.m_CommittedPipelineResourceSignatures);

        for (uint32_t i = 0u; i < p_Context.vk.m_PipelineResourceSignatureIndex;
             ++i) {
          vkDestroyDescriptorSetLayout(
              l_Context.m_Device,
              l_Context.m_PipelineResourceSignatures[i].m_DescriptorSetLayout,
              nullptr);
          Util::Memory::main_allocator()->deallocate(
              p_Context.vk.m_PipelineResourceSignatures[i].m_DescriptorSets);
          Util::Memory::main_allocator()->deallocate(
              p_Context.vk.m_PipelineResourceSignatures[i].m_Bindings);
        }
        Util::Memory::main_allocator()->deallocate(
            p_Context.vk.m_PipelineResourceSignatures);

        Util::Memory::main_allocator()->deallocate(l_Context.m_CommandBuffers);

        {
          vkDestroyDescriptorPool(p_Context.vk.m_Device,
                                  p_Context.vk.m_DescriptorPool, nullptr);
          vkDestroyCommandPool(p_Context.vk.m_Device,
                               p_Context.vk.m_CommandPool, nullptr);
        }

        vmaDestroyAllocator(p_Context.vk.m_Alloc);

        // Clean devices and instance
        {
          vkDestroyDevice(l_Context.m_Device, nullptr);

          vkDestroySurfaceKHR(l_Context.m_Instance, l_Context.m_Surface,
                              nullptr);

          if (l_Context.m_ValidationEnabled) {
            ContextHelper::destroy_debug_utils_messenger_ext(
                l_Context.m_Instance, l_Context.m_DebugMessenger, nullptr);
          }

          vkDestroyInstance(l_Context.m_Instance, nullptr);
        }
      }

      void vk_context_wait_idle(Backend::Context &p_Context)
      {
        vkDeviceWaitIdle(p_Context.vk.m_Device);
      }

      uint8_t vk_frame_prepare(Backend::Context &p_Context)
      {
        vkWaitForFences(
            p_Context.vk.m_Device, 1,
            &p_Context.vk.m_InFlightFences[p_Context.currentFrameIndex],
            VK_TRUE, UINT64_MAX);

        uint32_t l_CurrentImage;

        VkResult l_Result = vkAcquireNextImageKHR(
            p_Context.vk.m_Device, p_Context.vk.m_Swapchain, UINT64_MAX,
            p_Context.vk
                .m_ImageAvailableSemaphores[p_Context.currentFrameIndex],
            VK_NULL_HANDLE, &l_CurrentImage);

        p_Context.currentImageIndex = l_CurrentImage;

        // Handle window resize
        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR) {
          return Backend::ContextState::OUT_OF_DATE;
        }

        LOW_ASSERT(l_Result == VK_SUCCESS || l_Result == VK_SUBOPTIMAL_KHR,
                   "Failed to acquire swapchain image");

        vkResetFences(
            p_Context.vk.m_Device, 1,
            &p_Context.vk.m_InFlightFences[p_Context.currentFrameIndex]);

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = 0;
        l_BeginInfo.pInheritanceInfo = nullptr;

        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(p_Context);
        LOW_ASSERT(vkBeginCommandBuffer(l_CommandBuffer, &l_BeginInfo) ==
                       VK_SUCCESS,
                   "Failed to begin recording command buffer");

        return Backend::ContextState::SUCCESS;
      }

      void vk_frame_render(Backend::Context &p_Context)
      {
        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(p_Context);

        LOW_ASSERT(vkEndCommandBuffer(l_CommandBuffer) == VK_SUCCESS,
                   "Failed to stop recording the command buffer");

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore l_WaitSemaphores[] = {
            p_Context.vk
                .m_ImageAvailableSemaphores[p_Context.currentFrameIndex]};
        VkPipelineStageFlags l_WaitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        l_SubmitInfo.waitSemaphoreCount = 1;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers = &l_CommandBuffer;

        VkSemaphore l_SignalSemaphores[] = {
            p_Context.vk
                .m_RenderFinishedSemaphores[p_Context.currentFrameIndex]};
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

        VkResult l_SubmitResult = vkQueueSubmit(
            p_Context.vk.m_GraphicsQueue, 1, &l_SubmitInfo,
            p_Context.vk.m_InFlightFences[p_Context.currentFrameIndex]);

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

        VkResult l_Result =
            vkQueuePresentKHR(p_Context.vk.m_PresentQueue, &l_PresentInfo);

        // Handle window resize
        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR ||
            l_Result == VK_SUBOPTIMAL_KHR || /*g_FramebufferResized*/ false) {
          // g_FramebufferResized = false;
          // recreate_swapchain();
          // TODO: Handle reconfigure renderer
        } else {
          LOW_ASSERT(l_Result == VK_SUCCESS,
                     "Failed to present swapchain image");
        }

        p_Context.currentFrameIndex =
            (p_Context.currentFrameIndex + 1) % p_Context.framesInFlight;

        p_Context.vk.m_BoundPipelineLayout = VK_NULL_HANDLE;
      }

      void vk_renderpass_create(Backend::Renderpass &p_Renderpass,
                                Backend::RenderpassCreateParams &p_Params)
      {
        p_Renderpass.context = p_Params.context;
        p_Renderpass.dimensions = p_Params.dimensions;
        p_Renderpass.clearDepthColor = p_Params.clearDepthColor;
        p_Renderpass.renderTargetCount = p_Params.renderTargetCount;
        p_Renderpass.useDepth = p_Params.useDepth;
        p_Renderpass.renderTargets =
            (Backend::ImageResource *)Util::Memory::main_allocator()->allocate(
                p_Params.renderTargetCount * sizeof(Backend::ImageResource));

        p_Renderpass.clearTargetColor =
            (Math::Color *)Util::Memory::main_allocator()->allocate(
                p_Params.renderTargetCount * sizeof(Math::Color));

        Low::Util::List<VkAttachmentDescription> l_Attachments;
        Low::Util::List<VkAttachmentReference> l_ColorAttachmentRefs;

        VkAttachmentDescription l_DepthAttachment{};
        VkAttachmentReference l_DepthAttachmentRef{};

        VkSubpassDescription l_Subpass{};
        l_Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        for (uint32_t i = 0u; i < p_Params.renderTargetCount; ++i) {
          p_Renderpass.clearTargetColor[i] = p_Params.clearTargetColor[i];
          p_Renderpass.renderTargets[i] = p_Params.renderTargets[i];

          VkAttachmentDescription l_ColorAttachment{};
          l_ColorAttachment.format = Helper::imageformat_to_vkformat(
              p_Renderpass.renderTargets[i].format);
          l_ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
          l_ColorAttachment.loadOp = p_Params.clearTargetColor[i].a > 0.0f
                                         ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                         : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          l_ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          l_ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
          l_ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          l_ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

          l_Attachments.push_back(l_ColorAttachment);

          VkAttachmentReference l_ColorAttachmentRef{};
          l_ColorAttachmentRef.attachment = i;
          l_ColorAttachmentRef.layout =
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

          l_ColorAttachmentRefs.push_back(l_ColorAttachmentRef);
        }

        if (p_Params.useDepth) {
          l_DepthAttachment.format =
              Helper::vkformat_get_depth(*p_Params.context);
          l_DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
          l_DepthAttachment.loadOp = p_Params.clearDepthColor.g > 0.0f
                                         ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                         : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          l_DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          l_DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
          l_DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          l_DepthAttachment.finalLayout =
              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

          l_DepthAttachmentRef.attachment = p_Params.renderTargetCount;
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
          l_Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }

        VkRenderPassCreateInfo l_RenderpassInfo{};
        l_RenderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        l_RenderpassInfo.attachmentCount =
            static_cast<uint32_t>(l_Attachments.size());
        l_RenderpassInfo.pAttachments = l_Attachments.data();
        l_RenderpassInfo.subpassCount = 1;
        l_RenderpassInfo.pSubpasses = &l_Subpass;
        l_RenderpassInfo.dependencyCount = 1;
        l_RenderpassInfo.pDependencies = &l_Dependency;

        LOW_ASSERT(vkCreateRenderPass(
                       p_Params.context->vk.m_Device, &l_RenderpassInfo,
                       nullptr, &(p_Renderpass.vk.m_Renderpass)) == VK_SUCCESS,
                   "Failed to create render pass");

        {
          Util::List<VkImageView> l_Attachments;
          l_Attachments.resize(p_Params.renderTargetCount);

          for (int i_Iter = 0; i_Iter < p_Params.renderTargetCount; i_Iter++) {
            l_Attachments[i_Iter] =
                p_Params.renderTargets[i_Iter].vk.m_ImageView;
          }

          VkFramebufferCreateInfo l_FramebufferInfo{};
          l_FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
          l_FramebufferInfo.renderPass = p_Renderpass.vk.m_Renderpass;
          l_FramebufferInfo.attachmentCount =
              static_cast<uint32_t>(l_Attachments.size());
          l_FramebufferInfo.pAttachments = l_Attachments.data();
          l_FramebufferInfo.width = p_Params.dimensions.x;
          l_FramebufferInfo.height = p_Params.dimensions.y;
          l_FramebufferInfo.layers = 1;

          LOW_ASSERT(vkCreateFramebuffer(p_Params.context->vk.m_Device,
                                         &l_FramebufferInfo, nullptr,
                                         &(p_Renderpass.vk.m_Framebuffer)) ==
                         VK_SUCCESS,
                     "Failed to create framebuffer");
        }
      }

      void vk_renderpass_cleanup(Backend::Renderpass &p_Renderpass)
      {
        vkDestroyFramebuffer(p_Renderpass.context->vk.m_Device,
                             p_Renderpass.vk.m_Framebuffer, nullptr);

        vkDestroyRenderPass(p_Renderpass.context->vk.m_Device,
                            p_Renderpass.vk.m_Renderpass, nullptr);

        Util::Memory::main_allocator()->deallocate(
            p_Renderpass.clearTargetColor);
        Util::Memory::main_allocator()->deallocate(p_Renderpass.renderTargets);
      }

      void vk_renderpass_begin(Backend::Renderpass &p_Renderpass)
      {
        VkRenderPassBeginInfo l_RenderpassInfo{};
        l_RenderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        l_RenderpassInfo.renderPass = p_Renderpass.vk.m_Renderpass;
        l_RenderpassInfo.framebuffer = p_Renderpass.vk.m_Framebuffer;
        l_RenderpassInfo.renderArea.offset = {0, 0};

        VkExtent2D l_ActualExtent = {
            static_cast<uint32_t>(p_Renderpass.dimensions.x),
            static_cast<uint32_t>(p_Renderpass.dimensions.y)};

        l_RenderpassInfo.renderArea.extent = l_ActualExtent;

        Low::Util::List<VkClearValue> l_ClearValues;

        for (uint32_t i = 0u; i < p_Renderpass.renderTargetCount; ++i) {
          VkClearValue l_ClearColor = {{{p_Renderpass.clearTargetColor[i].r,
                                         p_Renderpass.clearTargetColor[i].g,
                                         p_Renderpass.clearTargetColor[i].b,
                                         p_Renderpass.clearTargetColor[i].a}}};
          l_ClearValues.push_back(l_ClearColor);
        }
        if (p_Renderpass.clearDepthColor.y > 0.0f) {
          l_ClearValues.push_back(
              {p_Renderpass.clearDepthColor.r, p_Renderpass.clearDepthColor.y});
        }

        l_RenderpassInfo.clearValueCount =
            static_cast<uint32_t>(l_ClearValues.size());
        l_RenderpassInfo.pClearValues = l_ClearValues.data();

        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(*p_Renderpass.context);

        vkCmdBeginRenderPass(l_CommandBuffer, &l_RenderpassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        VkViewport l_Viewport{};
        l_Viewport.x = 0.f;
        l_Viewport.y = 0.f;
        l_Viewport.width = static_cast<float>(l_ActualExtent.width);
        l_Viewport.height = static_cast<float>(l_ActualExtent.height);
        l_Viewport.minDepth = 0.f;
        l_Viewport.maxDepth = 1.f;
        vkCmdSetViewport(l_CommandBuffer, 0, 1, &l_Viewport);

        VkRect2D l_Scissor{};
        l_Scissor.offset = {0, 0};
        l_Scissor.extent = l_ActualExtent;
        vkCmdSetScissor(l_CommandBuffer, 0, 1, &l_Scissor);
      }

      void vk_renderpass_end(Backend::Renderpass &p_Renderpass)
      {
        VkCommandBuffer l_CommandBuffer =
            ContextHelper::get_current_commandbuffer(*p_Renderpass.context);
        vkCmdEndRenderPass(l_CommandBuffer);
      }

      namespace ImageHelper {
        static void create_image_view(Backend::Context &p_Context,
                                      Backend::ImageResource &p_Image2d,
                                      uint8_t &p_Format, bool p_Depth)
        {
          VkImageViewCreateInfo l_CreateInfo{};
          l_CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
          l_CreateInfo.image = p_Image2d.vk.m_Image;
          l_CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
          l_CreateInfo.format = Helper::imageformat_to_vkformat(p_Format);

          l_CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
          l_CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
          l_CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
          l_CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

          l_CreateInfo.subresourceRange.aspectMask =
              p_Depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
          l_CreateInfo.subresourceRange.baseMipLevel = 0;
          l_CreateInfo.subresourceRange.levelCount = 1;
          l_CreateInfo.subresourceRange.baseArrayLayer = 0;
          l_CreateInfo.subresourceRange.layerCount = 1;

          LOW_ASSERT(vkCreateImageView(p_Context.vk.m_Device, &l_CreateInfo,
                                       nullptr, &(p_Image2d.vk.m_ImageView)) ==
                         VK_SUCCESS,
                     "Could not create image view");

          LOW_LOG_DEBUG("Image view created");
        }

        static void create_2d_sampler(Backend::Context &p_Context,
                                      Backend::ImageResource &p_Image)
        {
          VkSamplerCreateInfo l_SamplerInfo{};
          l_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
          l_SamplerInfo.magFilter = VK_FILTER_LINEAR;
          l_SamplerInfo.minFilter = VK_FILTER_LINEAR;
          l_SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
          l_SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
          l_SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
          l_SamplerInfo.anisotropyEnable = VK_TRUE;

          VkPhysicalDeviceProperties l_Properties{};
          vkGetPhysicalDeviceProperties(p_Context.vk.m_PhysicalDevice,
                                        &l_Properties);

          l_SamplerInfo.maxAnisotropy =
              l_Properties.limits.maxSamplerAnisotropy;
          l_SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
          l_SamplerInfo.unnormalizedCoordinates = VK_FALSE;
          l_SamplerInfo.compareEnable = VK_FALSE;
          l_SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
          l_SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
          l_SamplerInfo.mipLodBias = 0.f;
          l_SamplerInfo.minLod = 0.f;
          l_SamplerInfo.maxLod = 0.f;

          LOW_ASSERT(vkCreateSampler(p_Context.vk.m_Device, &l_SamplerInfo,
                                     nullptr,
                                     &(p_Image.vk.m_Sampler)) == VK_SUCCESS,
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

          l_Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          l_Region.imageSubresource.mipLevel = 0;
          l_Region.imageSubresource.baseArrayLayer = 0;
          l_Region.imageSubresource.layerCount = 1;

          l_Region.imageOffset = {0, 0, 0};
          l_Region.imageExtent = {p_Width, p_Height, 1};

          vkCmdCopyBufferToImage(l_CommandBuffer, p_Buffer, p_Image.vk.m_Image,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                 &l_Region);

          Helper::end_single_time_commands(*p_Image.context, l_CommandBuffer);
        }

        static void create_image(Backend::ImageResource &p_Image,
                                 uint32_t p_Width, uint32_t p_Height,
                                 VkFormat p_Format, VkImageTiling p_Tiling,
                                 VkImageUsageFlags p_Usage,
                                 VkMemoryPropertyFlags p_Properties,
                                 void *p_ImageData, size_t p_ImageDataSize)
        {
          VkBuffer l_StagingBuffer;
          VkDeviceMemory l_StagingBufferMemory;

          if (p_ImageData) {
            Helper::create_buffer(*p_Image.context, p_ImageDataSize,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  l_StagingBuffer, l_StagingBufferMemory);

            void *l_Data;
            vkMapMemory(p_Image.context->vk.m_Device, l_StagingBufferMemory, 0,
                        p_ImageDataSize, 0, &l_Data);
            memcpy(l_Data, p_ImageData, p_ImageDataSize);
            vkUnmapMemory(p_Image.context->vk.m_Device, l_StagingBufferMemory);
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

          LOW_ASSERT(vkCreateImage(p_Image.context->vk.m_Device, &l_ImageInfo,
                                   nullptr,
                                   &(p_Image.vk.m_Image)) == VK_SUCCESS,
                     "Failed to create image");

          VkMemoryRequirements l_MemRequirements;
          vkGetImageMemoryRequirements(p_Image.context->vk.m_Device,
                                       p_Image.vk.m_Image, &l_MemRequirements);

          VkMemoryAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
          l_AllocInfo.allocationSize = l_MemRequirements.size;
          l_AllocInfo.memoryTypeIndex = Helper::find_memory_type(
              p_Image.context->vk, l_MemRequirements.memoryTypeBits,
              p_Properties);

          LOW_ASSERT(vkAllocateMemory(p_Image.context->vk.m_Device,
                                      &l_AllocInfo, nullptr,
                                      &(p_Image.vk.m_Memory)) == VK_SUCCESS,
                     "Failed to allocate image memory");

          vkBindImageMemory(p_Image.context->vk.m_Device, p_Image.vk.m_Image,
                            p_Image.vk.m_Memory, 0);
        }
      } // namespace ImageHelper

      void vk_imageresource_create(Backend::ImageResource &p_Image,
                                   Backend::ImageResourceCreateParams &p_Params)
      {
        p_Image.context = p_Params.context;
        p_Image.format = p_Params.format;
        p_Image.dimensions = p_Params.dimensions;
        p_Image.vk.m_State = ImageState::UNDEFINED;

        VkImageUsageFlags l_UsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (p_Params.writable) {
          l_UsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if (p_Params.createImage) {
          ImageHelper::create_image(
              p_Image, p_Params.dimensions.x, p_Params.dimensions.y,
              Helper::imageformat_to_vkformat(p_Params.format),
              VK_IMAGE_TILING_OPTIMAL, l_UsageFlags,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_Params.imageData,
              p_Params.imageDataSize);
        }

        ImageHelper::create_image_view(*p_Params.context, p_Image,
                                       p_Image.format, p_Params.depth);

        ImageHelper::create_2d_sampler(*p_Params.context, p_Image);

        p_Image.swapchainImage = false;
      }

      void vk_imageresource_cleanup(Backend::ImageResource &p_Image)
      {
        Context &l_Context = p_Image.context->vk;
        ImageResource &l_Image = p_Image.vk;

        vkDestroySampler(l_Context.m_Device, l_Image.m_Sampler, nullptr);
        vkDestroyImageView(l_Context.m_Device, l_Image.m_ImageView, nullptr);

        if (!p_Image.swapchainImage) {
          // TL TODO: Switch to vma
          vkDestroyImage(l_Context.m_Device, l_Image.m_Image, nullptr);
          vkFreeMemory(l_Context.m_Device, l_Image.m_Memory, nullptr);
        }
      }

      void vk_pipeline_resource_signature_create(
          Backend::PipelineResourceSignature &p_Signature,
          Backend::PipelineResourceSignatureCreateParams &p_Params)
      {
        LOW_ASSERT(p_Params.binding < LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS,
                   "Signature binding index too high");
        p_Signature.binding = p_Params.binding;
        p_Signature.vk.m_Index =
            p_Params.context->vk.m_PipelineResourceSignatureIndex++;
        p_Signature.context = p_Params.context;
        p_Signature.resourceCount = p_Params.resourceDescriptionCount;
        p_Signature.resources =
            (Backend::PipelineResourceBinding *)Util::Memory::main_allocator()
                ->allocate(p_Signature.resourceCount *
                           sizeof(Backend::PipelineResourceBinding));

        {
          Util::List<VkDescriptorSetLayoutBinding> l_Bindings;

          for (uint32_t i = 0u; i < p_Params.resourceDescriptionCount; ++i) {
            Backend::PipelineResourceDescription &i_Resource =
                p_Params.resourceDescriptions[i];

            p_Signature.resources[i].description = i_Resource;
            p_Signature.resources[i].boundResourceHandleId = ~0ull;

            VkDescriptorSetLayoutBinding i_Binding;
            i_Binding.binding = i;

            i_Binding.descriptorCount = i_Resource.arraySize;

            if (i_Resource.step == Backend::ResourcePipelineStep::COMPUTE) {
              i_Binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            } else if (i_Resource.step ==
                       Backend::ResourcePipelineStep::VERTEX) {
              i_Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            } else if (i_Resource.step ==
                       Backend::ResourcePipelineStep::FRAGMENT) {
              i_Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            } else if (i_Resource.step ==
                       Backend::ResourcePipelineStep::GRAPHICS) {
              i_Binding.stageFlags =
                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            } else {
              LOW_ASSERT(false, "Unknown pipeline step");
            }

            if (i_Resource.type == Backend::ResourceType::CONSTANT_BUFFER) {
              i_Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            } else if (i_Resource.type == Backend::ResourceType::BUFFER) {
              i_Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            } else if (i_Resource.type == Backend::ResourceType::IMAGE) {
              i_Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            } else if (i_Resource.type == Backend::ResourceType::SAMPLER) {
              i_Binding.descriptorType =
                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            } else {
              LOW_ASSERT(false, "Unknown uniform interface type");
            }

            i_Binding.pImmutableSamplers = nullptr;

            l_Bindings.push_back(i_Binding);
          }

          VkDescriptorSetLayoutCreateInfo l_LayoutInfo{};
          l_LayoutInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
          l_LayoutInfo.bindingCount = static_cast<uint32_t>(l_Bindings.size());
          l_LayoutInfo.pBindings = l_Bindings.data();

          LOW_ASSERT(
              vkCreateDescriptorSetLayout(
                  p_Params.context->vk.m_Device, &l_LayoutInfo, nullptr,
                  &(p_Signature.context->vk
                        .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
                        .m_DescriptorSetLayout)) == VK_SUCCESS,
              "Failed to create descriptor set layout");
        }

        {
          PipelineResourceSignatureInternal &l_InternalSignature =
              p_Signature.context->vk
                  .m_PipelineResourceSignatures[p_Signature.vk.m_Index];

          l_InternalSignature.m_BindingCount =
              p_Params.resourceDescriptionCount;
          l_InternalSignature.m_Bindings =
              (Backend::PipelineResourceBinding *)Util::Memory::main_allocator()
                  ->allocate(sizeof(Backend::PipelineResourceBinding) *
                             p_Params.resourceDescriptionCount);

          for (uint32_t i = 0u; i < p_Params.resourceDescriptionCount; ++i) {
            l_InternalSignature.m_Bindings[i].boundResourceHandleId = ~0ull;
            l_InternalSignature.m_Bindings[i].description =
                p_Params.resourceDescriptions[i];
          }

          l_InternalSignature.m_DescriptorSets =
              (VkDescriptorSet *)Util::Memory::main_allocator()->allocate(
                  sizeof(VkDescriptorSet) *
                  p_Signature.context->framesInFlight);

          Util::List<VkDescriptorSetLayout> l_Layouts(
              p_Params.context->framesInFlight,
              l_InternalSignature.m_DescriptorSetLayout);

          VkDescriptorSetAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          l_AllocInfo.descriptorPool = p_Params.context->vk.m_DescriptorPool;
          l_AllocInfo.descriptorSetCount =
              static_cast<uint32_t>(p_Params.context->framesInFlight);
          l_AllocInfo.pSetLayouts = l_Layouts.data();

          LOW_ASSERT(vkAllocateDescriptorSets(
                         p_Params.context->vk.m_Device, &l_AllocInfo,
                         l_InternalSignature.m_DescriptorSets) == VK_SUCCESS,
                     "Failed to allocate descriptor sets");
        }
      }

      void vk_pipeline_resource_signature_commit(
          Backend::PipelineResourceSignature &p_Signature)
      {
        p_Signature.context->vk
            .m_CommittedPipelineResourceSignatures[p_Signature.binding] =
            p_Signature.vk.m_Index;
        for (uint32_t i = p_Signature.binding + 1;
             i < LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS; ++i) {
          p_Signature.context->vk.m_CommittedPipelineResourceSignatures[i] =
              ~0u;
        }
      }

      void vk_pipeline_resource_signature_set_image(
          Backend::PipelineResourceSignature &p_Signature, Util::Name p_Name,
          uint32_t p_ArrayIndex, Resource::Image p_ImageResource)
      {
        bool l_Found = false;
        for (uint32_t i = 0u; i < p_Signature.resourceCount; ++i) {
          if (p_Signature.resources[i].description.name == p_Name) {
            Backend::PipelineResourceBinding &i_Resource =
                p_Signature.resources[i];

            LOW_ASSERT(i_Resource.description.type ==
                           Backend::ResourceType::IMAGE,
                       "Expected image resource type");
            LOW_ASSERT(p_ArrayIndex < i_Resource.description.arraySize,
                       "Resource array index out of bounds");

            l_Found = true;
            i_Resource.boundResourceHandleId = p_ImageResource.get_id();
            p_Signature.context->vk
                .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
                .m_Bindings[i]
                .boundResourceHandleId = p_ImageResource.get_id();

            for (uint8_t j = 0u; j < p_Signature.context->framesInFlight; ++j) {
              VkDescriptorImageInfo i_ImageInfo;
              i_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
              i_ImageInfo.imageView =
                  p_ImageResource.get_image().vk.m_ImageView;
              i_ImageInfo.sampler = p_ImageResource.get_image().vk.m_Sampler;

              VkWriteDescriptorSet i_DescriptorWrite;

              i_DescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
              i_DescriptorWrite.dstSet =
                  p_Signature.context->vk
                      .m_PipelineResourceSignatures[p_Signature.vk.m_Index]
                      .m_DescriptorSets[j];
              i_DescriptorWrite.dstBinding = i;
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

            break;
          }
        }
        LOW_ASSERT(l_Found, "Unable to find resource binding");
      }

      namespace PipelineHelper {
        VkShaderModule create_shader_module(Backend::Context &p_Context,
                                            const Util::List<char> &p_Code)
        {
          VkShaderModuleCreateInfo l_CreateInfo{};
          l_CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
          l_CreateInfo.codeSize = p_Code.size() - 1; // Remove \0 terminator
          l_CreateInfo.pCode =
              reinterpret_cast<const uint32_t *>(p_Code.data());

          VkShaderModule l_ShaderModule;
          LOW_ASSERT(vkCreateShaderModule(p_Context.vk.m_Device, &l_CreateInfo,
                                          nullptr,
                                          &l_ShaderModule) == VK_SUCCESS,
                     "Failed to create shader module");

          return l_ShaderModule;
        }

        Util::List<char> read_shader_file(const char *p_Filename)
        {
          Util::FileIO::File l_File = Util::FileIO::open(
              p_Filename, Util::FileIO::FileMode::READ_BYTES);

          size_t l_Filesize = Util::FileIO::size_sync(l_File);
          Util::List<char> l_ContentBuffer(l_Filesize +
                                           1); // Add 1 because of \0 terminator

          Util::FileIO::read_sync(l_File, l_ContentBuffer.data());

          return l_ContentBuffer;
        }
      } // namespace PipelineHelper

      void
      vk_pipeline_compute_create(Backend::Pipeline &p_Pipeline,
                                 Backend::PipelineComputeCreateParams &p_Params)
      {
        p_Pipeline.context = p_Params.context;

        {
          Util::List<VkDescriptorSetLayout> l_DescriptorSetLayouts;
          l_DescriptorSetLayouts.resize(p_Params.signatureCount);
          for (uint32_t i = 0u; i < p_Params.signatureCount; ++i) {
            l_DescriptorSetLayouts[i] =
                p_Params.context->vk
                    .m_PipelineResourceSignatures[p_Params.signatures[i]
                                                      .vk.m_Index]
                    .m_DescriptorSetLayout;
          }

          VkPipelineLayoutCreateInfo l_PipelineLayoutInfo{};
          l_PipelineLayoutInfo.sType =
              VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
          if (p_Params.signatureCount == 0) {
            l_PipelineLayoutInfo.setLayoutCount = 0;
            l_PipelineLayoutInfo.pSetLayouts = nullptr;
          } else {
            l_PipelineLayoutInfo.setLayoutCount = p_Params.signatureCount;
            l_PipelineLayoutInfo.pSetLayouts = l_DescriptorSetLayouts.data();
          }
          l_PipelineLayoutInfo.pushConstantRangeCount = 0;
          l_PipelineLayoutInfo.pPushConstantRanges = nullptr;

          LOW_ASSERT(vkCreatePipelineLayout(
                         p_Params.context->vk.m_Device, &l_PipelineLayoutInfo,
                         nullptr,
                         &(p_Pipeline.vk.m_PipelineLayout)) == VK_SUCCESS,
                     "Failed to create pipeline layout");
        }

        {
          p_Pipeline.type = Backend::PipelineType::COMPUTE;

          VkShaderModule l_ShaderModule = PipelineHelper::create_shader_module(
              *p_Params.context,
              PipelineHelper::read_shader_file(p_Params.shaderPath));

          VkComputePipelineCreateInfo l_ComputePipelineInfo = {
              VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
          l_ComputePipelineInfo.stage.sType =
              VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
          l_ComputePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
          l_ComputePipelineInfo.stage.module = l_ShaderModule;
          l_ComputePipelineInfo.stage.pName = "main";
          l_ComputePipelineInfo.layout = p_Pipeline.vk.m_PipelineLayout;

          LOW_ASSERT(vkCreateComputePipelines(
                         p_Params.context->vk.m_Device, VK_NULL_HANDLE, 1,
                         &l_ComputePipelineInfo, nullptr,
                         &(p_Pipeline.vk.m_Pipeline)) == VK_SUCCESS,
                     "Failed to create compute pipeline");

          vkDestroyShaderModule(p_Params.context->vk.m_Device, l_ShaderModule,
                                nullptr);
        }
      }

      void vk_pipeline_cleanup(Backend::Pipeline &p_Pipeline)
      {
        vkDestroyPipeline(p_Pipeline.context->vk.m_Device,
                          p_Pipeline.vk.m_Pipeline, nullptr);
        vkDestroyPipelineLayout(p_Pipeline.context->vk.m_Device,
                                p_Pipeline.vk.m_PipelineLayout, nullptr);
      }

      void vk_pipeline_bind(Backend::Pipeline &p_Pipeline)
      {
        if (p_Pipeline.type == Backend::PipelineType::GRAPHICS) {
          p_Pipeline.context->vk.m_BoundPipelineLayout =
              p_Pipeline.vk.m_PipelineLayout;
          vkCmdBindPipeline(
              ContextHelper::get_current_commandbuffer(*p_Pipeline.context),
              VK_PIPELINE_BIND_POINT_GRAPHICS, p_Pipeline.vk.m_Pipeline);
        } else if (p_Pipeline.type == Backend::PipelineType::COMPUTE) {
          p_Pipeline.context->vk.m_BoundPipelineLayout =
              p_Pipeline.vk.m_PipelineLayout;
          vkCmdBindPipeline(
              ContextHelper::get_current_commandbuffer(*p_Pipeline.context),
              VK_PIPELINE_BIND_POINT_COMPUTE, p_Pipeline.vk.m_Pipeline);
        } else {
          LOW_ASSERT(false, "Unknown pipeline type");
        }
      }

      namespace BarrierHelpers {
        void transition_image_barrier(
            VkCommandBuffer p_CommandBuffer, Backend::ImageResource &p_Image2D,
            VkImageLayout p_OldLayout, VkImageLayout p_NewLayout,
            VkAccessFlags p_SrcAccessMask, VkAccessFlags p_DstAccessMask,
            VkPipelineStageFlags p_SourceStage, VkPipelineStageFlags p_DstStage)
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
                VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
          } else {
            l_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
                               l_DestinationStage, 0, 0, nullptr, 0, nullptr, 1,
                               &l_Barrier);
        }

        static void perform_resource_barrier_image(
            Backend::Context &p_Context,
            Backend::PipelineResourceBinding &p_Binding)
        {
          Util::Handle l_Handle(p_Binding.boundResourceHandleId);
          LOW_ASSERT(l_Handle.get_type() == Resource::Image::TYPE_ID,
                     "Unexpected resource type");
          Resource::Image l_Image(l_Handle.get_id());

          VkCommandBuffer l_CommandBuffer =
              ContextHelper::get_current_commandbuffer(p_Context);

          if (l_Image.get_image().vk.m_State == ImageState::GENERAL) {
            return;
          } else if (l_Image.get_image().vk.m_State == ImageState::UNDEFINED) {
            transition_image_barrier(
                l_CommandBuffer, l_Image.get_image(), VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_GENERAL, 0, VK_ACCESS_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            l_Image.get_image().vk.m_State = ImageState::GENERAL;
          } else {
            LOW_ASSERT_WARN(false, "Unsupported image state for transition");
          }
        }
      } // namespace BarrierHelpers

      static void vk_perform_barriers(Backend::Context &p_Context)
      {
        for (uint32_t i = 0u; i < LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS; ++i) {
          uint32_t i_Index =
              p_Context.vk.m_CommittedPipelineResourceSignatures[i];
          if (i_Index > MAX_PRS) {
            break;
          }

          PipelineResourceSignatureInternal &l_InternalSignature =
              p_Context.vk.m_PipelineResourceSignatures[i_Index];

          for (uint32_t j = 0u; j < l_InternalSignature.m_BindingCount; ++j) {
            Backend::PipelineResourceBinding &i_Binding =
                l_InternalSignature.m_Bindings[j];
            if (i_Binding.description.type == Backend::ResourceType::IMAGE) {
              BarrierHelpers::perform_resource_barrier_image(p_Context,
                                                             i_Binding);
            }
          }
        }
      }

      static void vk_bind_descriptor_sets(Backend::Context &p_Context,
                                          VkPipelineBindPoint p_BindPoint)
      {
        Util::List<VkDescriptorSet> l_DescriptorSets;

        for (uint32_t i = 0u; i < LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS; ++i) {
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

        vkCmdBindDescriptorSets(
            ContextHelper::get_current_commandbuffer(p_Context), p_BindPoint,
            p_Context.vk.m_BoundPipelineLayout, 0, l_DescriptorSets.size(),
            l_DescriptorSets.data(), 0, nullptr);
      }

      void vk_bind_resources(Backend::Context &p_Context,
                             VkPipelineBindPoint p_BindPoint)
      {
        vk_perform_barriers(p_Context);
        vk_bind_descriptor_sets(p_Context, VK_PIPELINE_BIND_POINT_COMPUTE);
      }

      void vk_compute_dispatch(Backend::Context &p_Context,
                               Math::UVector3 p_Dimensions)
      {
        vk_bind_resources(p_Context, VK_PIPELINE_BIND_POINT_COMPUTE);
        vkCmdDispatch(ContextHelper::get_current_commandbuffer(p_Context),
                      p_Dimensions.x, p_Dimensions.y, p_Dimensions.z);
      }

      void initialize_callback(Backend::ApiBackendCallback &p_Callbacks)
      {
        p_Callbacks.context_create = &vk_context_create;
        p_Callbacks.context_cleanup = &vk_context_cleanup;
        p_Callbacks.context_wait_idle = &vk_context_wait_idle;

        p_Callbacks.frame_prepare = &vk_frame_prepare;
        p_Callbacks.frame_render = &vk_frame_render;

        p_Callbacks.renderpass_create = &vk_renderpass_create;
        p_Callbacks.renderpass_begin = &vk_renderpass_begin;
        p_Callbacks.renderpass_end = &vk_renderpass_end;

        p_Callbacks.pipeline_resource_signature_create =
            &vk_pipeline_resource_signature_create;
        p_Callbacks.pipeline_resource_signature_set_image =
            &vk_pipeline_resource_signature_set_image;
        p_Callbacks.pipeline_resource_signature_commit =
            &vk_pipeline_resource_signature_commit;

        p_Callbacks.pipeline_compute_create = &vk_pipeline_compute_create;
        p_Callbacks.pipeline_cleanup = &vk_pipeline_cleanup;
        p_Callbacks.pipeline_bind = &vk_pipeline_bind;

        p_Callbacks.compute_dispatch = &vk_compute_dispatch;

        p_Callbacks.imageresource_create = &vk_imageresource_create;
        p_Callbacks.imageresource_cleanup = &vk_imageresource_cleanup;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low

#undef SKIP_DEBUG_LEVEL
