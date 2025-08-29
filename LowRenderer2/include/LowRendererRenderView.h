#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererRenderScene.h"
#include "LowRendererTexture.h"
#include "LowRendererRenderStep.h"
#include "LowRendererGpuSubmesh.h"
#include "LowRendererMaterialType.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    typedef void *RenderStepDataPtr;
    struct UiCanvas;

    struct DebugGeometryDraw
    {
      GpuSubmesh submesh;
      Math::Matrix4x4 transform;
      Math::Color color;

      bool depthTest;
      bool wireframe;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API RenderViewData
    {
      Low::Math::Vector3 camera_position;
      Low::Math::Vector3 camera_direction;
      uint64_t render_target_handle;
      uint64_t view_info_handle;
      Low::Math::UVector2 dimensions;
      Low::Renderer::RenderScene render_scene;
      Low::Renderer::Texture gbuffer_albedo;
      Low::Renderer::Texture gbuffer_normals;
      Low::Renderer::Texture gbuffer_depth;
      Low::Renderer::Texture gbuffer_viewposition;
      Low::Renderer::Texture lit_image;
      Low::Util::List<Low::Renderer::RenderStep> steps;
      Low::Util::List<RenderStepDataPtr> step_data;
      Low::Util::List<Low::Renderer::UiCanvas> ui_canvases;
      Low::Util::List<Low::Renderer::DebugGeometryDraw>
          debug_geometry;
      bool camera_dirty;
      bool dimensions_dirty;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(RenderViewData);
      }
    };

    struct LOW_RENDERER2_API RenderView : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<RenderView> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      RenderView();
      RenderView(uint64_t p_Id);
      RenderView(RenderView &p_Copy);

      static RenderView make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit RenderView(const RenderView &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static RenderView *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static RenderView find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      RenderView duplicate(Low::Util::Name p_Name) const;
      static RenderView duplicate(RenderView p_Handle,
                                  Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static RenderView find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == RenderView::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        RenderView l_RenderView = p_Handle.get_id();
        l_RenderView.destroy();
      }

      Low::Math::Vector3 &get_camera_position() const;
      void set_camera_position(Low::Math::Vector3 &p_Value);
      void set_camera_position(float p_X, float p_Y, float p_Z);
      void set_camera_position_x(float p_Value);
      void set_camera_position_y(float p_Value);
      void set_camera_position_z(float p_Value);

      Low::Math::Vector3 &get_camera_direction() const;
      void set_camera_direction(Low::Math::Vector3 &p_Value);
      void set_camera_direction(float p_X, float p_Y, float p_Z);
      void set_camera_direction_x(float p_Value);
      void set_camera_direction_y(float p_Value);
      void set_camera_direction_z(float p_Value);

      uint64_t get_render_target_handle() const;
      void set_render_target_handle(uint64_t p_Value);

      uint64_t get_view_info_handle() const;
      void set_view_info_handle(uint64_t p_Value);

      Low::Math::UVector2 &get_dimensions() const;
      void set_dimensions(Low::Math::UVector2 &p_Value);
      void set_dimensions(u32 p_X, u32 p_Y);
      void set_dimensions_x(u32 p_Value);
      void set_dimensions_y(u32 p_Value);

      Low::Renderer::RenderScene get_render_scene() const;
      void set_render_scene(Low::Renderer::RenderScene p_Value);

      Low::Renderer::Texture get_gbuffer_albedo() const;
      void set_gbuffer_albedo(Low::Renderer::Texture p_Value);

      Low::Renderer::Texture get_gbuffer_normals() const;
      void set_gbuffer_normals(Low::Renderer::Texture p_Value);

      Low::Renderer::Texture get_gbuffer_depth() const;
      void set_gbuffer_depth(Low::Renderer::Texture p_Value);

      Low::Renderer::Texture get_gbuffer_viewposition() const;
      void set_gbuffer_viewposition(Low::Renderer::Texture p_Value);

      Low::Renderer::Texture get_lit_image() const;
      void set_lit_image(Low::Renderer::Texture p_Value);

      Low::Util::List<Low::Renderer::RenderStep> &get_steps() const;

      Low::Util::List<RenderStepDataPtr> &get_step_data() const;
      void set_step_data(Low::Util::List<RenderStepDataPtr> &p_Value);

      Low::Util::List<Low::Renderer::UiCanvas> &
      get_ui_canvases() const;

      Low::Util::List<Low::Renderer::DebugGeometryDraw> &
      get_debug_geometry() const;

      bool is_camera_dirty() const;
      void set_camera_dirty(bool p_Value);
      void toggle_camera_dirty();
      void mark_camera_dirty();

      bool is_dimensions_dirty() const;
      void set_dimensions_dirty(bool p_Value);
      void toggle_dimensions_dirty();
      void mark_dimensions_dirty();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      void add_step(Low::Renderer::RenderStep p_Step);
      void add_step_by_name(Low::Util::Name p_StepName);
      void add_ui_canvas(Low::Renderer::UiCanvas p_Canvas);
      void add_debug_geometry(
          Low::Renderer::DebugGeometryDraw &p_DebugGeometryDraw);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
