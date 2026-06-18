#pragma once

#include "LowRendererRenderView.h"
#include "LowCoreEntity.h"
#include "LowUtilContainers.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    struct ViewportHandleRenderContext
    {
      float delta;
      Renderer::RenderView render_view;
      Util::Handle handle;
      Core::Entity entity;

      bool selected;
    };

    struct ViewportHandleRenderer;

    struct ViewportHandleRenderer
    {

      Util::Function<void(const ViewportHandleRenderContext &)>
          render;

      static void
      _register_renderer(const u16 p_TypeId, const u32 p_Priority,
                         const ViewportHandleRenderer &p_Renderer);

      template <typename T>
      static void
      register_renderer(const u32 p_Priority,
                        const ViewportHandleRenderer &p_Renderer)
      {
        _register_renderer(T::type_id(), p_Priority, p_Renderer);
      }

      static void _show(Renderer::RenderView p_RenderView,
                        Util::Handle p_Handle, Core::Entity p_Entity,
                        const bool p_Selected = false);

      static void _show(Renderer::RenderView p_RenderView,
                        Util::Handle p_Handle,
                        const bool p_Selected = false)
      {
        _show(p_RenderView, p_Handle, Util::Handle::DEAD, p_Selected);
      }

      template <typename T>
      static void show(Renderer::RenderView p_RenderView, T p_Handle,
                       const bool p_Selected = false)
      {
        _show(p_RenderView, p_Handle, Util::Handle::DEAD, p_Selected);
      }

      template <typename T>
      static void show(Renderer::RenderView p_RenderView, T p_Handle,
                       Core::Entity p_Entity,
                       const bool p_Selected = false)
      {
        _show(p_RenderView, p_Handle, p_Entity, p_Selected);
      }
    };

    struct ViewportEntityRenderer
    {
      Util::Function<void(const ViewportHandleRenderContext &)>
          render;
      Util::Function<void(const ViewportHandleRenderContext &)>
          render_selected;

      static void _register_component_renderer(
          const u16 p_ComponentTypeId, const u32 p_Priority,
          const ViewportEntityRenderer &p_Renderer);

      template <typename T>
      static void register_component_renderer(
          const u32 p_Priority,
          const ViewportEntityRenderer &p_Renderer)
      {
        _register_component_renderer(T::type_id(), p_Priority,
                                     p_Renderer);
      }

      static bool _show(Renderer::RenderView p_RenderView,
                        Core::Entity p_Entity,
                        const bool p_Selected = false);
      static bool _show(const ViewportHandleRenderContext &p_Context);

      static bool show(Renderer::RenderView p_RenderView,
                       Core::Entity p_Entity,
                       const bool p_Selected = false)
      {
        return _show(p_RenderView, p_Entity, p_Selected);
      }
    };
  } // namespace Editor
} // namespace Low
