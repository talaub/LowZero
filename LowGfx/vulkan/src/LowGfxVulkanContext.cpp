#include "LowGfxContext.h"
#include "LowGfxVulkanBackend.h"

#include "LowGfxLogInternal.h"
#include "LowGfxVulkanState.h"
#include "LowUtilAssert.h"
#include "VkBootstrap.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <cstdio>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Gfx {
    namespace Vulkan {
      static void configure_instance_window_extensions(
          Detail::InstanceImpl &p_Instance,
          vkb::InstanceBuilder &p_InstanceBuilder,
          const InstanceDesc &p_Desc)
      {
        switch (p_Desc.surface_window.backend) {
        case WindowBackend::None:
          p_InstanceBuilder.set_headless();
          return;
        case WindowBackend::SDL: {
          LOW_ASSERT(
              p_Desc.surface_window.handle,
              "SDL surface extension setup requires a window handle");

          SDL_Window *l_Window =
              static_cast<SDL_Window *>(p_Desc.surface_window.handle);
          unsigned int l_ExtensionCount = 0;
          if (!SDL_Vulkan_GetInstanceExtensions(
                  l_Window, &l_ExtensionCount, nullptr)) {
            Detail::logf(p_Instance, LogLevel::Error,
                         "Failed to query SDL Vulkan instance "
                         "extension count: {}",
                         SDL_GetError());
            LOW_ASSERT(
                false,
                "Failed to query SDL Vulkan instance extensions");
          }

          std::vector<const char *> l_Extensions(l_ExtensionCount);
          if (!SDL_Vulkan_GetInstanceExtensions(
                  l_Window, &l_ExtensionCount, l_Extensions.data())) {
            Detail::logf(
                p_Instance, LogLevel::Error,
                "Failed to query SDL Vulkan instance extensions: {}",
                SDL_GetError());
            LOW_ASSERT(
                false,
                "Failed to query SDL Vulkan instance extensions");
          }

          p_InstanceBuilder.enable_extensions(l_Extensions);
          p_InstanceBuilder.set_headless(false);
          return;
        }
        }

        LOW_ASSERT(false, "Unsupported LowGfx window backend");
      }

      static VulkanQueue
      get_required_queue(Detail::ContextImpl &p_Context,
                         const vkb::Device &p_Device,
                         vkb::QueueType p_Type, const char *p_Name)
      {
        vkb::Result<VkQueue> l_Queue = p_Device.get_queue(p_Type);
        if (!l_Queue) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to get Vulkan {} queue: {}", p_Name,
                       l_Queue.full_error().type.message().c_str());
        }
        LOW_ASSERT(l_Queue, "Failed to get required Vulkan queue");

        vkb::Result<u32> l_FamilyIndex =
            p_Device.get_queue_index(p_Type);
        if (!l_FamilyIndex) {
          Detail::logf(
              p_Context, LogLevel::Error,
              "Failed to get Vulkan {} queue family index: {}",
              p_Name,
              l_FamilyIndex.full_error().type.message().c_str());
        }
        LOW_ASSERT(
            l_FamilyIndex,
            "Failed to get required Vulkan queue family index");

        VulkanQueue l_Result;
        l_Result.queue = l_Queue.value();
        l_Result.family_index = l_FamilyIndex.value();
        return l_Result;
      }

      static VulkanQueue
      get_optional_queue_or_fallback(const vkb::Device &p_Device,
                                     vkb::QueueType p_Type,
                                     const VulkanQueue &p_Fallback)
      {
        vkb::Result<VkQueue> l_Queue = p_Device.get_queue(p_Type);
        vkb::Result<u32> l_FamilyIndex =
            p_Device.get_queue_index(p_Type);

        if (!l_Queue || !l_FamilyIndex) {
          return p_Fallback;
        }

        VulkanQueue l_Result;
        l_Result.queue = l_Queue.value();
        l_Result.family_index = l_FamilyIndex.value();
        return l_Result;
      }

      static void
      create_frame_command_pool(Detail::ContextImpl &p_Context,
                                const VulkanContextState &p_State,
                                const VulkanQueue &p_Queue,
                                const char *p_Name,
                                VulkanFrameCommandPool &p_Pool)
      {
        LOW_ASSERT(p_Queue.family_index != LOW_UINT32_MAX,
                   "Cannot create Vulkan command pool without queue "
                   "family");

        VkCommandPoolCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_Info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        l_Info.queueFamilyIndex = p_Queue.family_index;

        VkResult l_Result = vkCreateCommandPool(
            p_State.device, &l_Info, nullptr, &p_Pool.pool);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create Vulkan {} command pool: {}",
                       p_Name, static_cast<int>(l_Result));
        }
        LOW_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to create Vulkan command pool");
        p_Pool.next_command_buffer = 0;
      }

      static void
      reset_frame_command_pool(VkDevice p_Device,
                               VulkanFrameCommandPool &p_Pool)
      {
        if (p_Pool.pool == VK_NULL_HANDLE) {
          return;
        }

        VkResult l_Result = vkResetCommandPool(p_Device, p_Pool.pool,
                                               0);
        LOW_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to reset Vulkan frame command pool");
        p_Pool.next_command_buffer = 0;
      }

      static void
      destroy_frame_command_pool(VkDevice p_Device,
                                 VulkanFrameCommandPool &p_Pool)
      {
        if (p_Pool.pool != VK_NULL_HANDLE) {
          vkDestroyCommandPool(p_Device, p_Pool.pool, nullptr);
          p_Pool.pool = VK_NULL_HANDLE;
        }

        p_Pool.command_buffers.clear();
        p_Pool.next_command_buffer = 0;
      }

      static void destroy_frame_state(VkDevice p_Device,
                                      FrameState &p_Frame)
      {
        destroy_frame_command_pool(p_Device, p_Frame.graphics);
        destroy_frame_command_pool(p_Device, p_Frame.compute);
        destroy_frame_command_pool(p_Device, p_Frame.transfer);
        if (p_Frame.frame_fence != VK_NULL_HANDLE) {
          vkDestroyFence(p_Device, p_Frame.frame_fence, nullptr);
          p_Frame.frame_fence = VK_NULL_HANDLE;
        }
      }

      static VkDescriptorPool create_descriptor_pool(
          VkDevice p_Device, u32 p_SetCount,
          Util::Span<const VulkanDescriptorPoolSizeRatio> p_Ratios)
      {
        Util::List<VkDescriptorPoolSize> l_PoolSizes;
        l_PoolSizes.reserve(p_Ratios.size());
        for (const VulkanDescriptorPoolSizeRatio &i_Ratio :
             p_Ratios) {
          VkDescriptorPoolSize l_Size{};
          l_Size.type = i_Ratio.type;
          l_Size.descriptorCount =
              static_cast<u32>(i_Ratio.ratio * p_SetCount);
          if (l_Size.descriptorCount == 0) {
            l_Size.descriptorCount = 1;
          }
          l_PoolSizes.push_back(l_Size);
        }

        VkDescriptorPoolCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_Info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        l_Info.maxSets = p_SetCount;
        l_Info.poolSizeCount = l_PoolSizes.size();
        l_Info.pPoolSizes = l_PoolSizes.data();

        VkDescriptorPool l_Pool = VK_NULL_HANDLE;
        VkResult l_Result = vkCreateDescriptorPool(
            p_Device, &l_Info, nullptr, &l_Pool);
        LOW_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to create Vulkan descriptor pool");
        return l_Pool;
      }

      static void init_descriptor_allocator(
          VulkanDescriptorAllocatorGrowable &p_Allocator,
          VkDevice p_Device)
      {
        p_Allocator.ratios = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 30.0f},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 50.0f},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100.0f},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100.0f},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 700.0f},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 32.0f}};
        p_Allocator.sets_per_pool = 15;
        p_Allocator.ready_pools.push_back(create_descriptor_pool(
            p_Device, 10, Util::Span<const VulkanDescriptorPoolSizeRatio>(
                              p_Allocator.ratios.data(),
                              p_Allocator.ratios.size())));
      }

      static void destroy_descriptor_allocator(
          VulkanDescriptorAllocatorGrowable &p_Allocator,
          VkDevice p_Device)
      {
        for (VkDescriptorPool i_Pool : p_Allocator.ready_pools) {
          vkDestroyDescriptorPool(p_Device, i_Pool, nullptr);
        }
        p_Allocator.ready_pools.clear();

        for (VkDescriptorPool i_Pool : p_Allocator.full_pools) {
          vkDestroyDescriptorPool(p_Device, i_Pool, nullptr);
        }
        p_Allocator.full_pools.clear();

        p_Allocator.ratios.clear();
        p_Allocator.sets_per_pool = 0;
      }

      static void create_frame_state(Detail::ContextImpl &p_Context,
                                     VulkanContextState &p_State,
                                     FrameState &p_Frame)
      {
        create_frame_command_pool(p_Context, p_State,
                                  p_State.graphics_queue, "graphics",
                                  p_Frame.graphics);
        create_frame_command_pool(p_Context, p_State,
                                  p_State.compute_queue, "compute",
                                  p_Frame.compute);
        create_frame_command_pool(p_Context, p_State,
                                  p_State.transfer_queue, "transfer",
                                  p_Frame.transfer);

        VkFenceCreateInfo l_FenceInfo{};
        l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        l_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkResult l_FenceResult = vkCreateFence(
            p_State.device, &l_FenceInfo, nullptr, &p_Frame.frame_fence);
        if (l_FenceResult != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create Vulkan frame fence: {}",
                       static_cast<int>(l_FenceResult));
        }
        LOW_ASSERT(l_FenceResult == VK_SUCCESS,
                   "Failed to create Vulkan frame fence");

      }

      VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
          VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
          VkDebugUtilsMessageTypeFlagsEXT messageTypes,
          const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
          void *pUserData)
      {
        const char *l_Severity =
            messageSeverity &
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                ? "error"
            : messageSeverity &
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                ? "warning"
            : messageSeverity &
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                ? "info"
                : "verbose";

        const char *l_Type =
            messageTypes &
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                ? "validation"
            : messageTypes &
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                ? "performance"
                : "general";

        LogLevel l_Level = LogLevel::Debug;
        if (messageSeverity &
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
          l_Level = LogLevel::Error;
        } else if (messageSeverity &
                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
          l_Level = LogLevel::Warning;
        } else if (messageSeverity &
                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
          l_Level = LogLevel::Info;
        }

        Detail::InstanceImpl *l_Instance =
            static_cast<Detail::InstanceImpl *>(pUserData);
        const char *l_Message =
            pCallbackData && pCallbackData->pMessage
                ? pCallbackData->pMessage
                : "";

        if (l_Instance) {
          Detail::logf(*l_Instance, l_Level,
                       "Vulkan validation {} [{}]: {}", l_Severity,
                       l_Type, l_Message);
        } else {
          std::fprintf(stderr,
                       "[LowGfx] %s: Vulkan validation %s [%s]: %s\n",
                       Detail::log_level_name(l_Level), l_Severity,
                       l_Type, l_Message);
        }

        return VK_FALSE;
      }

      void *create_instance(Detail::InstanceImpl &p_Instance,
                            const InstanceDesc &p_Desc)
      {
        VulkanInstanceState *l_State = new VulkanInstanceState();
        l_State->validation_enabled = p_Desc.enable_validation;

        vkb::InstanceBuilder l_InstanceBuilder;
        l_InstanceBuilder.set_app_name("LowEngine")
            .request_validation_layers(p_Desc.enable_validation)
            .set_debug_callback(&debug_callback)
            .set_debug_callback_user_data_pointer(&p_Instance)
            .require_api_version(1, 3, 0);

        configure_instance_window_extensions(
            p_Instance, l_InstanceBuilder, p_Desc);

#ifdef LOW_GFX_VALIDATION_VERBOSE
        l_InstanceBuilder.set_debug_messenger_severity(
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
#endif

#ifdef LOW_GFX_VALIDATION_BEST_PRACTICES
        if (p_Desc.enable_validation) {
          l_InstanceBuilder.add_validation_feature_enable(
              VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
        }
#endif

#ifdef LOW_GFX_VALIDATION_SYNCHRONIZATION
        if (p_Desc.enable_validation) {
          l_InstanceBuilder.add_validation_feature_enable(
              VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
        }
#endif

#ifdef LOW_GFX_VALIDATION_GPU_ASSISTED
        if (p_Desc.enable_validation) {
          l_InstanceBuilder.add_validation_feature_enable(
              VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
          l_InstanceBuilder.add_validation_feature_enable(
              VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
        }
#endif

        vkb::Result<vkb::Instance> l_InstanceReturn =
            l_InstanceBuilder.build();

        if (!l_InstanceReturn) {
          Detail::logf(
              p_Instance, LogLevel::Error,
              "Failed to initialize Vulkan instance: {}",
              l_InstanceReturn.full_error().type.message().c_str());
        }
        LOW_ASSERT(l_InstanceReturn,
                   "Failed to initialize vulkan instance");

        l_State->vkb_instance = l_InstanceReturn.value();
        l_State->instance = l_State->vkb_instance.instance;
        l_State->debug_messenger =
            l_State->vkb_instance.debug_messenger;
        return l_State;
      }

      void destroy_instance(Detail::InstanceImpl &p_Instance)
      {
        VulkanInstanceState *l_State =
            static_cast<VulkanInstanceState *>(p_Instance.backend_state);
        if (!l_State) {
          return;
        }

        for (Detail::BackendAdapter &i_Adapter :
             p_Instance.adapters) {
          delete static_cast<VulkanAdapterState *>(
              i_Adapter.backend_state);
          i_Adapter.backend_state = nullptr;
        }

        if (l_State->instance != VK_NULL_HANDLE) {
          vkb::destroy_instance(l_State->vkb_instance);
        }

        delete l_State;
        p_Instance.backend_state = nullptr;
      }

      void enumerate_adapters(Detail::InstanceImpl &p_Instance)
      {
        (void)p_Instance;
      }

      Adapter select_adapter(const Detail::InstanceImpl &p_Instance,
                             const AdapterSelectionDesc &p_Desc)
      {
        VulkanInstanceState *l_InstanceState =
            static_cast<VulkanInstanceState *>(
                p_Instance.backend_state);
        LOW_ASSERT(l_InstanceState,
                   "Cannot select Vulkan adapter without instance");

        VkSurfaceKHR l_Surface = VK_NULL_HANDLE;
        if (p_Desc.compatible_surface) {
          const Detail::BackendSurface *l_BackendSurface =
              p_Instance.surfaces.get(p_Desc.compatible_surface);
          LOW_ASSERT(l_BackendSurface,
                     "Cannot select adapter for invalid surface");
          VulkanSurfaceState *l_SurfaceState =
              static_cast<VulkanSurfaceState *>(
                  l_BackendSurface->backend_state);
          LOW_ASSERT(l_SurfaceState,
                     "Cannot select adapter for invalid Vulkan surface");
          l_Surface = l_SurfaceState->surface;
        }

        VkPhysicalDeviceVulkan13Features l_Features13{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        l_Features13.dynamicRendering = true;
        l_Features13.synchronization2 = true;

        VkPhysicalDeviceVulkan12Features l_Features12{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        l_Features12.bufferDeviceAddress = true;
        l_Features12.descriptorIndexing = true;
        l_Features12.shaderSampledImageArrayNonUniformIndexing = true;

        VkPhysicalDeviceFeatures l_Features{};
        l_Features.fillModeNonSolid = VK_TRUE;
        l_Features.wideLines = VK_TRUE;

        vkb::PhysicalDeviceSelector l_Selector{
            l_InstanceState->vkb_instance};
        if (l_Surface != VK_NULL_HANDLE) {
          l_Selector.set_surface(l_Surface);
        }

        vkb::Result<vkb::PhysicalDevice> l_Selected =
            l_Selector.set_minimum_version(1, 3)
                .set_required_features_13(l_Features13)
                .set_required_features_12(l_Features12)
                .set_required_features(l_Features)
                .select();

        if (!l_Selected) {
          Detail::logf(
              const_cast<Detail::InstanceImpl &>(p_Instance),
              LogLevel::Error,
              "Failed to select Vulkan physical device: {}",
              l_Selected.full_error().type.message().c_str());
        }
        LOW_ASSERT(l_Selected,
                   "Failed to select Vulkan physical device");

        VulkanAdapterState *l_AdapterState =
            new VulkanAdapterState();
        l_AdapterState->physical_device = l_Selected.value();
        l_AdapterState->gpu =
            l_AdapterState->physical_device.physical_device;

        Detail::BackendAdapter l_BackendAdapter;
        l_BackendAdapter.backend_state = l_AdapterState;
        l_BackendAdapter.generation = 1;

        Detail::InstanceImpl &l_MutableInstance =
            const_cast<Detail::InstanceImpl &>(p_Instance);
        const u32 l_Index =
            static_cast<u32>(l_MutableInstance.adapters.size());
        l_MutableInstance.adapters.push_back(l_BackendAdapter);

        Adapter l_Adapter;
        l_Adapter.index = l_Index;
        l_Adapter.generation = l_BackendAdapter.generation;
        l_Adapter.owner_id = p_Instance.instance_id;
        return l_Adapter;
      }

      void *create_context(Detail::ContextImpl &p_Context,
                           Detail::InstanceImpl &p_Instance,
                           Adapter p_Adapter,
                           const ContextDesc &p_Desc)
      {
        VulkanInstanceState *l_InstanceState =
            static_cast<VulkanInstanceState *>(
                p_Instance.backend_state);
        LOW_ASSERT(l_InstanceState,
                   "Cannot create Vulkan context without instance");
        LOW_ASSERT(p_Adapter.index < p_Instance.adapters.size(),
                   "Cannot create Vulkan context with invalid adapter");

        VulkanAdapterState *l_AdapterState =
            static_cast<VulkanAdapterState *>(
                p_Instance.adapters[p_Adapter.index].backend_state);
        LOW_ASSERT(l_AdapterState,
                   "Cannot create Vulkan context without adapter state");

        VulkanContextState *l_State = new VulkanContextState();
        l_State->frames_in_flight = p_Desc.frames_in_flight;
        l_State->instance_state = l_InstanceState;
        l_State->gpu = l_AdapterState->gpu;

        vkb::DeviceBuilder l_DeviceBuilder{
            l_AdapterState->physical_device};
        vkb::Result<vkb::Device> l_DeviceReturn =
            l_DeviceBuilder.build();

        if (!l_DeviceReturn) {
          Detail::logf(
              p_Context, LogLevel::Error,
              "Failed to create Vulkan device: {}",
              l_DeviceReturn.full_error().type.message().c_str());
        }
        LOW_ASSERT(l_DeviceReturn, "Failed to create Vulkan device");
        l_State->vkb_device = l_DeviceReturn.value();
        l_State->device = l_State->vkb_device.device;

        VmaAllocatorCreateInfo l_AllocatorInfo{};
        l_AllocatorInfo.physicalDevice = l_State->gpu;
        l_AllocatorInfo.device = l_State->device;
        l_AllocatorInfo.instance = l_InstanceState->instance;
        l_AllocatorInfo.flags =
            VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        l_AllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        VkResult l_AllocatorResult =
            vmaCreateAllocator(&l_AllocatorInfo, &l_State->allocator);
        if (l_AllocatorResult != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create VMA allocator: {}",
                       static_cast<int>(l_AllocatorResult));
        }
        LOW_ASSERT(l_AllocatorResult == VK_SUCCESS,
                   "Failed to create VMA allocator");

        l_State->graphics_queue =
            get_required_queue(p_Context, l_State->vkb_device,
                               vkb::QueueType::graphics, "graphics");
        l_State->compute_queue = get_optional_queue_or_fallback(
            l_State->vkb_device, vkb::QueueType::compute,
            l_State->graphics_queue);
        l_State->transfer_queue = get_optional_queue_or_fallback(
            l_State->vkb_device, vkb::QueueType::transfer,
            l_State->graphics_queue);
        l_State->present_queue = get_optional_queue_or_fallback(
            l_State->vkb_device, vkb::QueueType::present,
            l_State->graphics_queue);

        init_descriptor_allocator(l_State->descriptor_allocator,
                                  l_State->device);

        for (u32 i = 0; i < p_Desc.frames_in_flight; ++i) {
          FrameState l_Frame;
          create_frame_state(p_Context, *l_State, l_Frame);
          l_State->frames.push_back(l_Frame);
        }

        return l_State;
      }

      void destroy_context(Detail::ContextImpl &p_Context)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        if (!l_State) {
          return;
        }

        if (l_State->device != VK_NULL_HANDLE) {
          vkDeviceWaitIdle(l_State->device);
        }

        if (l_State->device != VK_NULL_HANDLE) {
          for (FrameState &i_Frame : l_State->frames) {
            destroy_frame_state(l_State->device, i_Frame);
          }
          l_State->frames.clear();
        }

        if (l_State->device != VK_NULL_HANDLE) {
          destroy_descriptor_allocator(l_State->descriptor_allocator,
                                       l_State->device);
        }

        if (l_State->allocator) {
          vmaDestroyAllocator(l_State->allocator);
          l_State->allocator = nullptr;
        }

        if (l_State->device != VK_NULL_HANDLE) {
          vkb::destroy_device(l_State->vkb_device);
        }

        delete l_State;
        p_Context.backend_state = nullptr;
      }

      DeviceCaps get_caps(const Detail::ContextImpl &p_Context)
      {
        (void)p_Context;

        DeviceCaps l_Caps;
        l_Caps.compute = true;
        l_Caps.storage_buffers = true;
        l_Caps.storage_images = true;
        l_Caps.bindless_sampled_textures = true;
        l_Caps.sampled_texture_arrays = true;
        l_Caps.indirect_draw = true;
        l_Caps.multi_draw_indirect = true;
        l_Caps.timeline_sync = false;
        l_Caps.max_bind_groups = 4;
        l_Caps.max_sampled_textures_per_bind_group = 1024;
        l_Caps.max_storage_buffers_per_bind_group = 16;
        l_Caps.max_storage_buffer_range = LOW_UINT32_MAX;
        l_Caps.max_inline_uniform_bytes = 128;
        return l_Caps;
      }

      static VkCommandBuffer record_present_transition(
          VulkanContextState &p_State, FrameState &p_Frame,
          VulkanSwapchainImageState &p_Image)
      {
        if (p_Image.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
          return VK_NULL_HANDLE;
        }

        VkCommandBuffer l_Command = acquire_frame_command_buffer(
            p_State, p_Frame.graphics);

        VkCommandBufferBeginInfo l_BeginInfo{};
        l_BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VkResult l_BeginResult =
            vkBeginCommandBuffer(l_Command, &l_BeginInfo);
        LOW_ASSERT(l_BeginResult == VK_SUCCESS,
                   "Failed to begin Vulkan present transition command "
                   "buffer");

        VkImageMemoryBarrier2 l_Barrier{};
        l_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_Barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        l_Barrier.srcAccessMask = VK_ACCESS_2_NONE;
        l_Barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
        l_Barrier.dstAccessMask = VK_ACCESS_2_NONE;
        l_Barrier.oldLayout = p_Image.layout;
        l_Barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_Barrier.image = p_Image.image;
        l_Barrier.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;
        l_Barrier.subresourceRange.baseMipLevel = 0;
        l_Barrier.subresourceRange.levelCount = 1;
        l_Barrier.subresourceRange.baseArrayLayer = 0;
        l_Barrier.subresourceRange.layerCount = 1;

        VkDependencyInfo l_Dependency{};
        l_Dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_Dependency.imageMemoryBarrierCount = 1;
        l_Dependency.pImageMemoryBarriers = &l_Barrier;
        vkCmdPipelineBarrier2(l_Command, &l_Dependency);

        VkResult l_EndResult = vkEndCommandBuffer(l_Command);
        LOW_ASSERT(l_EndResult == VK_SUCCESS,
                   "Failed to end Vulkan present transition command "
                   "buffer");

        return l_Command;
      }

      void begin_frame(Detail::ContextImpl &p_Context,
                       const FrameContext &p_Frame)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        LOW_ASSERT(l_State,
                   "Cannot begin Vulkan frame without context state");
        const u32 l_FrameIndex =
            Detail::FrameContextAccess::frame_index(p_Frame);

        LOW_ASSERT(l_FrameIndex < l_State->frames.size(),
                   "Cannot begin Vulkan frame with invalid frame index");

        FrameState &l_Frame = l_State->frames[l_FrameIndex];

        VkResult l_WaitResult = vkWaitForFences(
            l_State->device, 1, &l_Frame.frame_fence, VK_TRUE,
            UINT64_MAX);
        LOW_ASSERT(l_WaitResult == VK_SUCCESS,
                   "Failed to wait for Vulkan frame fence");

        reset_frame_command_pool(l_State->device, l_Frame.graphics);
        reset_frame_command_pool(l_State->device, l_Frame.compute);
        reset_frame_command_pool(l_State->device, l_Frame.transfer);

        LOW_ASSERT(l_Frame.pending_graphics_submits.empty(),
                   "Cannot begin Vulkan frame with pending graphics "
                   "submissions");
        LOW_ASSERT(l_Frame.pending_presents.empty(),
                   "Cannot begin Vulkan frame with pending presents");
      }

      void acquire_swapchain(Detail::ContextImpl &p_Context,
                             const FrameContext &p_Frame,
                             SwapchainFrame &p_SwapchainFrame)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        LOW_ASSERT(l_State,
                   "Cannot acquire Vulkan swapchain without context "
                   "state");

        const u32 l_FrameIndex =
            Detail::FrameContextAccess::frame_index(p_Frame);
        LOW_ASSERT(l_FrameIndex < l_State->frames.size(),
                   "Cannot acquire Vulkan swapchain with invalid frame "
                   "index");

        const Swapchain l_FrameSwapchain =
            Detail::SwapchainFrameAccess::swapchain(
                p_SwapchainFrame);
        Detail::BackendSwapchain *l_BackendSwapchain =
            p_Context.swapchains.get(l_FrameSwapchain);
        LOW_ASSERT(l_BackendSwapchain,
                   "Cannot acquire invalid Vulkan swapchain");
        VulkanSwapchainState *l_Swapchain =
            static_cast<VulkanSwapchainState *>(
                l_BackendSwapchain->backend_state);
        LOW_ASSERT(l_Swapchain,
                   "Cannot acquire Vulkan swapchain without state");
        LOW_ASSERT(!l_Swapchain->acquired,
                   "Cannot acquire a Vulkan swapchain image while one "
                   "is already active");
        LOW_ASSERT(l_FrameIndex <
                       l_Swapchain->image_available_semaphores.size(),
                   "Cannot acquire Vulkan swapchain without frame "
                   "semaphore");

        FrameState &l_Frame = l_State->frames[l_FrameIndex];
        VkSemaphore l_ImageAvailable =
            l_Swapchain->image_available_semaphores[l_FrameIndex];

        u32 l_ImageIndex = LOW_UINT32_MAX;
        VkResult l_AcquireResult = vkAcquireNextImageKHR(
            l_State->device, l_Swapchain->swapchain, UINT64_MAX,
            l_ImageAvailable, VK_NULL_HANDLE, &l_ImageIndex);
        if (l_AcquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
          l_Swapchain->resize_requested = true;
          LOW_ASSERT(false, "Vulkan swapchain is out of date");
        }
        if (l_AcquireResult == VK_SUBOPTIMAL_KHR) {
          l_Swapchain->resize_requested = true;
        } else {
          LOW_ASSERT(l_AcquireResult == VK_SUCCESS,
                     "Failed to acquire Vulkan swapchain image");
        }

        LOW_ASSERT(l_ImageIndex < l_Swapchain->images.size(),
                   "Vulkan returned invalid swapchain image index");
        VulkanSwapchainImageState &l_Image =
            l_Swapchain->images[l_ImageIndex];

        if (l_Image.in_flight_fence != VK_NULL_HANDLE) {
          VkResult l_ImageWaitResult = vkWaitForFences(
              l_State->device, 1, &l_Image.in_flight_fence, VK_TRUE,
              UINT64_MAX);
          LOW_ASSERT(l_ImageWaitResult == VK_SUCCESS,
                     "Failed to wait for Vulkan swapchain image fence");
        }

        l_Image.in_flight_fence = l_Frame.frame_fence;
        l_Swapchain->acquired_image_index = l_ImageIndex;
        l_Swapchain->acquired = true;
        Detail::SwapchainFrameAccess::set_swapchain_image_index(
            p_SwapchainFrame, l_ImageIndex);
      }

      void present(Detail::ContextImpl &p_Context,
                   const SwapchainFrame &p_SwapchainFrame)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        LOW_ASSERT(l_State,
                   "Cannot present Vulkan swapchain without context "
                   "state");
        const u32 l_FrameIndex =
            Detail::SwapchainFrameAccess::frame_index(
                p_SwapchainFrame);
        const Swapchain l_FrameSwapchain =
            Detail::SwapchainFrameAccess::swapchain(
                p_SwapchainFrame);
        const u32 l_FrameImageIndex =
            Detail::SwapchainFrameAccess::swapchain_image_index(
                p_SwapchainFrame);

        LOW_ASSERT(l_FrameIndex < l_State->frames.size(),
                   "Cannot present Vulkan swapchain with invalid frame "
                   "index");

        Detail::BackendSwapchain *l_BackendSwapchain =
            p_Context.swapchains.get(l_FrameSwapchain);
        LOW_ASSERT(l_BackendSwapchain,
                   "Cannot present invalid Vulkan swapchain");
        VulkanSwapchainState *l_Swapchain =
            static_cast<VulkanSwapchainState *>(
                l_BackendSwapchain->backend_state);
        LOW_ASSERT(l_Swapchain,
                   "Cannot present Vulkan swapchain without state");
        LOW_ASSERT(l_Swapchain->acquired,
                   "Cannot present a Vulkan frame before image acquire");
        LOW_ASSERT(
            l_FrameImageIndex == l_Swapchain->acquired_image_index,
            "Cannot present a Vulkan frame with mismatched image index");

        FrameState &l_Frame = l_State->frames[l_FrameIndex];
        VulkanSwapchainImageState &l_Image =
            l_Swapchain->images[l_FrameImageIndex];

        VulkanPendingPresent l_Pending;
        l_Pending.swapchain = l_Swapchain;
        l_Pending.image = &l_Image;
        l_Pending.image_index = l_FrameImageIndex;
        l_Pending.image_available =
            l_Swapchain->image_available_semaphores[l_FrameIndex];
        l_Pending.present_transition_command =
            record_present_transition(*l_State, l_Frame, l_Image);
        l_Frame.pending_presents.push_back(l_Pending);
      }

      void end_frame(Detail::ContextImpl &p_Context,
                     const FrameContext &p_Frame)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        LOW_ASSERT(l_State,
                   "Cannot end Vulkan frame without context state");
        const u32 l_FrameIndex =
            Detail::FrameContextAccess::frame_index(p_Frame);

        LOW_ASSERT(l_FrameIndex < l_State->frames.size(),
                   "Cannot end Vulkan frame with invalid frame index");

        FrameState &l_Frame = l_State->frames[l_FrameIndex];

        Util::List<VkSemaphore> l_WaitSemaphores;
        Util::List<VkPipelineStageFlags> l_WaitStages;
        Util::List<VkSemaphore> l_SignalSemaphores;
        Util::List<VkCommandBuffer> l_CommandBuffers;

        for (VkCommandBuffer i_CommandBuffer :
             l_Frame.pending_graphics_submits) {
          LOW_ASSERT(i_CommandBuffer != VK_NULL_HANDLE,
                     "Cannot submit invalid pending Vulkan command "
                     "buffer");
          l_CommandBuffers.push_back(i_CommandBuffer);
        }

        for (VulkanPendingPresent &i_Pending :
             l_Frame.pending_presents) {
          LOW_ASSERT(i_Pending.swapchain && i_Pending.image,
                     "Cannot submit invalid pending Vulkan present");
          l_WaitSemaphores.push_back(i_Pending.image_available);
          l_WaitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
          l_SignalSemaphores.push_back(
              i_Pending.image->render_finished);
          if (i_Pending.present_transition_command !=
              VK_NULL_HANDLE) {
            l_CommandBuffers.push_back(
                i_Pending.present_transition_command);
          }
        }

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.waitSemaphoreCount =
            static_cast<u32>(l_WaitSemaphores.size());
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores.empty()
                                           ? nullptr
                                           : l_WaitSemaphores.data();
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages.empty()
                                             ? nullptr
                                             : l_WaitStages.data();
        l_SubmitInfo.commandBufferCount =
            static_cast<u32>(l_CommandBuffers.size());
        l_SubmitInfo.pCommandBuffers = l_CommandBuffers.empty()
                                           ? nullptr
                                           : l_CommandBuffers.data();
        l_SubmitInfo.signalSemaphoreCount =
            static_cast<u32>(l_SignalSemaphores.size());
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores.empty()
                                             ? nullptr
                                             : l_SignalSemaphores.data();

        VkResult l_ResetFenceResult =
            vkResetFences(l_State->device, 1, &l_Frame.frame_fence);
        LOW_ASSERT(l_ResetFenceResult == VK_SUCCESS,
                   "Failed to reset Vulkan frame fence");

        VkResult l_SubmitResult =
            vkQueueSubmit(l_State->graphics_queue.queue, 1,
                          &l_SubmitInfo, l_Frame.frame_fence);
        LOW_ASSERT(l_SubmitResult == VK_SUCCESS,
                   "Failed to submit Vulkan frame");

        for (VulkanPendingPresent &i_Pending :
             l_Frame.pending_presents) {
          i_Pending.image->layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

          VkSwapchainKHR l_Swapchain = i_Pending.swapchain->swapchain;
          VkSemaphore l_RenderFinished =
              i_Pending.image->render_finished;

          VkPresentInfoKHR l_PresentInfo{};
          l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
          l_PresentInfo.waitSemaphoreCount = 1;
          l_PresentInfo.pWaitSemaphores = &l_RenderFinished;
          l_PresentInfo.swapchainCount = 1;
          l_PresentInfo.pSwapchains = &l_Swapchain;
          l_PresentInfo.pImageIndices = &i_Pending.image_index;

          VkResult l_PresentResult = vkQueuePresentKHR(
              l_State->present_queue.queue, &l_PresentInfo);
          if (l_PresentResult == VK_ERROR_OUT_OF_DATE_KHR ||
              l_PresentResult == VK_SUBOPTIMAL_KHR) {
            i_Pending.swapchain->resize_requested = true;
          } else {
            LOW_ASSERT(l_PresentResult == VK_SUCCESS,
                       "Failed to present Vulkan frame");
          }

          i_Pending.swapchain->acquired_image_index =
              LOW_UINT32_MAX;
          i_Pending.swapchain->acquired = false;
        }

        l_Frame.pending_presents.clear();
        l_Frame.pending_graphics_submits.clear();
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
