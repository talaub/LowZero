#include "LowRendererEditorImageGpu.h"
#include "LowRendererVulkan.h"

#include "LowRendererVulkanBase.h"

#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVulkanPipeline.h"
#include "LowRendererVulkanDescriptor.h"
#include "LowRendererVulkanBuffer.h"
#include "LowRendererGlobals.h"
#include "LowRendererVulkan.h"
#include "LowRendererTexture.h"

#include "VkBootstrap.h"

#include "LowUtilResource.h"
#include "LowUtil.h"

#include "vk_mem_alloc.h"

#include "SDL.h"
#include "SDL_vulkan.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Renderer {
    namespace Vulkan {

      size_t
      StagingBuffer::request_space(const size_t p_RequestedSize,
                                   size_t *p_OutOffset)
      {
        const size_t l_AvailableSpace = size - occupied;

        *p_OutOffset = occupied;

        if (l_AvailableSpace >= p_RequestedSize) {
          occupied += p_RequestedSize;
          return p_RequestedSize;
        }

        occupied += l_AvailableSpace;
        return l_AvailableSpace;
      }

      bool StagingBuffer::write(void *p_Data, const size_t p_DataSize,
                                const size_t p_Offset)
      {
        // Converting the buffer to u8 so that it is easier for us to
        // offset the upload point into the buffer by p_Offset
        u8 *l_Buffer = (u8 *)buffer.info.pMappedData;

        memcpy((void *)&(l_Buffer[p_Offset]), p_Data, p_DataSize);

        return true;
      }

      namespace Global {
#ifdef LOW_RENDERER_VALIDATION_ENABLED
        const bool g_ValidationEnabled = true;
#else
        const bool g_ValidationEnabled = false;
#endif

#define RESOURCE_STAGING_BUFFER_SIZE (LOW_MEGABYTE_I * 32)
#define FRAME_STAGING_BUFFER_SIZE (LOW_MEGABYTE_I * 2)

        VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData)
        {
          // Print the validation message
          LOW_LOG_ERROR << "Validation Layer: "
                        << pCallbackData->pMessage << LOW_LOG_END;

          // Break on error or warning
          if (messageSeverity >=
              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
#ifdef _WIN32
            __debugbreak(); // Windows
                            // LOW_LOG_DEBUG << "Test" << LOW_LOG_END;
#else
            raise(SIGTRAP); // Unix-like
#endif
          }

          return VK_FALSE;
        }

        struct Frame
        {
          VkCommandPool commandPool;
          VkCommandBuffer mainCommandBuffer;

          StagingBuffer resourceStagingBuffer;
          VkDescriptorSet textureDescriptorSet;

          StagingBuffer frameStagingBuffer;

          Low::Util::Queue<TextureUpdate> textureUpdateQueue;
          Low::Util::Queue<EditorImageUpdate> editorImageUpdateQueue;
        };

        u32 g_FrameOverlap;
        u64 g_FrameNumber;
        u32 g_CurrentFrameIndex;
        Low::Util::List<Frame> g_Frames;

        VkInstance g_Instance;
        VkDebugUtilsMessengerEXT g_DebugMessenger;
        VkPhysicalDevice g_Gpu;
        VkDevice g_Device;
        vkb::Device g_VkbDevice;
        VkSurfaceKHR g_Surface;

        DynamicBuffer g_MeshVertexBuffer;
        DynamicBuffer g_MeshIndexBuffer;

        DynamicBuffer g_DrawCommandBuffer;

        AllocatedBuffer g_MaterialDataBuffer;

        VmaAllocator g_Allocator;

        VkQueue g_GraphicsQueue;
        u32 g_GraphicsQueueFamily;

        VkQueue g_TransferQueue;
        u32 g_TransferQueueFamily;

        Samplers g_Samplers;

        DescriptorUtil::DescriptorAllocatorGrowable
            g_GlobalDescriptorAllocatorGrowable;
        VkDescriptorPool g_ImguiPool;

        VkDescriptorSetLayout g_ViewInfoDescriptorSetLayout;
        VkDescriptorSetLayout g_GBufferDescriptorSetLayout;
        VkDescriptorSetLayout g_LightingDescriptorSetLayout;

        VkDescriptorSetLayout g_GlobalDescriptorSetLayout;
        VkDescriptorSet g_GlobalDescriptorSet;

        VkDescriptorSetLayout g_TextureDescriptorSetLayout;
        VkDescriptorSet *g_TextureDescriptorSets;

        VkPipelineLayout g_LightingPipelineLayout;

        VkPipelineLayout g_BlurPipelineLayout;

        struct BlurPushConstants
        {
          Math::Vector2 texelSize;
          u32 inputTextureIndex;
        };

        struct
        {
          Pipeline x_1;
          Pipeline y_1;
        } g_BlurPipelines;

        Low::Util::Queue<TextureUpdate> &
        get_texture_update_queue(u32 p_FrameIndex)
        {
          return g_Frames[p_FrameIndex].textureUpdateQueue;
        }

        Low::Util::Queue<EditorImageUpdate> &
        get_editor_image_update_queue(u32 p_FrameIndex)
        {
          return g_Frames[p_FrameIndex].editorImageUpdateQueue;
        }

        Low::Util::Queue<TextureUpdate> &
        get_current_texture_update_queue()
        {
          return get_texture_update_queue(get_current_frame_index());
        }

        Low::Util::Queue<EditorImageUpdate> &
        get_current_editor_image_update_queue()
        {
          return get_editor_image_update_queue(
              get_current_frame_index());
        }

        VkDescriptorSet get_texture_descriptor_set(u32 p_FrameIndex)
        {
          return g_Frames[p_FrameIndex].textureDescriptorSet;
        }

        VkDescriptorSet get_current_texture_descriptor_set()
        {
          return get_texture_descriptor_set(
              get_current_frame_index());
        }

        VkDescriptorSetLayout get_global_descriptor_set_layout()
        {
          return g_GlobalDescriptorSetLayout;
        }

        VkDescriptorSet get_global_descriptor_set()
        {
          return g_GlobalDescriptorSet;
        }

        VkDescriptorSetLayout get_texture_descriptor_set_layout()
        {
          return g_TextureDescriptorSetLayout;
        }

        Samplers &get_samplers()
        {
          return g_Samplers;
        }

        VkPipelineLayout get_lighting_pipeline_layout()
        {
          return g_LightingPipelineLayout;
        }

        VkDescriptorSetLayout get_gbuffer_descriptor_set_layout()
        {
          return g_GBufferDescriptorSetLayout;
        }
        VkDescriptorSetLayout get_view_info_descriptor_set_layout()
        {
          return g_ViewInfoDescriptorSetLayout;
        }
        u32 get_frame_overlap()
        {
          return g_FrameOverlap;
        }
        u64 get_frame_number()
        {
          return g_FrameNumber;
        }
        u32 get_current_frame_index()
        {
          return g_CurrentFrameIndex;
        }
        VkCommandBuffer get_current_command_buffer()
        {
          return g_Frames[g_CurrentFrameIndex].mainCommandBuffer;
        }
        VkCommandPool get_current_command_pool()
        {
          return g_Frames[g_CurrentFrameIndex].commandPool;
        }
        StagingBuffer &get_current_resource_staging_buffer()
        {
          return g_Frames[g_CurrentFrameIndex].resourceStagingBuffer;
        }
        StagingBuffer &get_current_frame_staging_buffer()
        {
          return g_Frames[g_CurrentFrameIndex].frameStagingBuffer;
        }
        DynamicBuffer &get_mesh_vertex_buffer()
        {
          return g_MeshVertexBuffer;
        }
        DynamicBuffer &get_mesh_index_buffer()
        {
          return g_MeshIndexBuffer;
        }
        VkFormat get_swapchain_format()
        {
          return VK_FORMAT_B8G8R8A8_UNORM;
        }
        DynamicBuffer &get_drawcommand_buffer()
        {
          return g_DrawCommandBuffer;
        }
        AllocatedBuffer get_material_data_buffer()
        {
          return g_MaterialDataBuffer;
        }

        VkInstance get_instance()
        {
          return g_Instance;
        }
        VkPhysicalDevice get_physical_device()
        {
          return g_Gpu;
        }
        VkDevice get_device()
        {
          return g_Device;
        }
        vkb::Device get_vkbdevice()
        {
          return g_VkbDevice;
        }
        VkSurfaceKHR get_surface()
        {
          return g_Surface;
        }
        VmaAllocator get_allocator()
        {
          return g_Allocator;
        }
        VkQueue get_graphics_queue()
        {
          return g_GraphicsQueue;
        }
        u32 get_graphics_queue_family()
        {
          return g_GraphicsQueueFamily;
        }
        VkQueue get_transfer_queue()
        {
          return g_TransferQueue;
        }
        u32 get_transfer_queue_family()
        {
          return g_TransferQueueFamily;
        }
        DescriptorUtil::DescriptorAllocatorGrowable &
        get_global_descriptor_allocator()
        {
          return g_GlobalDescriptorAllocatorGrowable;
        }

        bool advance_frame_count()
        {
          g_FrameNumber++;
          g_CurrentFrameIndex = g_FrameNumber % g_FrameOverlap;
          return true;
        }

        static bool initialize_global_descriptors()
        {
          {
            DescriptorUtil::DescriptorLayoutBuilder l_Builder;
            l_Builder.add_binding(0,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Builder.add_binding(1,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Builder.add_binding(2,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            g_GlobalDescriptorSetLayout = l_Builder.build(
                Global::get_device(), VK_SHADER_STAGE_ALL_GRAPHICS);
          }

          g_GlobalDescriptorSet =
              Global::get_global_descriptor_allocator().allocate(
                  Global::get_device(),
                  get_global_descriptor_set_layout());

          {
            DescriptorUtil::DescriptorWriter l_Writer;
            l_Writer.write_buffer(
                0, Global::get_mesh_vertex_buffer().m_Buffer.buffer,
                Global::get_mesh_vertex_buffer().m_ElementSize *
                    Global::get_mesh_vertex_buffer().m_ElementCount,
                0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            l_Writer.write_buffer(
                1, Global::get_drawcommand_buffer().m_Buffer.buffer,
                Global::get_drawcommand_buffer().m_ElementSize *
                    Global::get_drawcommand_buffer().m_ElementCount,
                0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            l_Writer.write_buffer(
                2, get_material_data_buffer().buffer,
                get_material_data_buffer().info.size, 0,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            l_Writer.update_set(Global::get_device(),
                                get_global_descriptor_set());
          }

          {
            DescriptorUtil::DescriptorLayoutBuilder l_Builder;
            l_Builder.add_binding(
                0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                GpuTexture::get_capacity());
            l_Builder.add_binding(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                EditorImageGpu::get_capacity());
            g_TextureDescriptorSetLayout = l_Builder.build(
                Global::get_device(), VK_SHADER_STAGE_ALL_GRAPHICS);
          }

          return true;
        }

        bool imgui_init()
        {
          VkDescriptorPoolSize l_PoolSizes[] = {
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

          VkDescriptorPoolCreateInfo l_PoolInfo = {};
          l_PoolInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
          l_PoolInfo.flags =
              VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
          l_PoolInfo.maxSets = 1000;
          l_PoolInfo.poolSizeCount = (uint32_t)std::size(l_PoolSizes);
          l_PoolInfo.pPoolSizes = l_PoolSizes;

          LOWR_VK_CHECK_RETURN(vkCreateDescriptorPool(
              g_Device, &l_PoolInfo, nullptr, &g_ImguiPool));

          ImGui::CreateContext();

          {
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.IniFilename = "loweditor.ini";
            io.ConfigFlags |=
                ImGuiConfigFlags_DockingEnable; // Enable Docking
            io.ConfigFlags |=
                ImGuiConfigFlags_ViewportsEnable; // Enable
                                                  // Multi-Viewport /
          }

          ImGui_ImplSDL2_InitForVulkan(
              Util::Window::get_main_window().sdlwindow);

          // this initializes imgui for Vulkan
          ImGui_ImplVulkan_InitInfo l_InitInfo = {};
          l_InitInfo.Instance = g_Instance;
          l_InitInfo.PhysicalDevice = g_Gpu;
          l_InitInfo.Device = g_Device;
          l_InitInfo.Queue = g_GraphicsQueue;
          l_InitInfo.DescriptorPool = g_ImguiPool;
          l_InitInfo.MinImageCount = 3;
          l_InitInfo.ImageCount = 3;
          l_InitInfo.UseDynamicRendering = true;

          VkFormat l_Formats[] = {get_swapchain_format()};

          // dynamic rendering parameters for imgui to use
          l_InitInfo.PipelineRenderingCreateInfo.sType =
              VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
          l_InitInfo.PipelineRenderingCreateInfo
              .colorAttachmentCount = 1;
          l_InitInfo.PipelineRenderingCreateInfo
              .pColorAttachmentFormats = l_Formats;

          l_InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

          ImGui_ImplVulkan_Init(&l_InitInfo);

          // ImGui_ImplVulkan_CreateFontsTexture();

          return true;
        }

        static bool
        dynamic_buffer_init(DynamicBuffer &p_DynamicBuffer,
                            u32 p_ElementSize, u32 p_ElementCount,
                            VkBufferUsageFlags p_Usage,
                            VmaMemoryUsage p_MemoryUsage)
        {
          p_DynamicBuffer.m_ElementSize = p_ElementSize;
          p_DynamicBuffer.m_ElementCount = p_ElementCount;

          DynamicBufferFreeSlot l_Slot;
          l_Slot.start = 0;
          l_Slot.length = p_ElementCount;
          p_DynamicBuffer.m_FreeSlots.push_back(l_Slot);

          p_DynamicBuffer.m_Buffer = BufferUtil::create_buffer(
              p_ElementSize * p_ElementCount, p_Usage, p_MemoryUsage);

          return true;
        }

        static bool resource_buffers_init()
        {
          LOW_ASSERT_ERROR_RETURN_FALSE(
              dynamic_buffer_init(
                  g_MeshVertexBuffer, sizeof(Util::Resource::Vertex),
                  125000,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                  VMA_MEMORY_USAGE_GPU_ONLY),
              "Could not initialize mesh vertex resource buffer");

          LOW_ASSERT_ERROR_RETURN_FALSE(
              dynamic_buffer_init(
                  g_MeshIndexBuffer, sizeof(u32), 500000,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                  VMA_MEMORY_USAGE_GPU_ONLY),
              "Could not initialize mesh index resource buffer");

          return true;
        }

        static bool buffers_init()
        {
          LOW_ASSERT_ERROR_RETURN_FALSE(
              dynamic_buffer_init(
                  g_DrawCommandBuffer, sizeof(DrawCommandUpload),
                  10000,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                  VMA_MEMORY_USAGE_GPU_ONLY),
              "Could not initialize draw command buffer");

          g_MaterialDataBuffer = BufferUtil::create_buffer(
              MATERIAL_DATA_SIZE * 1000,
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VMA_MEMORY_USAGE_GPU_ONLY);

          return true;
        }

        static bool allocator_init()
        {
          VmaAllocatorCreateInfo l_AllocatorInfo = {};
          l_AllocatorInfo.physicalDevice = g_Gpu;
          l_AllocatorInfo.device = g_Device;
          l_AllocatorInfo.instance = g_Instance;
          l_AllocatorInfo.flags =
              VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
          vmaCreateAllocator(&l_AllocatorInfo, &g_Allocator);

          return true;
        }

        static bool graphics_queue_init()
        {
          g_GraphicsQueue =
              g_VkbDevice.get_queue(vkb::QueueType::graphics).value();
          g_GraphicsQueueFamily =
              g_VkbDevice.get_queue_index(vkb::QueueType::graphics)
                  .value();

          return true;
        }

        static bool transfer_queue_init()
        {
          g_TransferQueue =
              g_VkbDevice.get_queue(vkb::QueueType::transfer).value();
          g_TransferQueueFamily =
              g_VkbDevice.get_queue_index(vkb::QueueType::transfer)
                  .value();

          return true;
        }

        static bool blur_pipelines_init()
        {
          Util::List<VkDescriptorSetLayout> l_DescriptorSetLayouts;
          l_DescriptorSetLayouts.push_back(
              get_global_descriptor_set_layout());
          l_DescriptorSetLayouts.push_back(
              get_texture_descriptor_set_layout());

          VkPipelineLayoutCreateInfo l_Layout{};
          l_Layout.sType =
              VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
          l_Layout.pNext = nullptr;
          l_Layout.pSetLayouts = l_DescriptorSetLayouts.data();
          l_Layout.setLayoutCount = l_DescriptorSetLayouts.size();

          VkPushConstantRange l_PushConstant{};
          l_PushConstant.offset = 0;
          l_PushConstant.size = sizeof(BlurPushConstants);
          l_PushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

          l_Layout.pPushConstantRanges = &l_PushConstant;
          l_Layout.pushConstantRangeCount = 1;

          LOWR_VK_CHECK_RETURN(
              vkCreatePipelineLayout(get_device(), &l_Layout, nullptr,
                                     &g_BlurPipelineLayout));

          {
            g_BlurPipelines.x_1 =
                PipelineUtil::GraphicsPipelineBuilder::
                    prepare_fullscreen_effect(g_BlurPipelineLayout,
                                              "blur_x_1.frag",
                                              VK_FORMAT_R8_UNORM)
                        .register_pipeline();
            g_BlurPipelines.y_1 =
                PipelineUtil::GraphicsPipelineBuilder::
                    prepare_fullscreen_effect(g_BlurPipelineLayout,
                                              "blur_y_1.frag",
                                              VK_FORMAT_R8_UNORM)
                        .register_pipeline();
          }
          return true;
        }

        static bool samplers_init()
        {
          {
            VkSamplerCreateInfo sampl{};
            sampl.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

            sampl.magFilter = VK_FILTER_NEAREST;
            sampl.minFilter = VK_FILTER_NEAREST;
            sampl.mipmapMode =
                VK_SAMPLER_MIPMAP_MODE_LINEAR; // Enable mipmaps

            sampl.addressModeU =
                VK_SAMPLER_ADDRESS_MODE_REPEAT; // Addressing mode
            sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            sampl.mipLodBias = 0.0f; // LOD bias for mip selection
            sampl.minLod = 0.0f;     // Minimum LOD level
            sampl.maxLod = 0.0f;     // Max LOD level
            sampl.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

            vkCreateSampler(
                Low::Renderer::Vulkan::Global::get_device(), &sampl,
                nullptr, &g_Samplers.no_lod_nearest_repeat_black);
          }
          {
            VkSamplerCreateInfo sampl{};
            sampl.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

            sampl.magFilter = VK_FILTER_NEAREST;
            sampl.minFilter = VK_FILTER_NEAREST;
            sampl.mipmapMode =
                VK_SAMPLER_MIPMAP_MODE_LINEAR; // Enable mipmaps

            sampl.addressModeU =
                VK_SAMPLER_ADDRESS_MODE_REPEAT; // Addressing mode
            sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            sampl.mipLodBias = 0.0f; // LOD bias for mip selection
            sampl.minLod = 0.0f;     // Minimum LOD level
            sampl.maxLod = 0.0f;     // Max LOD level
            sampl.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;

            vkCreateSampler(
                Low::Renderer::Vulkan::Global::get_device(), &sampl,
                nullptr, &g_Samplers.no_lod_nearest_repeat_white);
          }

          {
            g_Samplers.lod_nearest_repeat_black.resize(
                IMAGE_MIPMAP_COUNT);
            for (int i = 0; i < IMAGE_MIPMAP_COUNT; ++i) {
              VkSamplerCreateInfo sampl{};
              sampl.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

              sampl.magFilter = VK_FILTER_NEAREST;
              sampl.minFilter = VK_FILTER_NEAREST;
              sampl.mipmapMode =
                  VK_SAMPLER_MIPMAP_MODE_LINEAR; // Enable mipmaps

              sampl.addressModeU =
                  VK_SAMPLER_ADDRESS_MODE_REPEAT; // Addressing mode
              sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
              sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

              sampl.mipLodBias = 0.0f; // LOD bias for mip selection
              sampl.minLod =
                  static_cast<float>(i); // Minimum LOD level
              sampl.maxLod = static_cast<float>(IMAGE_MIPMAP_COUNT -
                                                1); // Max LOD level
              sampl.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

              vkCreateSampler(
                  Low::Renderer::Vulkan::Global::get_device(), &sampl,
                  nullptr, &g_Samplers.lod_nearest_repeat_black[i]);
            }
          }
          {
            g_Samplers.lod_linear_repeat_black.resize(
                IMAGE_MIPMAP_COUNT);
            for (int i = 0; i < IMAGE_MIPMAP_COUNT; ++i) {
              VkSamplerCreateInfo sampl{};
              sampl.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

              sampl.magFilter = VK_FILTER_LINEAR;
              sampl.minFilter = VK_FILTER_LINEAR;
              sampl.mipmapMode =
                  VK_SAMPLER_MIPMAP_MODE_LINEAR; // Enable mipmaps

              sampl.addressModeU =
                  VK_SAMPLER_ADDRESS_MODE_REPEAT; // Addressing mode
              sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
              sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

              sampl.mipLodBias = 0.0f; // LOD bias for mip selection
              sampl.minLod =
                  static_cast<float>(i); // Minimum LOD level
              sampl.maxLod = static_cast<float>(IMAGE_MIPMAP_COUNT -
                                                1); // Max LOD level
              sampl.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

              vkCreateSampler(
                  Low::Renderer::Vulkan::Global::get_device(), &sampl,
                  nullptr, &g_Samplers.lod_linear_repeat_black[i]);
            }
          }

          return true;
        }

        static bool frames_init()
        {
          LOW_LOG_DEBUG << "Setting resource staging buffer size to "
                        << RESOURCE_STAGING_BUFFER_SIZE
                        << LOW_LOG_END;

          g_FrameOverlap = VK_FRAME_OVERLAP;
          g_CurrentFrameIndex = 0;
          g_FrameNumber = 0;

          VkCommandPoolCreateInfo l_CommandPoolInfo =
              InitUtil::command_pool_create_info(
                  Global::get_graphics_queue_family(),
                  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

          g_Frames.resize(g_FrameOverlap);

          for (u32 i = 0; i < g_FrameOverlap; ++i) {
            LOWR_VK_CHECK_RETURN(vkCreateCommandPool(
                Global::get_device(), &l_CommandPoolInfo, nullptr,
                &g_Frames[i].commandPool));

            VkCommandBufferAllocateInfo l_CmdAllocInfo =
                InitUtil::command_buffer_allocate_info(
                    g_Frames[i].commandPool);

            LOWR_VK_CHECK_RETURN(vkAllocateCommandBuffers(
                Global::get_device(), &l_CmdAllocInfo,
                &g_Frames[i].mainCommandBuffer));

            {
              g_Frames[i].resourceStagingBuffer.buffer =
                  BufferUtil::create_buffer(
                      RESOURCE_STAGING_BUFFER_SIZE,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VMA_MEMORY_USAGE_CPU_TO_GPU);
              g_Frames[i].resourceStagingBuffer.size =
                  RESOURCE_STAGING_BUFFER_SIZE;
              g_Frames[i].resourceStagingBuffer.occupied = 0u;
            }

            {
              g_Frames[i].frameStagingBuffer.buffer =
                  BufferUtil::create_buffer(
                      FRAME_STAGING_BUFFER_SIZE,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VMA_MEMORY_USAGE_CPU_TO_GPU);
              g_Frames[i].frameStagingBuffer.size =
                  FRAME_STAGING_BUFFER_SIZE;
              g_Frames[i].frameStagingBuffer.occupied = 0u;
            }

            {
              g_Frames[i].textureDescriptorSet =
                  Global::get_global_descriptor_allocator().allocate(
                      Global::get_device(),
                      g_TextureDescriptorSetLayout);
            }
          }
        }

        bool initialize()
        {
          vkb::InstanceBuilder l_InstanceBuilder;

          vkb::Result<vkb::Instance> l_InstanceReturn =
              l_InstanceBuilder.set_app_name("LowEngine")
                  .request_validation_layers(g_ValidationEnabled)
                  //.use_default_debug_messenger()
                  .set_debug_callback(&DebugCallback)
                  .require_api_version(1, 3, 0)
                  .build();

          if (!l_InstanceReturn) {
            LOW_LOG_ERROR << l_InstanceReturn.full_error()
                                 .type.message()
                                 .c_str()
                          << LOW_LOG_END;
          }
          LOWR_VK_ASSERT(l_InstanceReturn,
                         "Failed to initialize vulkan instance");
          vkb::Instance l_VkbInstance = l_InstanceReturn.value();

          g_Instance = l_VkbInstance.instance;
          g_DebugMessenger = l_VkbInstance.debug_messenger;
          // End initialize instance

          // Begin initialize surface
          SDL_Vulkan_CreateSurface(Util::Window::get_main_window()
                                       .get_main_window()
                                       .sdlwindow,
                                   g_Instance, &g_Surface);
          // End initialize surface

          // Begin initialize gpu
          // vulkan 1.3 features
          VkPhysicalDeviceVulkan13Features l_Features{
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
          l_Features.dynamicRendering = true;
          l_Features.synchronization2 = true;

          // vulkan 1.2 features
          VkPhysicalDeviceVulkan12Features l_Features12{
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
          l_Features12.bufferDeviceAddress = true;
          l_Features12.descriptorIndexing = true;

          VkPhysicalDeviceFeatures l_ExtensionFeatures{};
          l_ExtensionFeatures.fillModeNonSolid = VK_TRUE;
          l_ExtensionFeatures.wideLines = VK_TRUE;

          // use vkbootstrap to select a gpu.
          // We want a gpu that can write to the SDL surface and
          // supports vulkan 1.3 with the correct features
          vkb::PhysicalDeviceSelector l_GpuSelector{l_VkbInstance};
          vkb::PhysicalDevice l_VkbGpu =
              l_GpuSelector.set_minimum_version(1, 3)
                  .set_required_features_13(l_Features)
                  .set_required_features_12(l_Features12)
                  .set_required_features(l_ExtensionFeatures)
                  .set_surface(g_Surface)
                  .select()
                  .value();

          g_Gpu = l_VkbGpu.physical_device;
          // End initialize gpu

          // Begin initialize device
          vkb::DeviceBuilder l_DeviceBuilder{l_VkbGpu};
          g_VkbDevice = l_DeviceBuilder.build().value();

          g_Device = g_VkbDevice.device;
          // End initialize device

          LOWR_VK_ASSERT(allocator_init(),
                         "Could not initialize allocator");

          LOWR_VK_ASSERT(graphics_queue_init(),
                         "Could not initialize graphics queue");
          LOWR_VK_ASSERT(transfer_queue_init(),
                         "Could not initialize transfer queue");

          LOWR_VK_ASSERT(buffers_init(),
                         "Could not initialize buffers");

          LOWR_VK_ASSERT(resource_buffers_init(),
                         "Could not initialize resource buffers");

          LOWR_VK_ASSERT(imgui_init(), "Could not initialize imgui");

          LOWR_VK_ASSERT(samplers_init(),
                         "Could not initialize global samplers");

          Util::List<DescriptorUtil::PoolSizeRatio> l_Sizes = {
              {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 30},
              {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 50},
              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
              {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}};

          g_GlobalDescriptorAllocatorGrowable.init(g_Device, 10,
                                                   l_Sizes);

          LOWR_VK_ASSERT(initialize_global_descriptors(),
                         "Could not initialize global descriptors");

          LOWR_VK_ASSERT(frames_init(),
                         "Could not initialize frames");

          {
            DescriptorUtil::DescriptorLayoutBuilder l_Builder;
            l_Builder.add_binding(0,
                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            l_Builder.add_binding(1,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Builder.add_binding(2,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Builder.add_binding(3,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Builder.add_binding(4,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Builder.add_binding(5,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Builder.add_binding(6,
                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            g_ViewInfoDescriptorSetLayout =
                l_Builder.build(Global::get_device(),
                                VK_SHADER_STAGE_ALL_GRAPHICS |
                                    VK_SHADER_STAGE_COMPUTE_BIT);
          }

          {
            DescriptorUtil::DescriptorLayoutBuilder l_Builder;
            l_Builder.add_binding(
                0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            l_Builder.add_binding(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            g_GBufferDescriptorSetLayout = l_Builder.build(
                Global::get_device(), VK_SHADER_STAGE_ALL);
          }

          {
            Util::List<VkDescriptorSetLayout> l_DescriptorSetLayouts;
            l_DescriptorSetLayouts.push_back(
                get_global_descriptor_set_layout());
            l_DescriptorSetLayouts.push_back(
                get_texture_descriptor_set_layout());
            l_DescriptorSetLayouts.push_back(
                get_view_info_descriptor_set_layout());

            VkPipelineLayoutCreateInfo l_Layout{};
            l_Layout.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            l_Layout.pNext = nullptr;
            l_Layout.pSetLayouts = l_DescriptorSetLayouts.data();
            l_Layout.setLayoutCount = l_DescriptorSetLayouts.size();

            LOWR_VK_CHECK_RETURN(vkCreatePipelineLayout(
                Global::get_device(), &l_Layout, nullptr,
                &g_LightingPipelineLayout));
          }

          LOWR_VK_ASSERT(blur_pipelines_init(),
                         "Could not initialize blur pipelines");

          return true;
        }

        static bool frames_cleanup()
        {
          for (u32 i = 0; i < g_FrameOverlap; ++i) {
            vkFreeCommandBuffers(Global::get_device(),
                                 g_Frames[i].commandPool, 1,
                                 &g_Frames[i].mainCommandBuffer);

            vkDestroyCommandPool(Global::get_device(),
                                 g_Frames[i].commandPool, nullptr);

            BufferUtil::destroy_buffer(
                g_Frames[i].resourceStagingBuffer.buffer);
            g_Frames[i].resourceStagingBuffer.size = 0;

            BufferUtil::destroy_buffer(
                g_Frames[i].frameStagingBuffer.buffer);
            g_Frames[i].frameStagingBuffer.size = 0;
          }

          g_Frames.clear();

          return true;
        }

        static bool buffer_cleanup()
        {
          get_drawcommand_buffer().destroy();
          BufferUtil::destroy_buffer(get_material_data_buffer());

          return true;
        }

        static bool global_descriptors_cleanup()
        {
          vkDestroyDescriptorSetLayout(
              Global::get_device(),
              Global::get_global_descriptor_set_layout(), nullptr);

          return true;
        }

        static bool mesh_buffer_cleanup()
        {
          get_mesh_vertex_buffer().destroy();
          get_mesh_index_buffer().destroy();

          return true;
        }

        bool cleanup()
        {
          ImGui_ImplVulkan_Shutdown();
          vkDestroyDescriptorPool(g_Device, g_ImguiPool, nullptr);

          vkDestroyDescriptorSetLayout(
              g_Device, g_GBufferDescriptorSetLayout, nullptr);
          vkDestroyDescriptorSetLayout(
              g_Device, g_ViewInfoDescriptorSetLayout, nullptr);

          LOWR_VK_ASSERT(global_descriptors_cleanup(),
                         "Could not cleanup global descriptors");
          LOWR_VK_ASSERT(frames_cleanup(),
                         "Could not cleanup frames");
          LOWR_VK_ASSERT(buffer_cleanup(),
                         "Could not cleanup buffers");
          LOWR_VK_ASSERT(mesh_buffer_cleanup(),
                         "Could not cleanup mesh buffers");

          vkDestroySurfaceKHR(g_Instance, g_Surface, nullptr);

          vmaDestroyAllocator(g_Allocator);

          vkDestroyDevice(g_Device, nullptr);

          vkb::destroy_debug_utils_messenger(g_Instance,
                                             g_DebugMessenger);
          vkDestroyInstance(g_Instance, nullptr);

          return true;
        }

        bool blur_image_1(Texture p_ImageToBlur, Texture p_TempImage,
                          Texture p_OutImage,
                          Math::UVector2 p_Dimensions)
        {
          VkCommandBuffer l_Cmd = get_current_command_buffer();

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}};

          Math::Vector2 l_InverseDimensions;
          l_InverseDimensions.x = 1.0f / float(p_Dimensions.x);
          l_InverseDimensions.y = 1.0f / float(p_Dimensions.y);

          Util::List<VkDescriptorSet> l_DescriptorSets;
          l_DescriptorSets.resize(2);
          l_DescriptorSets[0] = get_global_descriptor_set();
          l_DescriptorSets[1] = get_current_texture_descriptor_set();

          VkViewport l_Viewport = {};
          l_Viewport.x = 0;
          l_Viewport.y = 0;
          l_Viewport.width = static_cast<float>(p_Dimensions.x);
          l_Viewport.height = static_cast<float>(p_Dimensions.y);
          l_Viewport.minDepth = 0.f;
          l_Viewport.maxDepth = 1.f;

          VkRect2D l_Scissor = {};
          l_Scissor.offset.x = 0;
          l_Scissor.offset.y = 0;
          l_Scissor.extent.width = p_Dimensions.x;
          l_Scissor.extent.height = p_Dimensions.y;

          Samplers &l_Samplers = get_samplers();

          Image l_TempImage = p_TempImage.get_gpu().get_data_handle();
          Image l_ImageToBlur =
              p_ImageToBlur.get_gpu().get_data_handle();
          Image l_OutImage = p_OutImage.get_gpu().get_data_handle();

          {
            ImageUtil::cmd_transition(
                l_Cmd, l_TempImage,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
            l_ColorAttachments.resize(1);
            l_ColorAttachments[0] = InitUtil::attachment_info(
                l_TempImage.get_allocated_image().imageView,
                &l_ClearColorValue,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
                {p_Dimensions.x, p_Dimensions.y},
                l_ColorAttachments.data(), l_ColorAttachments.size(),
                nullptr);
            vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

            vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              g_BlurPipelines.x_1.get_pipeline());

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_BlurPipelineLayout, 0, l_DescriptorSets.size(),
                l_DescriptorSets.data(), 0, nullptr);

            BlurPushConstants l_PushConstants;
            l_PushConstants.texelSize = l_InverseDimensions;
            l_PushConstants.inputTextureIndex =
                p_ImageToBlur.get_gpu().get_index();

            vkCmdPushConstants(l_Cmd, g_BlurPipelineLayout,
                               VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(BlurPushConstants),
                               &l_PushConstants);

            vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

            vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

            vkCmdDraw(l_Cmd, 3, 1, 0, 0);

            vkCmdEndRendering(l_Cmd);

            ImageUtil::cmd_transition(
                l_Cmd, l_TempImage,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }

          {
            ImageUtil::cmd_transition(
                l_Cmd, l_OutImage,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
            l_ColorAttachments.resize(1);
            l_ColorAttachments[0] = InitUtil::attachment_info(
                l_OutImage.get_allocated_image().imageView,
                &l_ClearColorValue,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
                {p_Dimensions.x, p_Dimensions.y},
                l_ColorAttachments.data(), l_ColorAttachments.size(),
                nullptr);
            vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

            vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              g_BlurPipelines.y_1.get_pipeline());

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_BlurPipelineLayout, 0, l_DescriptorSets.size(),
                l_DescriptorSets.data(), 0, nullptr);

            BlurPushConstants l_PushConstants;
            l_PushConstants.texelSize = l_InverseDimensions;
            l_PushConstants.inputTextureIndex =
                p_TempImage.get_gpu().get_index();

            vkCmdPushConstants(l_Cmd, g_BlurPipelineLayout,
                               VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(BlurPushConstants),
                               &l_PushConstants);

            vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

            vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

            vkCmdDraw(l_Cmd, 3, 1, 0, 0);

            vkCmdEndRendering(l_Cmd);

            ImageUtil::cmd_transition(
                l_Cmd, l_OutImage,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }

          return true;
        }

        void renderdoc_section_begin(Util::String p_SectionLabel,
                                     Math::ColorRGB p_Color)
        {
          VkCommandBuffer l_CommandBuffer =
              get_current_command_buffer();

          VkDebugUtilsLabelEXT markerInfo = {};
          markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
          markerInfo.pLabelName = p_SectionLabel.c_str();
          markerInfo.color[0] = p_Color.r;
          markerInfo.color[1] = p_Color.g;
          markerInfo.color[2] = p_Color.b;
          markerInfo.color[3] = 1.0f;

          auto l_Function =
              (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
                  get_instance(), "vkCmdBeginDebugUtilsLabelEXT");

          if (l_Function != nullptr) {
            l_Function(l_CommandBuffer, &markerInfo);
          }
        }

        void renderdoc_section_end()
        {
          VkCommandBuffer l_CommandBuffer =
              get_current_command_buffer();

          auto l_Function =
              (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
                  get_instance(), "vkCmdEndDebugUtilsLabelEXT");

          if (l_Function != nullptr) {
            l_Function(l_CommandBuffer);
          }
        }
      } // namespace Global
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
