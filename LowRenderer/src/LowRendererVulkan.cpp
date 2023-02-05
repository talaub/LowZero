#include "LowRendererVulkan.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilFileIO.h"

#include "LowMath.h"

#include "LowRendererBackend.h"

#include "vulkan/vulkan_core.h"

#include <GLFW/glfw3.h>

#include <corecrt_malloc.h>
#include <stdint.h>
#include <string>

#define SKIP_DEBUG_LEVEL true

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace Utils {
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

        static Utils::QueueFamilyIndices
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
      } // namespace Utils

      namespace ContextUtils {
        const Util::List<const char *> g_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"};

        const Util::List<const char *> g_DeviceExtensions = {
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
          l_CreateInfo.enabledExtensionCount = l_Extensions.size();
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

        static void create_surface(Backend::Context &p_Context)
        {
          // TL TODO: glfw hard coded
          LOW_ASSERT(glfwCreateWindowSurface(
                         p_Context.vk.m_Instance, p_Context.m_Window.m_Glfw,
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

          Utils::QueueFamilyIndices l_Indices =
              Utils::find_queue_families(p_Context, p_Device);
          if (!l_Indices.is_complete())
            return 0;

          if (!check_device_extension_support(p_Device))
            return 0;

          Utils::SwapChainSupportDetails l_SwapChainSupport =
              Utils::query_swap_chain_support(p_Context, p_Device);
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
          Utils::QueueFamilyIndices l_Indices =
              Utils::find_queue_families(p_Context, p_Context.m_PhysicalDevice);

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
      } // namespace ContextUtils

      void vk_context_create(Backend::Context &p_Context,
                             Backend::ContextCreateParams &p_Params)
      {
        // Check for validation layer support
        if (p_Params.validation_enabled) {
          LOW_ASSERT(ContextUtils::check_validation_layer_support(),
                     "Validation layers requested, but not available");
          LOW_LOG_DEBUG("Validation layers enabled");
        }

        p_Context.vk.m_ValidationEnabled = p_Params.validation_enabled;
        p_Context.m_Window = *(p_Params.window);

        ContextUtils::create_instance(p_Context.vk);

        ContextUtils::setup_debug_messenger(p_Context.vk);

        ContextUtils::create_surface(p_Context);

        ContextUtils::select_physical_device(p_Context.vk);

        ContextUtils::create_logical_device(p_Context.vk);

        LOW_LOG_DEBUG("Vulkan context initialized");
      }

      void vk_context_cleanup(Backend::Context &p_Context)
      {
        Context l_Context = p_Context.vk;

        vkDestroyDevice(l_Context.m_Device, nullptr);

        vkDestroySurfaceKHR(l_Context.m_Instance, l_Context.m_Surface, nullptr);

        if (l_Context.m_ValidationEnabled) {
          ContextUtils::destroy_debug_utils_messenger_ext(
              l_Context.m_Instance, l_Context.m_DebugMessenger, nullptr);
        }

        vkDestroyInstance(l_Context.m_Instance, nullptr);
      }

      void vk_context_wait_idle(Backend::Context &p_Context)
      {
        vkDeviceWaitIdle(p_Context.vk.m_Device);
      }

      void vk_framebuffer_create(Backend::Framebuffer &p_Framebuffer,
                                 Backend::FramebufferCreateParams &p_Params)
      {
        p_Framebuffer.context = p_Params.context;

        Util::List<VkImageView> l_Attachments;
        l_Attachments.resize(p_Params.renderTargetCount);

        for (int i_Iter = 0; i_Iter < p_Params.renderTargetCount; i_Iter++) {
          l_Attachments[i_Iter] = p_Params.renderTargets[i_Iter].vk.m_ImageView;
        }

        VkFramebufferCreateInfo l_FramebufferInfo{};
        l_FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        l_FramebufferInfo.renderPass = p_Params.renderpass->vk.m_Handle;
        l_FramebufferInfo.attachmentCount =
            static_cast<uint32_t>(l_Attachments.size());
        l_FramebufferInfo.pAttachments = l_Attachments.data();
        l_FramebufferInfo.width = p_Params.dimensions.x;
        l_FramebufferInfo.height = p_Params.dimensions.y;
        l_FramebufferInfo.layers = 1;

        LOW_ASSERT(vkCreateFramebuffer(
                       p_Params.context->vk.m_Device, &l_FramebufferInfo,
                       nullptr, &(p_Framebuffer.vk.m_Handle)) == VK_SUCCESS,
                   "Failed to create framebuffer");

        p_Framebuffer.vk.m_Dimensions.x = p_Params.dimensions.x;
        p_Framebuffer.vk.m_Dimensions.y = p_Params.dimensions.y;

        LOW_LOG_DEBUG("Framebuffer created");
      }

      void vk_framebuffer_get_dimensions(Backend::Framebuffer &p_Framebuffer,
                                         Math::UVector2 &p_Dimensions)
      {
        p_Dimensions.x = p_Framebuffer.vk.m_Dimensions.x;
        p_Dimensions.y = p_Framebuffer.vk.m_Dimensions.y;
      }

      void vk_framebuffer_cleanup(Backend::Framebuffer &p_Framebuffer)
      {
        vkDestroyFramebuffer(p_Framebuffer.context->vk.m_Device,
                             p_Framebuffer.vk.m_Handle, nullptr);
      }

      void vk_commandpool_create(Backend::CommandPool &p_CommandPool,
                                 Backend::CommandPoolCreateParams &p_Params)
      {
        p_CommandPool.context = p_Params.context;

        Utils::QueueFamilyIndices l_QueueFamilyIndices =
            Utils::find_queue_families(p_Params.context->vk,
                                       p_Params.context->vk.m_PhysicalDevice);

        VkCommandPoolCreateInfo l_PoolInfo{};
        l_PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        l_PoolInfo.queueFamilyIndex =
            l_QueueFamilyIndices.m_GraphicsFamily.value();

        LOW_ASSERT(vkCreateCommandPool(
                       p_Params.context->vk.m_Device, &l_PoolInfo, nullptr,
                       &(p_CommandPool.vk.m_Handle)) == VK_SUCCESS,
                   "Failed to create command pool");

        LOW_LOG_DEBUG("Command pool created");
      }

      void vk_commandpool_cleanup(Backend::CommandPool &p_CommandPool)
      {
        vkDestroyCommandPool(p_CommandPool.context->vk.m_Device,
                             p_CommandPool.vk.m_Handle, nullptr);
      }

      void vk_renderpass_create(Backend::Renderpass &p_Renderpass,
                                Backend::RenderpassCreateParams &p_Params)
      {
        p_Renderpass.context = p_Params.context;
        p_Renderpass.clearDepth = p_Params.clearDepth;
        p_Renderpass.formatCount = p_Params.formatCount;
        p_Renderpass.useDepth = p_Params.useDepth;
        p_Renderpass.clearTarget =
            (bool *)calloc(p_Params.formatCount, sizeof(bool));

        Low::Util::List<VkAttachmentDescription> l_Attachments;
        Low::Util::List<VkAttachmentReference> l_ColorAttachmentRefs;

        VkAttachmentDescription l_DepthAttachment{};
        VkAttachmentReference l_DepthAttachmentRef{};

        VkSubpassDescription l_Subpass{};
        l_Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        for (uint32_t i = 0u; i < p_Params.formatCount; ++i) {
          ImageFormat &i_ImageFormat = p_Params.formats[i].vk;

          p_Renderpass.clearTarget[i] = p_Params.clearTarget[i];

          VkAttachmentDescription l_ColorAttachment{};
          l_ColorAttachment.format = i_ImageFormat.m_Handle;
          l_ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
          l_ColorAttachment.loadOp = p_Params.clearTarget[i]
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
          Backend::ImageFormat l_DepthFormat;
          Backend::imageformat_get_depth(*p_Params.context, l_DepthFormat);
          l_DepthAttachment.format = l_DepthFormat.vk.m_Handle;
          l_DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
          l_DepthAttachment.loadOp = p_Params.clearDepth
                                         ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                         : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          l_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          l_DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
          l_DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
          l_DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
          l_DepthAttachment.finalLayout =
              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

          l_DepthAttachmentRef.attachment = p_Params.formatCount;
          l_DepthAttachmentRef.layout =
              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

          l_Attachments.push_back(l_DepthAttachment);

          l_Subpass.pDepthStencilAttachment = &l_DepthAttachmentRef;
        } else {
          l_Subpass.pDepthStencilAttachment = nullptr;
        }

        l_Subpass.colorAttachmentCount = l_ColorAttachmentRefs.size();
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
                       nullptr, &(p_Renderpass.vk.m_Handle)) == VK_SUCCESS,
                   "Failed to create render pass");

        LOW_LOG_DEBUG("Renderpass created");
      }

      void vk_renderpass_cleanup(Backend::Renderpass &p_Renderpass)
      {
        vkDestroyRenderPass(p_Renderpass.context->vk.m_Device,
                            p_Renderpass.vk.m_Handle, nullptr);

        free(p_Renderpass.clearTarget);
      }

      void vk_renderpass_start(Backend::Renderpass &p_Renderpass,
                               Backend::RenderpassStartParams &p_Params)
      {
        VkRenderPassBeginInfo l_RenderpassInfo{};
        l_RenderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        l_RenderpassInfo.renderPass = p_Renderpass.vk.m_Handle;
        l_RenderpassInfo.framebuffer = p_Params.framebuffer->vk.m_Handle;
        l_RenderpassInfo.renderArea.offset = {0, 0};

        Math::UVector2 l_Dimensions;
        Backend::framebuffer_get_dimensions(*p_Params.framebuffer,
                                            l_Dimensions);

        VkExtent2D l_ActualExtent = {static_cast<uint32_t>(l_Dimensions.x),
                                     static_cast<uint32_t>(l_Dimensions.y)};

        l_RenderpassInfo.renderArea.extent = l_ActualExtent;

        Low::Util::List<VkClearValue> l_ClearValues;

        for (uint32_t i = 0u; i < p_Renderpass.formatCount; ++i) {
          VkClearValue l_ClearColor = {
              {{p_Params.clearColorValues[i].r, p_Params.clearColorValues[i].g,
                p_Params.clearColorValues[i].b,
                p_Params.clearColorValues[i].a}}};
          l_ClearValues.push_back(l_ClearColor);
        }
        if (p_Renderpass.clearDepth) {
          l_ClearValues.push_back(
              {p_Params.clearDepthValue.r,
               p_Params.clearDepthValue
                   .y}); // TL TODO: Passed value currently ignored
        }

        l_RenderpassInfo.clearValueCount = l_ClearValues.size();
        l_RenderpassInfo.pClearValues = l_ClearValues.data();

        vkCmdBeginRenderPass(p_Params.commandBuffer->vk.m_Handle,
                             &l_RenderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport l_Viewport{};
        l_Viewport.x = 0.f;
        l_Viewport.y = 0.f;
        l_Viewport.width = static_cast<float>(l_ActualExtent.width);
        l_Viewport.height = static_cast<float>(l_ActualExtent.height);
        l_Viewport.minDepth = 0.f;
        l_Viewport.maxDepth = 1.f;
        vkCmdSetViewport(p_Params.commandBuffer->vk.m_Handle, 0, 1,
                         &l_Viewport);

        VkRect2D l_Scissor{};
        l_Scissor.offset = {0, 0};
        l_Scissor.extent = l_ActualExtent;
        vkCmdSetScissor(p_Params.commandBuffer->vk.m_Handle, 0, 1, &l_Scissor);
      }

      void vk_renderpass_stop(Backend::Renderpass &p_Renderpass,
                              Backend::RenderpassStopParams &p_Params)
      {
        vkCmdEndRenderPass(p_Params.commandBuffer->vk.m_Handle);
      }

      void vk_imageformat_get_depth(Backend::Context &p_Context,
                                    Backend::ImageFormat &p_Format)
      {
        p_Format.vk.m_Handle = Utils::find_supported_format(
            p_Context.vk,
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
             VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
      }

      namespace SwapchainUtils {
        static void create_render_targets(Backend::Swapchain &p_Swapchain,
                                          Low::Util::List<VkImage> &p_Images)
        {
          Swapchain &l_Swapchain = p_Swapchain.vk;

          l_Swapchain.m_RenderTargets = (Backend::Image2D *)calloc(
              p_Images.size(), sizeof(Backend::Image2D));

          Low::Math::UVector2 l_Dimensions = l_Swapchain.m_Dimensions;

          Backend::Image2DCreateParams l_Params;
          l_Params.context = p_Swapchain.context;
          l_Params.create_image = false;
          l_Params.depth = false;
          l_Params.writable = false;
          l_Params.dimensions.x = l_Dimensions.x;
          l_Params.dimensions.y = l_Dimensions.y;
          Backend::ImageFormat l_Format;
          l_Format.vk = p_Swapchain.vk.m_ImageFormat;
          l_Params.format = &l_Format;

          for (size_t i_Iter = 0; i_Iter < p_Images.size(); i_Iter++) {
            l_Swapchain.m_RenderTargets[i_Iter].vk.m_Image = p_Images[i_Iter];

            vk_image2d_create(l_Swapchain.m_RenderTargets[i_Iter], l_Params);
            l_Swapchain.m_RenderTargets[i_Iter].swapchainImage = true;
          }

          LOW_LOG_DEBUG("Swapchain render targets created");
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
          glfwGetFramebufferSize(p_Context.m_Window.m_Glfw, &l_Width,
                                 &l_Height);

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

        static void create_sync_objects(Context &p_Context,
                                        Swapchain &p_Swapchain)

        {
          p_Swapchain.m_ImageAvailableSemaphores = (VkSemaphore *)calloc(
              p_Swapchain.m_ImageCount, sizeof(VkSemaphore));
          p_Swapchain.m_RenderFinishedSemaphores = (VkSemaphore *)calloc(
              p_Swapchain.m_ImageCount, sizeof(VkSemaphore));
          p_Swapchain.m_InFlightFences =
              (VkFence *)calloc(p_Swapchain.m_ImageCount, sizeof(VkFence));

          VkSemaphoreCreateInfo l_SemaphoreInfo{};
          l_SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

          VkFenceCreateInfo l_FenceInfo{};
          l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
          l_FenceInfo.flags =
              VK_FENCE_CREATE_SIGNALED_BIT; // Set to resolved immidiately to
                                            // not get stuck on frame one

          for (size_t i_Iter = 0; i_Iter < p_Swapchain.m_ImageCount; i_Iter++) {
            LOW_ASSERT(vkCreateSemaphore(
                           p_Context.m_Device, &l_SemaphoreInfo, nullptr,
                           &(p_Swapchain.m_ImageAvailableSemaphores[i_Iter])) ==
                           VK_SUCCESS,
                       "Failed to create semaphore");
            LOW_ASSERT(vkCreateSemaphore(
                           p_Context.m_Device, &l_SemaphoreInfo, nullptr,
                           &(p_Swapchain.m_RenderFinishedSemaphores[i_Iter])) ==
                           VK_SUCCESS,
                       "Failed to create semaphore");
            LOW_ASSERT(vkCreateFence(p_Context.m_Device, &l_FenceInfo, nullptr,
                                     &(p_Swapchain.m_InFlightFences[i_Iter])) ==
                           VK_SUCCESS,
                       "Failed to create fence");
          }

          LOW_LOG_DEBUG("Swapchain sync objects created");
        }

        static void create_command_buffers(Backend::Context &p_Context,
                                           Backend::CommandPool &p_CommandPool,
                                           Swapchain &p_Swapchain)
        {
          p_Swapchain.m_CommandBuffers = (Backend::CommandBuffer *)calloc(
              p_Swapchain.m_FramesInFlight, sizeof(Backend::CommandBuffer));

          VkCommandBufferAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          l_AllocInfo.commandPool = p_CommandPool.vk.m_Handle;
          l_AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          l_AllocInfo.commandBufferCount = 1;

          for (uint8_t i = 0; i < p_Swapchain.m_FramesInFlight; ++i) {
            LOW_ASSERT(vkAllocateCommandBuffers(
                           p_Context.vk.m_Device, &l_AllocInfo,
                           &(p_Swapchain.m_CommandBuffers[i].vk.m_Handle)) ==
                           VK_SUCCESS,
                       "Failed to allocate command buffer");
          }

          LOW_LOG_DEBUG("Swapchain commandbuffers created");
        }

        static void create_framebuffers(Backend::Context &p_Context,
                                        Backend::Swapchain &p_Swapchain)
        {

          p_Swapchain.vk.m_Framebuffers = (Backend::Framebuffer *)calloc(
              p_Swapchain.vk.m_ImageCount, sizeof(Backend::Framebuffer));

          Low::Math::UVector2 l_Dimensions(p_Swapchain.vk.m_Dimensions.x,
                                           p_Swapchain.vk.m_Dimensions.y);

          for (size_t i_Iter = 0; i_Iter < p_Swapchain.vk.m_ImageCount;
               i_Iter++) {

            Backend::FramebufferCreateParams i_FramebufferParams;
            i_FramebufferParams.context = &p_Context;
            i_FramebufferParams.renderTargets =
                &(p_Swapchain.vk.m_RenderTargets[i_Iter]);
            i_FramebufferParams.renderpass = &p_Swapchain.renderpass;
            i_FramebufferParams.dimensions = l_Dimensions;
            i_FramebufferParams.renderTargetCount = 1;
            i_FramebufferParams.framesInFlight =
                p_Swapchain.vk.m_FramesInFlight;

            vk_framebuffer_create(p_Swapchain.vk.m_Framebuffers[i_Iter],
                                  i_FramebufferParams);
          }

          LOW_LOG_DEBUG("Swapchain framebuffers created");
        }
      } // namespace SwapchainUtils

      void vk_swapchain_create(Backend::Swapchain &p_Swapchain,
                               Backend::SwapchainCreateParams &p_Params)
      {
        p_Swapchain.context = p_Params.context;
        p_Swapchain.vk.m_FramesInFlight =
            2; // TL TODO: Turn that into a parameter

        Context &l_Context = p_Params.context->vk;
        Swapchain &l_Swapchain = p_Swapchain.vk;

        l_Swapchain.m_CurrentFrameIndex = 0;

        Utils::SwapChainSupportDetails l_SwapChainSupportDetails =
            Utils::query_swap_chain_support(l_Context,
                                            l_Context.m_PhysicalDevice);

        VkSurfaceFormatKHR l_SurfaceFormat = Utils::choose_swap_surface_format(
            l_SwapChainSupportDetails.m_Formats);
        VkPresentModeKHR l_PresentMode = Utils::choose_swap_present_mode(
            l_SwapChainSupportDetails.m_PresentModes);
        VkExtent2D l_Extent = SwapchainUtils::choose_swap_extent(
            *p_Params.context, l_SwapChainSupportDetails.m_Capabilities);

        uint32_t l_ImageCount =
            l_SwapChainSupportDetails.m_Capabilities.minImageCount + 1;

        l_Swapchain.m_ImageCount = l_ImageCount;

        // Clamp the imagecount to not exceed the maximum
        // A set maximum of 0 means that there is no maximum
        if (l_SwapChainSupportDetails.m_Capabilities.maxImageCount > 0 &&
            l_ImageCount >
                l_SwapChainSupportDetails.m_Capabilities.maxImageCount) {
          l_ImageCount = l_SwapChainSupportDetails.m_Capabilities.maxImageCount;
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

        Utils::QueueFamilyIndices l_Indices =
            Utils::find_queue_families(l_Context, l_Context.m_PhysicalDevice);
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
                                        nullptr,
                                        &(l_Swapchain.m_Handle)) == VK_SUCCESS,
                   "Could not create swap chain");
        LOW_LOG_DEBUG("Swapchain created");

        // Retrieve the vkimages of the swapchain
        vkGetSwapchainImagesKHR(l_Context.m_Device, l_Swapchain.m_Handle,
                                &l_ImageCount, nullptr);

        Low::Util::List<VkImage> l_Images;
        l_Images.resize(l_ImageCount);
        vkGetSwapchainImagesKHR(l_Context.m_Device, l_Swapchain.m_Handle,
                                &l_ImageCount, l_Images.data());
        LOW_LOG_DEBUG("Retrieved swapchain images");

        // Store some swapchain info
        l_Swapchain.m_ImageFormat.m_Handle = l_SurfaceFormat.format;
        l_Swapchain.m_Dimensions =
            Math::UVector2(l_Extent.width, l_Extent.height);

        Backend::RenderpassCreateParams l_RenderPassParams;
        l_RenderPassParams.context = p_Params.context;
        l_RenderPassParams.formatCount = 1;
        Backend::ImageFormat l_RenderpassImageFormat;
        l_RenderpassImageFormat.vk = l_Swapchain.m_ImageFormat;
        l_RenderPassParams.formats = &l_RenderpassImageFormat;
        bool l_ClearRenderpassImage = true;
        l_RenderPassParams.clearTarget = &l_ClearRenderpassImage;
        l_RenderPassParams.useDepth = false;

        vk_renderpass_create(p_Swapchain.renderpass, l_RenderPassParams);

        SwapchainUtils::create_render_targets(p_Swapchain, l_Images);
        SwapchainUtils::create_framebuffers(*p_Params.context, p_Swapchain);

        SwapchainUtils::create_sync_objects(p_Params.context->vk, l_Swapchain);
        SwapchainUtils::create_command_buffers(
            *p_Params.context, *p_Params.commandPool, l_Swapchain);

        LOW_LOG_INFO("Swapchain initialized");
      }

      void vk_swapchain_cleanup(Backend::Swapchain &p_Swapchain)
      {
        Swapchain &l_Swapchain = p_Swapchain.vk;
        Context &l_Context = p_Swapchain.context->vk;

        for (uint32_t i = 0u; i < l_Swapchain.m_ImageCount; ++i) {
          vkDestroySemaphore(l_Context.m_Device,
                             l_Swapchain.m_ImageAvailableSemaphores[i],
                             nullptr);
          vkDestroySemaphore(l_Context.m_Device,
                             l_Swapchain.m_RenderFinishedSemaphores[i],
                             nullptr);
          vkDestroyFence(l_Context.m_Device, l_Swapchain.m_InFlightFences[i],
                         nullptr);
        }

        for (uint32_t i = 0u; i < l_Swapchain.m_ImageCount; ++i) {
          vk_image2d_cleanup(l_Swapchain.m_RenderTargets[i]);
        }

        vkDestroySwapchainKHR(l_Context.m_Device, l_Swapchain.m_Handle,
                              nullptr);
      }

      uint8_t vk_swapchain_prepare(Backend::Swapchain &p_Swapchain)
      {
        vkWaitForFences(
            p_Swapchain.context->vk.m_Device, 1,
            &p_Swapchain.vk
                 .m_InFlightFences[p_Swapchain.vk.m_CurrentFrameIndex],
            VK_TRUE, UINT64_MAX);

        uint32_t l_CurrentImage;

        VkResult l_Result = vkAcquireNextImageKHR(
            p_Swapchain.context->vk.m_Device, p_Swapchain.vk.m_Handle,
            UINT64_MAX,
            p_Swapchain.vk
                .m_ImageAvailableSemaphores[p_Swapchain.vk.m_CurrentFrameIndex],
            VK_NULL_HANDLE, &l_CurrentImage);

        p_Swapchain.vk.m_CurrentImageIndex = l_CurrentImage;

        // Handle window resize
        if (l_Result == VK_ERROR_OUT_OF_DATE_KHR) {
          return Backend::SwapchainState::OUT_OF_DATE;
        }

        LOW_ASSERT(l_Result == VK_SUCCESS || l_Result == VK_SUBOPTIMAL_KHR,
                   "Failed to acquire swapchain image");

        vkResetFences(
            p_Swapchain.context->vk.m_Device, 1,
            &p_Swapchain.vk
                 .m_InFlightFences[p_Swapchain.vk.m_CurrentFrameIndex]);

        return Backend::SwapchainState::SUCCESS;
      }

      void vk_swapchain_swap(Backend::Swapchain &p_Swapchain)
      {
        Swapchain &l_Swapchain = p_Swapchain.vk;

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore l_WaitSemaphores[] = {
            l_Swapchain
                .m_ImageAvailableSemaphores[l_Swapchain.m_CurrentFrameIndex]};
        VkPipelineStageFlags l_WaitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        l_SubmitInfo.waitSemaphoreCount = 1;
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores;
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages;
        l_SubmitInfo.commandBufferCount = 1;
        l_SubmitInfo.pCommandBuffers =
            &(vk_swapchain_get_current_commandbuffer(p_Swapchain).vk.m_Handle);

        VkSemaphore l_SignalSemaphores[] = {
            l_Swapchain
                .m_RenderFinishedSemaphores[l_Swapchain.m_CurrentFrameIndex]};
        l_SubmitInfo.signalSemaphoreCount = 1;
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores;

        VkResult l_SubmitResult = vkQueueSubmit(
            p_Swapchain.context->vk.m_GraphicsQueue, 1, &l_SubmitInfo,
            l_Swapchain.m_InFlightFences[l_Swapchain.m_CurrentFrameIndex]);

        LOW_ASSERT(l_SubmitResult == VK_SUCCESS,
                   "Failed to submit draw command buffer");

        VkPresentInfoKHR l_PresentInfo{};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        l_PresentInfo.waitSemaphoreCount = 1;
        l_PresentInfo.pWaitSemaphores = l_SignalSemaphores;

        VkSwapchainKHR l_Swapchains[] = {l_Swapchain.m_Handle};
        l_PresentInfo.swapchainCount = 1;
        l_PresentInfo.pSwapchains = l_Swapchains;
        uint32_t l_ImageIndex = p_Swapchain.vk.m_CurrentImageIndex;
        l_PresentInfo.pImageIndices = &l_ImageIndex;
        l_PresentInfo.pResults = nullptr;

        VkResult l_Result = vkQueuePresentKHR(
            p_Swapchain.context->vk.m_PresentQueue, &l_PresentInfo);

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

        p_Swapchain.vk.m_CurrentFrameIndex =
            (p_Swapchain.vk.m_CurrentFrameIndex + 1) %
            p_Swapchain.vk.m_FramesInFlight;
      }

      Backend::CommandBuffer &
      vk_swapchain_get_current_commandbuffer(Backend::Swapchain &p_Swapchain)
      {
        return p_Swapchain.vk
            .m_CommandBuffers[p_Swapchain.vk.m_CurrentFrameIndex];
      }

      Backend::CommandBuffer &
      vk_swapchain_get_commandbuffer(Backend::Swapchain &p_Swapchain,
                                     uint8_t p_Index)
      {
        return p_Swapchain.vk.m_CommandBuffers[p_Index];
      }

      Backend::Framebuffer &
      vk_swapchain_get_framebuffer(Backend::Swapchain &p_Swapchain,
                                   uint8_t p_Index)
      {
        return p_Swapchain.vk.m_Framebuffers[p_Index];
      }

      Backend::Framebuffer &
      vk_swapchain_get_current_framebuffer(Backend::Swapchain &p_Swapchain)
      {
        return p_Swapchain.vk
            .m_Framebuffers[p_Swapchain.vk.m_CurrentImageIndex];
      }

      uint8_t vk_swapchain_get_frames_in_flight(Backend::Swapchain &p_Swapchain)
      {
        return p_Swapchain.vk.m_FramesInFlight;
      }

      uint8_t vk_swapchain_get_image_count(Backend::Swapchain &p_Swapchain)
      {
        return p_Swapchain.vk.m_ImageCount;
      }

      uint8_t
      vk_swapchain_get_current_frame_index(Backend::Swapchain &p_Swapchain)
      {
        return p_Swapchain.vk.m_CurrentFrameIndex;
      }

      uint8_t
      vk_swapchain_get_current_image_index(Backend::Swapchain &p_Swapchain)
      {
        return p_Swapchain.vk.m_CurrentImageIndex;
      }

      void vk_commandbuffer_start(Backend::CommandBuffer &p_CommandBuffer)
      {
        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = 0;
        l_BeginInfo.pInheritanceInfo = nullptr;

        LOW_ASSERT(vkBeginCommandBuffer(p_CommandBuffer.vk.m_Handle,
                                        &l_BeginInfo) == VK_SUCCESS,
                   "Failed to begin recording command buffer");
      }

      void vk_commandbuffer_stop(Backend::CommandBuffer &p_CommandBuffer)
      {
        LOW_ASSERT(vkEndCommandBuffer(p_CommandBuffer.vk.m_Handle) ==
                       VK_SUCCESS,
                   "Failed to stop recording the command buffer");
      }

      namespace Image2DUtils {
        static void create_image_view(Backend::Context &p_Context,
                                      Backend::Image2D &p_Image2d,
                                      ImageFormat &p_Format, bool p_Depth)
        {

          VkImageViewCreateInfo l_CreateInfo{};
          l_CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
          l_CreateInfo.image = p_Image2d.vk.m_Image;
          l_CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
          l_CreateInfo.format = p_Format.m_Handle;

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
                                      Backend::Image2D &p_RenderTarget)
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
                                     nullptr, &(p_RenderTarget.vk.m_Sampler)) ==
                         VK_SUCCESS,
                     "Failed to create image2d sampler");
        }

        void create_image(Backend::Context &p_Context, uint32_t p_Width,
                          uint32_t p_Height, VkFormat p_Format,
                          VkImageTiling p_Tiling, VkImageUsageFlags p_Usage,
                          VkMemoryPropertyFlags p_Properties, VkImage &p_Image,
                          VkDeviceMemory &p_ImageMemory)
        {
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

          LOW_ASSERT(vkCreateImage(p_Context.vk.m_Device, &l_ImageInfo, nullptr,
                                   &p_Image) == VK_SUCCESS,
                     "Failed to create image");

          VkMemoryRequirements l_MemRequirements;
          vkGetImageMemoryRequirements(p_Context.vk.m_Device, p_Image,
                                       &l_MemRequirements);

          VkMemoryAllocateInfo l_AllocInfo{};
          l_AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
          l_AllocInfo.allocationSize = l_MemRequirements.size;
          l_AllocInfo.memoryTypeIndex = Utils::find_memory_type(
              p_Context.vk, l_MemRequirements.memoryTypeBits, p_Properties);

          LOW_ASSERT(vkAllocateMemory(p_Context.vk.m_Device, &l_AllocInfo,
                                      nullptr, &p_ImageMemory) == VK_SUCCESS,
                     "Failed to allocate image memory");

          vkBindImageMemory(p_Context.vk.m_Device, p_Image, p_ImageMemory, 0);
        }

      } // namespace Image2DUtils

      void vk_image2d_create(Backend::Image2D &p_Image2d,
                             Backend::Image2DCreateParams &p_Params)
      {
        p_Image2d.context = p_Params.context;

        VkImageUsageFlags l_UsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        if (p_Params.writable) {
          l_UsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if (p_Params.create_image) {
          Image2DUtils::create_image(
              *p_Params.context, p_Params.dimensions.x, p_Params.dimensions.y,
              p_Params.format->vk.m_Handle, VK_IMAGE_TILING_OPTIMAL,
              l_UsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              p_Image2d.vk.m_Image, p_Image2d.vk.m_Memory);
        }

        Image2DUtils::create_image_view(*p_Params.context, p_Image2d,
                                        p_Params.format->vk, p_Params.depth);

        Image2DUtils::create_2d_sampler(*p_Params.context, p_Image2d);

        p_Image2d.swapchainImage = false;
      }

      void vk_image2d_cleanup(Backend::Image2D &p_Image2d)
      {
        Context &l_Context = p_Image2d.context->vk;
        Image2D &l_Image2d = p_Image2d.vk;

        vkDestroySampler(l_Context.m_Device, l_Image2d.m_Sampler, nullptr);
        vkDestroyImageView(l_Context.m_Device, l_Image2d.m_ImageView, nullptr);

        if (!p_Image2d.swapchainImage) {
          vkDestroyImage(l_Context.m_Device, l_Image2d.m_Image, nullptr);
          vkFreeMemory(l_Context.m_Device, l_Image2d.m_Memory, nullptr);
        }
      }

      namespace PipelineUtils {
        typedef VkVertexInputBindingDescription(BindingDescriptionCallback)();
        typedef Low::Util::List<VkVertexInputAttributeDescription>(
            InputAttributeCallback)();

        namespace VertexEmpty {
          static VkVertexInputBindingDescription get_binding_description()
          {
            VkVertexInputBindingDescription l_BindingDescription{};
            l_BindingDescription.binding = 0;
            l_BindingDescription.stride = 0;
            l_BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return l_BindingDescription;
          }

          static Low::Util::List<VkVertexInputAttributeDescription>
          get_attribute_descriptions()
          {
            Low::Util::List<VkVertexInputAttributeDescription>
                l_AttributeDescriptions;

            return l_AttributeDescriptions;
          }
        } // namespace VertexEmpty

        namespace VertexBasic {
          static VkVertexInputBindingDescription get_binding_description()
          {
            VkVertexInputBindingDescription l_BindingDescription{};
            l_BindingDescription.binding = 0;
            l_BindingDescription.stride =
                sizeof(Low::Math::Vector3) + sizeof(Low::Math::Vector3) +
                sizeof(Low::Math::Vector2) + sizeof(Low::Math::Vector3) +
                sizeof(Low::Math::Vector3);
            l_BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return l_BindingDescription;
          }

          static Low::Util::List<VkVertexInputAttributeDescription>
          get_attribute_descriptions()
          {
            Low::Util::List<VkVertexInputAttributeDescription>
                l_AttributeDescriptions;
            l_AttributeDescriptions.resize(5);

            l_AttributeDescriptions[0].binding = 0;
            l_AttributeDescriptions[0].location = 0;
            l_AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            l_AttributeDescriptions[0].offset = 0;

            l_AttributeDescriptions[1].binding = 0;
            l_AttributeDescriptions[1].location = 1;
            l_AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            l_AttributeDescriptions[1].offset = sizeof(Low::Math::Vector3);

            l_AttributeDescriptions[2].binding = 0;
            l_AttributeDescriptions[2].location = 2;
            l_AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            l_AttributeDescriptions[2].offset = sizeof(Low::Math::Vector3) * 2;

            l_AttributeDescriptions[3].binding = 0;
            l_AttributeDescriptions[3].location = 3;
            l_AttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            l_AttributeDescriptions[3].offset =
                sizeof(Low::Math::Vector3) * 2 + sizeof(Low::Math::Vector2);

            l_AttributeDescriptions[4].binding = 0;
            l_AttributeDescriptions[4].location = 4;
            l_AttributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            l_AttributeDescriptions[4].offset =
                sizeof(Low::Math::Vector3) * 3 + sizeof(Low::Math::Vector2);

            return l_AttributeDescriptions;
          }
        } // namespace VertexBasic

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
      } // namespace PipelineUtils

      void vk_pipeline_interface_create(
          Backend::PipelineInterface &p_PipelineInterface,
          Backend::PipelineInterfaceCreateParams &p_Params)
      {
        p_PipelineInterface.context = p_Params.context;

        VkPipelineLayoutCreateInfo l_PipelineLayoutInfo{};
        l_PipelineLayoutInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_PipelineLayoutInfo.setLayoutCount = 0;
        l_PipelineLayoutInfo.pSetLayouts = nullptr;
        l_PipelineLayoutInfo.pushConstantRangeCount = 0;
        l_PipelineLayoutInfo.pPushConstantRanges = nullptr;

        LOW_ASSERT(vkCreatePipelineLayout(p_Params.context->vk.m_Device,
                                          &l_PipelineLayoutInfo, nullptr,
                                          &(p_PipelineInterface.vk.m_Handle)) ==
                       VK_SUCCESS,
                   "Failed to create pipeline layout");
      }

      void vk_pipeline_interface_cleanup(
          Backend::PipelineInterface &p_PipelineInterface)
      {
        vkDestroyPipelineLayout(p_PipelineInterface.context->vk.m_Device,
                                p_PipelineInterface.vk.m_Handle, nullptr);
      }

      void vk_pipeline_graphics_create(
          Backend::Pipeline &p_Pipeline,
          Backend::GraphicsPipelineCreateParams &p_Params)
      {
        p_Pipeline.context = p_Params.context;

        auto l_VertexShaderCode =
            PipelineUtils::read_shader_file(p_Params.vertexShaderPath);
        auto l_FragmentShaderCode =
            PipelineUtils::read_shader_file(p_Params.fragmentShaderPath);

        VkShaderModule l_VertexShaderModule =
            PipelineUtils::create_shader_module(*p_Params.context,
                                                l_VertexShaderCode);
        VkShaderModule l_FragmentShaderModule =
            PipelineUtils::create_shader_module(*p_Params.context,
                                                l_FragmentShaderCode);

        VkPipelineShaderStageCreateInfo l_VertexShaderStageInfo{};
        l_VertexShaderStageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_VertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        l_VertexShaderStageInfo.module = l_VertexShaderModule;
        l_VertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo l_FragmentShaderStageInfo{};
        l_FragmentShaderStageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_FragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
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

        VkVertexInputBindingDescription l_BindingDescription;
        Low::Util::List<VkVertexInputAttributeDescription>
            l_AttributeDescription;

        if (p_Params.vertexInput) {
          l_BindingDescription =
              PipelineUtils::VertexBasic::get_binding_description();
          l_AttributeDescription =
              PipelineUtils::VertexBasic::get_attribute_descriptions();
        } else {
          l_BindingDescription =
              PipelineUtils::VertexEmpty::get_binding_description();
          l_AttributeDescription =
              PipelineUtils::VertexEmpty::get_attribute_descriptions();
        }

        l_VertexInputInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        if (p_Params.vertexInput) {
          l_VertexInputInfo.vertexBindingDescriptionCount = 1;
          l_VertexInputInfo.pVertexBindingDescriptions = &l_BindingDescription;
          l_VertexInputInfo.vertexAttributeDescriptionCount =
              l_AttributeDescription.size();
          l_VertexInputInfo.pVertexAttributeDescriptions =
              l_AttributeDescription.data();
        } else {
          l_VertexInputInfo.vertexBindingDescriptionCount = 0;
          l_VertexInputInfo.pVertexBindingDescriptions = nullptr;
          l_VertexInputInfo.vertexAttributeDescriptionCount = 0;
          l_VertexInputInfo.pVertexAttributeDescriptions = nullptr;
        }

        VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
        l_InputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        l_InputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport l_Viewport{};
        l_Viewport.x = 0.f;
        l_Viewport.y = 0.f;
        l_Viewport.width = (float)p_Params.dimensions.x;
        l_Viewport.height = (float)p_Params.dimensions.y;
        l_Viewport.minDepth = 0.f;
        l_Viewport.maxDepth = 1.f;

        VkExtent2D l_Extent{p_Params.dimensions.x, p_Params.dimensions.y};

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
        // ~l_Rasterizer.lineWidth~ so be set to some float. If the float is
        // larger than 1 then a special GPU feature ~wideLines~ has to be
        // enabled
        l_Rasterizer.lineWidth = 1.f;
        if (p_Params.cullMode == Backend::PipelineRasterizerCullMode::FRONT) {
          l_Rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
        } else if (p_Params.cullMode ==
                   Backend::PipelineRasterizerCullMode::BACK) {
          l_Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        } else {
          LOW_ASSERT(false, "Unknown pipeline rasterizer cull mode");
        }

        if (p_Params.frontFace ==
            Backend::PipelineRasterizerFrontFace::CLOCKWISE) {
          l_Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        } else if (p_Params.frontFace ==
                   Backend::PipelineRasterizerFrontFace::COUNTER_CLOCKWISE) {
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
        l_DepthStencil.depthTestEnable = VK_TRUE;
        l_DepthStencil.depthWriteEnable = VK_TRUE;
        l_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
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

          int test = i_Target.wirteMask & LOW_RENDERER_COLOR_WRITE_BIT_RED;

          VkColorComponentFlags l_WriteMask = 0;
          if ((i_Target.wirteMask & LOW_RENDERER_COLOR_WRITE_BIT_RED) ==
              LOW_RENDERER_COLOR_WRITE_BIT_RED) {
            l_WriteMask |= VK_COLOR_COMPONENT_R_BIT;
          }
          if ((i_Target.wirteMask & LOW_RENDERER_COLOR_WRITE_BIT_GREEN) ==
              LOW_RENDERER_COLOR_WRITE_BIT_GREEN) {
            l_WriteMask |= VK_COLOR_COMPONENT_G_BIT;
          }
          if ((i_Target.wirteMask & LOW_RENDERER_COLOR_WRITE_BIT_BLUE) ==
              LOW_RENDERER_COLOR_WRITE_BIT_BLUE) {
            l_WriteMask |= VK_COLOR_COMPONENT_B_BIT;
          }
          if ((i_Target.wirteMask & LOW_RENDERER_COLOR_WRITE_BIT_ALPHA) ==
              LOW_RENDERER_COLOR_WRITE_BIT_ALPHA) {
            l_WriteMask |= VK_COLOR_COMPONENT_A_BIT;
          }

          VkPipelineColorBlendAttachmentState l_ColorBlendAttachment{};
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
          l_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
          l_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
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
        l_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
        l_PipelineInfo.layout = p_Params.interface->vk.m_Handle;
        l_PipelineInfo.renderPass = p_Params.renderpass->vk.m_Handle;
        l_PipelineInfo.subpass = 0;
        l_PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        l_PipelineInfo.basePipelineIndex = -1;

        LOW_ASSERT(vkCreateGraphicsPipelines(
                       p_Params.context->vk.m_Device, VK_NULL_HANDLE, 1,
                       &l_PipelineInfo, nullptr,
                       &(p_Pipeline.vk.m_Handle)) == VK_SUCCESS,
                   "Failed to create graphics pipeline");

        LOW_LOG_DEBUG("Graphics pipeline created");

        vkDestroyShaderModule(p_Params.context->vk.m_Device,
                              l_FragmentShaderModule, nullptr);
        vkDestroyShaderModule(p_Params.context->vk.m_Device,
                              l_VertexShaderModule, nullptr);
      }

      void vk_pipeline_cleanup(Backend::Pipeline &p_Pipeline)
      {
        vkDestroyPipeline(p_Pipeline.context->vk.m_Device,
                          p_Pipeline.vk.m_Handle, nullptr);
      }

      void vk_pipeline_bind(Backend::Pipeline &p_Pipeline,
                            Backend::PipelineBindParams &p_Params)
      {
        vkCmdBindPipeline(p_Params.commandBuffer->vk.m_Handle,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          p_Pipeline.vk.m_Handle);
      }

      void vk_draw(Backend::DrawParams &p_Params)
      {
        vkCmdDraw(p_Params.commandBuffer->vk.m_Handle, p_Params.vertexCount,
                  p_Params.instanceCount, p_Params.firstVertex,
                  p_Params.firstInstance);
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low

#undef SKIP_DEBUG_LEVEL
