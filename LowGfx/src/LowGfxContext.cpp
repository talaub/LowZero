#include "LowGfxContext.h"

#include "LowGfxBackend.h"
#include "LowGfxLogInternal.h"
#include "LowGfxVulkanBackend.h"

#include "LowUtilAssert.h"

#include <atomic>
#include <utility>

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

namespace Low {
  namespace Gfx {
    static u32 allocate_instance_id()
    {
      static std::atomic<u32> g_NextInstanceId{1};
      return g_NextInstanceId.fetch_add(1, std::memory_order_relaxed);
    }

    static u32 allocate_context_id()
    {
      static std::atomic<u32> g_NextContextId{1};
      return g_NextContextId.fetch_add(1, std::memory_order_relaxed);
    }

    static const Detail::BackendProvider &
    get_backend_provider(Backend p_Backend)
    {
      switch (p_Backend) {
      case Backend::Vulkan:
        return Vulkan::get_backend_provider();
      }

      LOW_ASSERT(false, "Unsupported LowGfx backend");
      return Vulkan::get_backend_provider();
    }

    static void cleanup_instance(Detail::InstanceImpl *p_Impl)
    {
      if (p_Impl && p_Impl->api) {
        p_Impl->surfaces.for_each(
            [p_Impl](Detail::BackendSurface &p_Surface) {
              p_Impl->api->destroy_surface(*p_Impl, p_Surface);
            });
        p_Impl->surfaces.clear();

        p_Impl->api->destroy_instance(*p_Impl);
        p_Impl->adapters.clear();
        p_Impl->api = nullptr;
        p_Impl->context_api = nullptr;
      }
    }

    static void cleanup_context(Detail::ContextImpl *p_Impl)
    {
      if (p_Impl && p_Impl->api) {
        p_Impl->buffers.for_each(
            [p_Impl](Detail::BackendBuffer &p_Buffer) {
              p_Impl->api->destroy_buffer(*p_Impl, p_Buffer);
            });
        p_Impl->buffers.clear();

        p_Impl->command_lists.for_each(
            [p_Impl](Detail::BackendCommandList &p_CommandList) {
              p_Impl->api->destroy_command_list(*p_Impl,
                                                p_CommandList);
            });
        p_Impl->command_lists.clear();

        p_Impl->swapchains.for_each(
            [p_Impl](Detail::BackendSwapchain &p_Swapchain) {
              p_Impl->api->destroy_swapchain(*p_Impl, p_Swapchain);
            });
        p_Impl->swapchains.clear();

        p_Impl->api->destroy_context(*p_Impl);
        p_Impl->api = nullptr;
        p_Impl->instance = nullptr;
      }
    }

    Instance::Instance(const InstanceDesc &p_Desc)
        : m_Impl(std::make_unique<Detail::InstanceImpl>())
    {
      const Detail::BackendProvider &l_Provider =
          get_backend_provider(p_Desc.backend);
      LOW_ASSERT(l_Provider.instance_api,
                 "LowGfx backend does not provide an instance API");
      LOW_ASSERT(l_Provider.context_api,
                 "LowGfx backend does not provide a context API");

      m_Impl->instance_id = allocate_instance_id();
      m_Impl->backend = p_Desc.backend;
      m_Impl->api = l_Provider.instance_api;
      m_Impl->context_api = l_Provider.context_api;
      m_Impl->log_callback = p_Desc.log_callback;
      m_Impl->log_user_data = p_Desc.log_user_data;
      m_Impl->surfaces.set_owner_id(m_Impl->instance_id);
      m_Impl->backend_state =
          m_Impl->api->create_instance(*m_Impl, p_Desc);
    }

    Instance::~Instance()
    {
      cleanup_instance(m_Impl.get());
    }

    Instance::Instance(Instance &&) noexcept = default;

    Instance &Instance::operator=(Instance &&p_Other) noexcept
    {
      if (this != &p_Other) {
        cleanup_instance(m_Impl.get());
        m_Impl = std::move(p_Other.m_Impl);
      }

      return *this;
    }

    Backend Instance::get_backend() const
    {
      return m_Impl->backend;
    }

    Surface Instance::create_surface(const SurfaceDesc &p_Desc)
    {
      Detail::BackendSurface l_BackendSurface =
          m_Impl->api->create_surface(*m_Impl, p_Desc);
      return m_Impl->surfaces.create(std::move(l_BackendSurface));
    }

    void Instance::destroy(Surface p_Surface)
    {
      Detail::BackendSurface *l_Surface =
          m_Impl->surfaces.get(p_Surface);
      if (!l_Surface) {
        return;
      }

      m_Impl->api->destroy_surface(*m_Impl, *l_Surface);
      m_Impl->surfaces.destroy(p_Surface);
    }

    bool Instance::is_valid(Surface p_Surface) const
    {
      return m_Impl->surfaces.is_valid(p_Surface);
    }

    Adapter Instance::select_adapter(
        const AdapterSelectionDesc &p_Desc)
    {
      Adapter l_Adapter = m_Impl->api->select_adapter(*m_Impl, p_Desc);
      LOW_ASSERT(is_valid(l_Adapter),
                 "Backend returned an invalid LowGfx adapter");
      return l_Adapter;
    }

    bool Instance::is_valid(Adapter p_Adapter) const
    {
      if (!p_Adapter || p_Adapter.owner_id != m_Impl->instance_id ||
          p_Adapter.index >= m_Impl->adapters.size()) {
        return false;
      }

      return m_Impl->adapters[p_Adapter.index].generation ==
             p_Adapter.generation;
    }

    Context::Context(Instance &p_Instance, Adapter p_Adapter,
                     const ContextDesc &p_Desc)
        : m_Impl(std::make_unique<Detail::ContextImpl>())
    {
      LOW_ASSERT(p_Desc.frames_in_flight > 0,
                 "LowGfx frames_in_flight must be greater than zero");
      LOW_ASSERT(p_Instance.is_valid(p_Adapter),
                 "Cannot create LowGfx context from invalid adapter");

      m_Impl->context_id = allocate_context_id();
      m_Impl->instance_id = p_Instance.m_Impl->instance_id;
      m_Impl->backend = p_Instance.m_Impl->backend;
      m_Impl->api = p_Instance.m_Impl->context_api;
      m_Impl->instance = p_Instance.m_Impl.get();
      m_Impl->adapter = p_Adapter;
      m_Impl->log_callback = p_Instance.m_Impl->log_callback;
      m_Impl->log_user_data = p_Instance.m_Impl->log_user_data;
      m_Impl->frames_in_flight = p_Desc.frames_in_flight;
      m_Impl->buffers.set_owner_id(m_Impl->context_id);
      m_Impl->command_lists.set_owner_id(m_Impl->context_id);
      m_Impl->swapchains.set_owner_id(m_Impl->context_id);
      m_Impl->backend_state = m_Impl->api->create_context(
          *m_Impl, *m_Impl->instance, p_Adapter, p_Desc);
      m_Impl->caps = m_Impl->api->get_caps(*m_Impl);
    }

    Context::~Context()
    {
      cleanup_context(m_Impl.get());
    }

    Context::Context(Context &&) noexcept = default;

    Context &Context::operator=(Context &&p_Other) noexcept
    {
      if (this != &p_Other) {
        cleanup_context(m_Impl.get());
        m_Impl = std::move(p_Other.m_Impl);
      }

      return *this;
    }

    Backend Context::get_backend() const
    {
      return m_Impl->backend;
    }

    const DeviceCaps &Context::get_caps() const
    {
      return m_Impl->caps;
    }

    Buffer Context::create_buffer(const BufferDesc &p_Desc)
    {
      LOW_ASSERT(p_Desc.size > 0, "Cannot create zero-sized buffer");
      LOW_ASSERT(p_Desc.usage != BufferUsage::None,
                 "Cannot create buffer without usage flags");

      Detail::BackendBuffer l_BackendBuffer =
          m_Impl->api->create_buffer(*m_Impl, p_Desc);
      return m_Impl->buffers.create(std::move(l_BackendBuffer));
    }

    void Context::destroy(Buffer p_Buffer)
    {
      Detail::BackendBuffer *l_Buffer = m_Impl->buffers.get(p_Buffer);
      if (!l_Buffer) {
        return;
      }

      m_Impl->api->destroy_buffer(*m_Impl, *l_Buffer);
      m_Impl->buffers.destroy(p_Buffer);
    }

    bool Context::is_valid(Buffer p_Buffer) const
    {
      return m_Impl->buffers.is_valid(p_Buffer);
    }

    CommandList
    Context::request_command_list(const FrameContext &p_Frame,
                                  const QueueRole p_QueueRole)
    {
      LOW_ASSERT(m_Impl->frame_active,
                 "Cannot request a frame command list before "
                 "begin_frame");
      LOW_ASSERT(p_Frame.m_FrameIndex == m_Impl->frame_index &&
                     p_Frame.m_FrameNumber == m_Impl->frame_number,
                 "Cannot request a command list with a stale frame "
                 "context");

      Detail::BackendCommandList l_BackendCommandList =
          m_Impl->api->request_command_list(*m_Impl, p_Frame,
                                            p_QueueRole);
      CommandList l_CommandList = m_Impl->command_lists.create(
          std::move(l_BackendCommandList));
      m_Impl->frame_command_lists.push_back(l_CommandList);
      return l_CommandList;
    }

    CommandList Context::request_immediate_command_list(
        const QueueRole p_QueueRole)
    {
      Detail::BackendCommandList l_BackendCommandList =
          m_Impl->api->request_immediate_command_list(*m_Impl,
                                                      p_QueueRole);
      return m_Impl->command_lists.create(
          std::move(l_BackendCommandList));
    }

    void Context::destroy(CommandList p_CommandList)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      if (!l_CommandList) {
        return;
      }

      LOW_ASSERT(l_CommandList->state != CommandListState::Submitted,
                 "Cannot destroy a submitted command list");

      m_Impl->api->destroy_command_list(*m_Impl, *l_CommandList);
      m_Impl->command_lists.destroy(p_CommandList);
    }

    bool Context::is_valid(CommandList p_CommandList) const
    {
      return m_Impl->command_lists.is_valid(p_CommandList);
    }

    Swapchain Context::create_swapchain(const SwapchainDesc &p_Desc)
    {
      LOW_ASSERT(p_Desc.width > 0,
                 "Cannot create zero-width swapchain");
      LOW_ASSERT(p_Desc.height > 0,
                 "Cannot create zero-height swapchain");
      LOW_ASSERT(m_Impl->instance &&
                     m_Impl->instance->surfaces.is_valid(
                         p_Desc.surface),
                 "Cannot create swapchain from invalid surface");

      Detail::BackendSwapchain l_BackendSwapchain =
          m_Impl->api->create_swapchain(*m_Impl, p_Desc);
      return m_Impl->swapchains.create(std::move(l_BackendSwapchain));
    }

    void Context::destroy(Swapchain p_Swapchain)
    {
      Detail::BackendSwapchain *l_Swapchain =
          m_Impl->swapchains.get(p_Swapchain);
      if (!l_Swapchain) {
        return;
      }

      LOW_ASSERT(!m_Impl->frame_active,
                 "Cannot destroy swapchain while a frame is active");

      m_Impl->api->destroy_swapchain(*m_Impl, *l_Swapchain);
      m_Impl->swapchains.destroy(p_Swapchain);
    }

    bool Context::is_valid(Swapchain p_Swapchain) const
    {
      return m_Impl->swapchains.is_valid(p_Swapchain);
    }

    FrameContext Context::begin_frame()
    {
      LOW_ASSERT(
          !m_Impl->frame_active,
          "Cannot begin a frame while another frame is active");

      FrameContext l_Frame;
      l_Frame.m_FrameIndex = m_Impl->frame_index;
      l_Frame.m_FrameNumber = m_Impl->frame_number;

      m_Impl->api->begin_frame(*m_Impl, l_Frame);
      m_Impl->frame_active = true;

      return l_Frame;
    }

    SwapchainFrame
    Context::acquire_swapchain(const FrameContext &p_Frame,
                               Swapchain p_Swapchain)
    {
      LOW_ASSERT(m_Impl->frame_active,
                 "Cannot acquire swapchain before begin_frame");
      LOW_ASSERT(p_Frame.m_FrameIndex == m_Impl->frame_index &&
                     p_Frame.m_FrameNumber == m_Impl->frame_number,
                 "Cannot acquire swapchain with a stale frame context");
      LOW_ASSERT(m_Impl->swapchains.is_valid(p_Swapchain),
                 "Cannot acquire invalid swapchain");

      SwapchainFrame l_SwapchainFrame;
      l_SwapchainFrame.m_FrameIndex = p_Frame.m_FrameIndex;
      l_SwapchainFrame.m_FrameNumber = p_Frame.m_FrameNumber;
      l_SwapchainFrame.m_Swapchain = p_Swapchain;

      m_Impl->api->acquire_swapchain(*m_Impl, p_Frame,
                                     l_SwapchainFrame);
      return l_SwapchainFrame;
    }

    void
    Context::present(const SwapchainFrame &p_SwapchainFrame)
    {
      LOW_ASSERT(m_Impl->frame_active,
                 "Cannot present swapchain before begin_frame");
      LOW_ASSERT(
          p_SwapchainFrame.m_FrameIndex == m_Impl->frame_index &&
              p_SwapchainFrame.m_FrameNumber == m_Impl->frame_number,
          "Cannot present swapchain with a stale swapchain frame");
      LOW_ASSERT(
          m_Impl->swapchains.is_valid(p_SwapchainFrame.m_Swapchain),
          "Cannot present invalid swapchain");

      m_Impl->api->present(*m_Impl, p_SwapchainFrame);
    }

    void Context::end_frame(const FrameContext &p_Frame)
    {
      LOW_ASSERT(m_Impl->frame_active,
                 "Cannot end a frame before begin_frame");
      LOW_ASSERT(p_Frame.m_FrameIndex == m_Impl->frame_index &&
                     p_Frame.m_FrameNumber == m_Impl->frame_number,
                 "Cannot end frame with a stale frame context");

      m_Impl->api->end_frame(*m_Impl, p_Frame);

      for (CommandList i_CommandList : m_Impl->frame_command_lists) {
        destroy(i_CommandList);
      }
      m_Impl->frame_command_lists.clear();

      m_Impl->frame_active = false;

      m_Impl->frame_number++;
      m_Impl->frame_index =
          (m_Impl->frame_index + 1) % m_Impl->frames_in_flight;
    }
  } // namespace Gfx
} // namespace Low
