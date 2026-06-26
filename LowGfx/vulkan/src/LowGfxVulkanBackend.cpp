#include "LowGfxVulkanBackend.h"

namespace Low {
  namespace Gfx {
    namespace Vulkan {
      const Detail::BackendProvider &get_backend_provider()
      {
        static const Detail::InstanceBackendApi g_InstanceApi = {
            &create_instance,
            &destroy_instance,
            &enumerate_adapters,
            &create_surface,
            &destroy_surface,
            &select_adapter};

        static const Detail::ContextBackendApi g_ContextApi = {
            &create_context,
            &destroy_context,
            &get_caps,
            &create_buffer,
            &destroy_buffer,
            &request_command_list,
            &request_immediate_command_list,
            &destroy_command_list,
            &create_swapchain,
            &destroy_swapchain,
            &begin_frame,
            &acquire_swapchain,
            &present,
            &end_frame};

        static const Detail::BackendProvider g_Provider = {
            Backend::Vulkan, &g_InstanceApi, &g_ContextApi};

        return g_Provider;
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
