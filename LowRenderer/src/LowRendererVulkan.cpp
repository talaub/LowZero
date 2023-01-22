#include "LowRendererVulkan.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"

#include "vulkan/vulkan_core.h"

#include <GLFW/glfw3.h>

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
        query_swap_chain_support(VulkanContext &p_Context,
                                 VkPhysicalDevice p_Device)
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
        find_queue_families(VulkanContext &p_Context, VkPhysicalDevice p_Device)
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

        static void create_instance(VulkanContext &p_Context)
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

        static void setup_debug_messenger(VulkanContext &p_Context)
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

        static int rate_device_suitability(VulkanContext &p_Context,
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

        static void select_physical_device(VulkanContext &p_Context)
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

        static void create_logical_device(VulkanContext &p_Context)
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
                             Backend::ContextInit &p_Init)
      {
        // Check for validation layer support
        if (p_Init.validation_enabled) {
          LOW_ASSERT(ContextUtils::check_validation_layer_support(),
                     "Validation layers requested, but not available");
          LOW_LOG_DEBUG("Validation layers enabled");
        }

        p_Context.vk.m_ValidationEnabled = p_Init.validation_enabled;
        p_Context.m_Window = *(p_Init.window);

        ContextUtils::create_instance(p_Context.vk);

        ContextUtils::setup_debug_messenger(p_Context.vk);

        ContextUtils::create_surface(p_Context);

        ContextUtils::select_physical_device(p_Context.vk);

        ContextUtils::create_logical_device(p_Context.vk);

        LOW_LOG_DEBUG("Vulkan context initialized");
      }

      void vk_context_cleanup(Backend::Context &p_Context)
      {
        VulkanContext l_Context = p_Context.vk;

        vkDestroyDevice(l_Context.m_Device, nullptr);

        vkDestroySurfaceKHR(l_Context.m_Instance, l_Context.m_Surface, nullptr);

        if (l_Context.m_ValidationEnabled) {
          ContextUtils::destroy_debug_utils_messenger_ext(
              l_Context.m_Instance, l_Context.m_DebugMessenger, nullptr);
        }

        vkDestroyInstance(l_Context.m_Instance, nullptr);
      }

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low

#undef SKIP_DEBUG_LEVEL
