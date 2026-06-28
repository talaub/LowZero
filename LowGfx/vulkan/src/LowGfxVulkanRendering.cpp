#include "LowGfxVulkanBackend.h"
#include "LowGfxVulkanState.h"
#include "LowUtilAssert.h"
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Gfx {
    namespace Vulkan {
      static VkAttachmentLoadOp to_vulkan_load_op(LoadOp p_LoadOp)
      {
        switch (p_LoadOp) {
        case LoadOp::Load:
          return VK_ATTACHMENT_LOAD_OP_LOAD;
        case LoadOp::Clear:
          return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case LoadOp::DontCare:
          return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }

        LOW_ASSERT(false, "Unsupported LowGfx load op");
        return VK_ATTACHMENT_LOAD_OP_LOAD;
      }

      static VkAttachmentStoreOp to_vulkan_store_op(StoreOp p_StoreOp)
      {
        switch (p_StoreOp) {
        case StoreOp::Store:
          return VK_ATTACHMENT_STORE_OP_STORE;
        case StoreOp::DontCare:
          return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        LOW_ASSERT(false, "Unsupported LowGfx store op");
        return VK_ATTACHMENT_STORE_OP_STORE;
      }

      static VkImageLayout
      to_vulkan_rendering_layout(ImageState p_State)
      {
        switch (p_State) {
        case ImageState::ColorAttachment:
          return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ImageState::DepthWrite:
          return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        case ImageState::DepthRead:
          return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
        default:
          break;
        }

        LOW_ASSERT(false,
                   "Unsupported LowGfx image state for rendering");
        return VK_IMAGE_LAYOUT_UNDEFINED;
      }

      static VkClearValue
      to_vulkan_clear_value(const ClearValue &p_ClearValue)
      {
        VkClearValue l_ClearValue{};
        l_ClearValue.color.float32[0] = p_ClearValue.color[0];
        l_ClearValue.color.float32[1] = p_ClearValue.color[1];
        l_ClearValue.color.float32[2] = p_ClearValue.color[2];
        l_ClearValue.color.float32[3] = p_ClearValue.color[3];
        l_ClearValue.depthStencil.depth = p_ClearValue.depth;
        l_ClearValue.depthStencil.stencil = p_ClearValue.stencil;
        return l_ClearValue;
      }

      static VkRenderingAttachmentInfo
      make_attachment_info(VkImageView p_ImageView,
                           ImageState p_State, LoadOp p_LoadOp,
                           StoreOp p_StoreOp,
                           const ClearValue &p_ClearValue)
      {
        VkRenderingAttachmentInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        l_Info.pNext = nullptr;
        l_Info.imageView = p_ImageView;
        l_Info.imageLayout = to_vulkan_rendering_layout(p_State);
        l_Info.loadOp = to_vulkan_load_op(p_LoadOp);
        l_Info.storeOp = to_vulkan_store_op(p_StoreOp);
        l_Info.clearValue = to_vulkan_clear_value(p_ClearValue);
        return l_Info;
      }

      static VulkanImageViewState *
      get_vulkan_image_view_state(Detail::ContextImpl &p_Context,
                                  ImageView p_ImageView)
      {
        Detail::BackendImageView *l_ImageView =
            p_Context.image_views.get(p_ImageView);
        LOW_ASSERT(l_ImageView,
                   "Cannot use invalid image view for Vulkan "
                   "rendering attachment");

        VulkanImageViewState *l_ImageViewState =
            static_cast<VulkanImageViewState *>(
                l_ImageView->backend_state);
        LOW_ASSERT(l_ImageViewState &&
                       l_ImageViewState->image_view != VK_NULL_HANDLE,
                   "Cannot use empty Vulkan image view for rendering "
                   "attachment");
        return l_ImageViewState;
      }

      void begin_dynamic_rendering(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          const RenderingInfo &p_RenderingInfo)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(l_State,
                   "Cannot begin rendering on Vulkan command list "
                   "without context state");
        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        LOW_ASSERT(l_CommandListState,
                   "Cannot begin rendering on Vulkan command list "
                   "without command list state");
        LOW_ASSERT(l_CommandListState->command_buffer !=
                       VK_NULL_HANDLE,
                   "Cannot begin rendering without Vulkan command "
                   "buffer");

        VkExtent2D l_RenderExtent;
        l_RenderExtent.width = p_RenderingInfo.extent.x;
        l_RenderExtent.height = p_RenderingInfo.extent.y;

        Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
        l_ColorAttachments.resize(
            p_RenderingInfo.color_attachments.size());

        for (int i = 0; i < p_RenderingInfo.color_attachments.size();
             ++i) {
          const ColorAttachmentDesc &i_Attachment =
              p_RenderingInfo.color_attachments[i];
          VulkanImageViewState *i_ImageView =
              get_vulkan_image_view_state(p_Context,
                                          i_Attachment.view);

          l_ColorAttachments[i] = make_attachment_info(
              i_ImageView->image_view, i_Attachment.state,
              i_Attachment.load_op, i_Attachment.store_op,
              i_Attachment.clear);
        }

        VkRenderingAttachmentInfo l_DepthAttachment{};
        VkRenderingAttachmentInfo *l_DepthAttachmentPtr = nullptr;
        if (p_RenderingInfo.depth_attachment) {
          VulkanImageViewState *l_ImageView =
              get_vulkan_image_view_state(
                  p_Context, p_RenderingInfo.depth_attachment->view);

          l_DepthAttachment = make_attachment_info(
              l_ImageView->image_view,
              p_RenderingInfo.depth_attachment->state,
              p_RenderingInfo.depth_attachment->load_op,
              p_RenderingInfo.depth_attachment->store_op,
              p_RenderingInfo.depth_attachment->clear);
          l_DepthAttachmentPtr = &l_DepthAttachment;
        }

        VkRenderingInfo l_RenderInfo{};
        l_RenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        l_RenderInfo.pNext = nullptr;

        l_RenderInfo.renderArea =
            VkRect2D{VkOffset2D{0, 0}, l_RenderExtent};
        l_RenderInfo.layerCount = 1;
        l_RenderInfo.colorAttachmentCount = l_ColorAttachments.size();
        l_RenderInfo.pColorAttachments = l_ColorAttachments.data();
        l_RenderInfo.pDepthAttachment = l_DepthAttachmentPtr;
        l_RenderInfo.pStencilAttachment = nullptr;

        vkCmdBeginRendering(l_CommandListState->command_buffer,
                            &l_RenderInfo);
      }

      void
      end_dynamic_rendering(Detail::ContextImpl &p_Context,
                            Detail::BackendCommandList &p_CommandList)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(l_State,
                   "Cannot end rendering on Vulkan command list "
                   "without context state");
        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        LOW_ASSERT(l_CommandListState,
                   "Cannot end rendering on Vulkan command list "
                   "without command list state");

        vkCmdEndRendering(l_CommandListState->command_buffer);
      }

      void set_viewport(Detail::ContextImpl &p_Context,
                        Detail::BackendCommandList &p_CommandList,
                        const Viewport &p_Viewport)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(l_State,
                   "Cannot set viewport on Vulkan command list "
                   "without context state");
        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        LOW_ASSERT(l_CommandListState,
                   "Cannot set viewport on Vulkan command list "
                   "without command list state");
        LOW_ASSERT(l_CommandListState->command_buffer !=
                       VK_NULL_HANDLE,
                   "Cannot set viewport without Vulkan command "
                   "buffer");

        VkViewport l_Viewport{};
        l_Viewport.x = p_Viewport.x;
        l_Viewport.y = p_Viewport.y;
        l_Viewport.width = p_Viewport.width;
        l_Viewport.height = p_Viewport.height;
        l_Viewport.minDepth = p_Viewport.min_depth;
        l_Viewport.maxDepth = p_Viewport.max_depth;

        vkCmdSetViewport(l_CommandListState->command_buffer, 0, 1,
                         &l_Viewport);
      }

      void set_scissor(Detail::ContextImpl &p_Context,
                       Detail::BackendCommandList &p_CommandList,
                       const Rect2D &p_Scissor)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(l_State,
                   "Cannot set scissor on Vulkan command list "
                   "without context state");
        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        LOW_ASSERT(l_CommandListState,
                   "Cannot set scissor on Vulkan command list "
                   "without command list state");
        LOW_ASSERT(l_CommandListState->command_buffer !=
                       VK_NULL_HANDLE,
                   "Cannot set scissor without Vulkan command "
                   "buffer");

        VkRect2D l_Scissor{};
        l_Scissor.offset.x = p_Scissor.offset.x;
        l_Scissor.offset.y = p_Scissor.offset.y;
        l_Scissor.extent.width = p_Scissor.extent.x;
        l_Scissor.extent.height = p_Scissor.extent.y;

        vkCmdSetScissor(l_CommandListState->command_buffer, 0, 1,
                        &l_Scissor);
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
