#include "LowGfxVulkanBackend.h"

namespace Low {
  namespace Gfx {
    namespace Vulkan {
      const Detail::BackendProvider &get_backend_provider()
      {
        static const Detail::InstanceBackendApi g_InstanceApi = {
            &create_instance, &destroy_instance, &enumerate_adapters,
            &create_surface,  &destroy_surface,  &select_adapter};

        static const Detail::ContextBackendApi g_ContextApi = {
            &create_context,
            &destroy_context,
            &get_caps,
            &wait_idle,
            &create_buffer,
            &destroy_buffer,
            &create_image,
            &destroy_image,
            &create_image_view,
            &destroy_image_view,
            &create_sampler,
            &destroy_sampler,
            &create_shader_module,
            &destroy_shader_module,
            &create_bind_group_layout,
            &destroy_bind_group_layout,
            &create_pipeline_layout,
            &destroy_pipeline_layout,
            &create_bind_group,
            &update_bind_group,
            &destroy_bind_group,
            &create_graphics_pipeline,
            &destroy_graphics_pipeline,
            &create_compute_pipeline,
            &destroy_compute_pipeline,
            &request_command_list,
            &request_immediate_command_list,
            &destroy_command_list,
            &create_swapchain,
            &destroy_swapchain,
            &begin_frame,
            &acquire_swapchain,
            &present,
            &end_frame,
            &begin_command_list,
            &end_command_list,
            &submit_command_list,
            &barrier_image_command_list,
            &begin_dynamic_rendering,
            &end_dynamic_rendering,
            &set_viewport,
            &set_scissor,
            &bind_graphics_pipeline,
            &bind_compute_pipeline,
            &bind_bind_group,
            &bind_vertex_buffer,
            &bind_index_buffer,
            &draw,
            &draw_indexed,
            &dispatch};

        static const Detail::BackendProvider g_Provider = {
            Backend::Vulkan, &g_InstanceApi, &g_ContextApi};

        return g_Provider;
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
