#include "LowEditorViewportRendering.h"
#include "LowCore.h"
#include "LowRendererRenderView.h"

#include <algorithm>

namespace Low {
  namespace Editor {
    struct ViewportHandleRendererEntry
    {
      ViewportHandleRenderer renderer;
      u32 priority;
    };

    struct ViewportEntityRendererEntry
    {
      ViewportEntityRenderer renderer;
      u32 priority;
    };

    struct ViewportEntityRendererCandidate
    {
      ViewportEntityRenderer renderer;
      u32 priority;
      Util::Handle component;
    };

    Util::Map<u16, Util::List<ViewportHandleRendererEntry>>
        ms_RenderersByType;
    Util::Map<u16, Util::List<ViewportEntityRendererEntry>>
        ms_EntityRenderersByComponentType;

    void ViewportHandleRenderer::_register_renderer(
        const u16 p_TypeId, const u32 p_Priority,
        const ViewportHandleRenderer &p_Renderer)
    {
      Util::List<ViewportHandleRendererEntry> &l_Renderers =
          ms_RenderersByType[p_TypeId];

      l_Renderers.push_back(
          ViewportHandleRendererEntry{p_Renderer, p_Priority});

      std::sort(l_Renderers.begin(), l_Renderers.end(),
                [](const ViewportHandleRendererEntry &p_Left,
                   const ViewportHandleRendererEntry &p_Right) {
                  return p_Left.priority < p_Right.priority;
                });
    }

    void ViewportHandleRenderer::_show(
        Renderer::RenderView p_RenderView, Util::Handle p_Handle,
        Core::Entity p_Entity, const bool p_Selected)
    {
      ViewportHandleRenderContext l_Context;
      l_Context.handle = p_Handle;
      l_Context.delta = LOW_DELTA_TIME;
      l_Context.entity = p_Entity;
      l_Context.selected = p_Selected;
      l_Context.render_view = p_RenderView;

      auto l_Entry = ms_RenderersByType.find(p_Handle.get_type());
      if (l_Entry == ms_RenderersByType.end()) {
        return;
      }

      for (const ViewportHandleRendererEntry &i_Entry :
           l_Entry->second) {
        if (i_Entry.renderer.render) {
          i_Entry.renderer.render(l_Context);
        }
      }
    }

    void ViewportEntityRenderer::_register_component_renderer(
        const u16 p_ComponentTypeId, const u32 p_Priority,
        const ViewportEntityRenderer &p_Renderer)
    {
      Util::List<ViewportEntityRendererEntry> &l_Renderers =
          ms_EntityRenderersByComponentType[p_ComponentTypeId];

      l_Renderers.push_back(
          ViewportEntityRendererEntry{p_Renderer, p_Priority});

      std::sort(l_Renderers.begin(), l_Renderers.end(),
                [](const ViewportEntityRendererEntry &p_Left,
                   const ViewportEntityRendererEntry &p_Right) {
                  return p_Left.priority < p_Right.priority;
                });
    }

    bool ViewportEntityRenderer::_show(
        Renderer::RenderView p_RenderView, Core::Entity p_Entity,
        const bool p_Selected)
    {
      ViewportHandleRenderContext l_Context;
      l_Context.handle = p_Entity;
      l_Context.delta = LOW_DELTA_TIME;
      l_Context.entity = p_Entity;
      l_Context.selected = p_Selected;
      l_Context.render_view = p_RenderView;

      return _show(l_Context);
    }

    bool ViewportEntityRenderer::_show(
        const ViewportHandleRenderContext &p_Context)
    {
      Core::Entity l_Entity = p_Context.entity;
      if (!l_Entity.is_alive() &&
          p_Context.handle.get_type() == Core::Entity::type_id()) {
        l_Entity = Core::Entity(p_Context.handle.get_id());
      }
      if (!l_Entity.is_alive()) {
        return false;
      }

      bool l_Rendered = false;
      ViewportEntityRendererCandidate l_RenderCandidate;
      l_RenderCandidate.priority = 0;
      l_RenderCandidate.component = Util::Handle::DEAD;

      Util::List<ViewportEntityRendererCandidate>
          l_SelectedRenderers;

      for (auto it = l_Entity.get_components().begin();
           it != l_Entity.get_components().end(); ++it) {
        Util::Handle i_Component = it->second;
        if (!i_Component.is_registered_type()) {
          continue;
        }
        if (!Util::Handle::get_type_info(i_Component.get_type())
                 .is_alive(i_Component)) {
          continue;
        }

        auto i_Entry = ms_EntityRenderersByComponentType.find(
            i_Component.get_type());
        if (i_Entry == ms_EntityRenderersByComponentType.end()) {
          continue;
        }

        for (const ViewportEntityRendererEntry &i_RendererEntry :
             i_Entry->second) {
          if (i_RendererEntry.renderer.render &&
              (!l_RenderCandidate.renderer.render ||
               i_RendererEntry.priority >=
                   l_RenderCandidate.priority)) {
            l_RenderCandidate.renderer = i_RendererEntry.renderer;
            l_RenderCandidate.priority = i_RendererEntry.priority;
            l_RenderCandidate.component = i_Component;
          }

          if (p_Context.selected &&
              i_RendererEntry.renderer.render_selected) {
            l_SelectedRenderers.push_back(
                ViewportEntityRendererCandidate{
                    i_RendererEntry.renderer,
                    i_RendererEntry.priority, i_Component});
          }
        }
      }

      if (l_RenderCandidate.renderer.render) {
        ViewportHandleRenderContext l_Context = p_Context;
        l_Context.handle = l_RenderCandidate.component;
        l_Context.entity = l_Entity;
        l_RenderCandidate.renderer.render(l_Context);
        l_Rendered = true;
      }

      std::sort(l_SelectedRenderers.begin(),
                l_SelectedRenderers.end(),
                [](const ViewportEntityRendererCandidate &p_Left,
                   const ViewportEntityRendererCandidate &p_Right) {
                  return p_Left.priority < p_Right.priority;
                });

      for (const ViewportEntityRendererCandidate &i_Candidate :
           l_SelectedRenderers) {
        ViewportHandleRenderContext l_Context = p_Context;
        l_Context.handle = i_Candidate.component;
        l_Context.entity = l_Entity;
        i_Candidate.renderer.render_selected(l_Context);
      }

      return l_Rendered;
    }
  } // namespace Editor
} // namespace Low
