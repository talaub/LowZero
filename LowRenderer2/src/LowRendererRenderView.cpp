#include "LowRendererRenderView.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererUiCanvas.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t RenderView::TYPE_ID = 53;
    uint32_t RenderView::ms_Capacity = 0u;
    uint8_t *RenderView::ms_Buffer = 0;
    std::shared_mutex RenderView::ms_BufferMutex;
    Low::Util::Instances::Slot *RenderView::ms_Slots = 0;
    Low::Util::List<RenderView> RenderView::ms_LivingInstances =
        Low::Util::List<RenderView>();

    RenderView::RenderView() : Low::Util::Handle(0ull)
    {
    }
    RenderView::RenderView(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    RenderView::RenderView(RenderView &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle RenderView::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    RenderView RenderView::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      RenderView l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderView::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, camera_position,
                              Low::Math::Vector3))
          Low::Math::Vector3();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, camera_direction,
                              Low::Math::Vector3))
          Low::Math::Vector3();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, dimensions,
                              Low::Math::UVector2))
          Low::Math::UVector2();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, render_scene,
                              Low::Renderer::RenderScene))
          Low::Renderer::RenderScene();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, gbuffer_albedo,
                              Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, gbuffer_normals,
                              Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, gbuffer_depth,
                              Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, RenderView, gbuffer_viewposition,
          Low::Renderer::Texture)) Low::Renderer::Texture();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, lit_image,
                              Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, RenderView, steps,
          Low::Util::List<Low::Renderer::RenderStep>))
          Low::Util::List<Low::Renderer::RenderStep>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderView, step_data,
                              Low::Util::List<RenderStepDataPtr>))
          Low::Util::List<RenderStepDataPtr>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, RenderView, ui_canvases,
          Low::Util::List<Low::Renderer::UiCanvas>))
          Low::Util::List<Low::Renderer::UiCanvas>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, RenderView, debug_geometry,
          Low::Util::List<Low::Renderer::DebugGeometryDraw>))
          Low::Util::List<Low::Renderer::DebugGeometryDraw>();
      ACCESSOR_TYPE_SOA(l_Handle, RenderView, camera_dirty, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, RenderView, dimensions_dirty,
                        bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, RenderView, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_camera_dirty(true);
      l_Handle.set_dimensions_dirty(true);
      l_Handle.get_step_data().resize(RenderStep::get_capacity());
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderView::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      for (auto it = get_steps().begin(); it != get_steps().end();
           ++it) {
        it->teardown(get_id());
        free(get_step_data()[it->get_index()]);
      }
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
    }

    void RenderView::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(RenderView));

      initialize_buffer(&ms_Buffer, RenderViewData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_RenderView);
      LOW_PROFILE_ALLOC(type_slots_RenderView);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(RenderView);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &RenderView::is_alive;
      l_TypeInfo.destroy = &RenderView::destroy;
      l_TypeInfo.serialize = &RenderView::serialize;
      l_TypeInfo.deserialize = &RenderView::deserialize;
      l_TypeInfo.find_by_index = &RenderView::_find_by_index;
      l_TypeInfo.notify = &RenderView::_notify;
      l_TypeInfo.find_by_name = &RenderView::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &RenderView::_make;
      l_TypeInfo.duplicate_default = &RenderView::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &RenderView::living_instances);
      l_TypeInfo.get_living_count = &RenderView::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: camera_position
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_position);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, camera_position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_camera_position();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            camera_position,
                                            Low::Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_camera_position(*(Low::Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Math::Vector3 *)p_Data) =
              l_Handle.get_camera_position();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: camera_position
      }
      {
        // Property: camera_direction
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_direction);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, camera_direction);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_camera_direction();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            camera_direction,
                                            Low::Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_camera_direction(
              *(Low::Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Math::Vector3 *)p_Data) =
              l_Handle.get_camera_direction();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: camera_direction
      }
      {
        // Property: render_target_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_target_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, render_target_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_render_target_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, render_target_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_render_target_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_render_target_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_target_handle
      }
      {
        // Property: view_info_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(view_info_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, view_info_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_view_info_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, view_info_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_view_info_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_view_info_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: view_info_handle
      }
      {
        // Property: dimensions
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, dimensions, Low::Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_dimensions(*(Low::Math::UVector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Math::UVector2 *)p_Data) =
              l_Handle.get_dimensions();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dimensions
      }
      {
        // Property: render_scene
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_scene);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, render_scene);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::RenderScene::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_render_scene();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, render_scene,
              Low::Renderer::RenderScene);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_render_scene(
              *(Low::Renderer::RenderScene *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Renderer::RenderScene *)p_Data) =
              l_Handle.get_render_scene();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_scene
      }
      {
        // Property: gbuffer_albedo
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gbuffer_albedo);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, gbuffer_albedo);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_gbuffer_albedo();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            gbuffer_albedo,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_gbuffer_albedo(
              *(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_gbuffer_albedo();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gbuffer_albedo
      }
      {
        // Property: gbuffer_normals
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gbuffer_normals);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, gbuffer_normals);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_gbuffer_normals();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            gbuffer_normals,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_gbuffer_normals(
              *(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_gbuffer_normals();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gbuffer_normals
      }
      {
        // Property: gbuffer_depth
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gbuffer_depth);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, gbuffer_depth);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_gbuffer_depth();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            gbuffer_depth,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_gbuffer_depth(
              *(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_gbuffer_depth();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gbuffer_depth
      }
      {
        // Property: gbuffer_viewposition
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gbuffer_viewposition);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, gbuffer_viewposition);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_gbuffer_viewposition();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            gbuffer_viewposition,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_gbuffer_viewposition(
              *(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_gbuffer_viewposition();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gbuffer_viewposition
      }
      {
        // Property: lit_image
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(lit_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, lit_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_lit_image();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            lit_image,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_lit_image(*(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_lit_image();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: lit_image
      }
      {
        // Property: steps
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(steps);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderViewData, steps);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_steps();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, steps,
              Low::Util::List<Low::Renderer::RenderStep>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Util::List<Low::Renderer::RenderStep> *)p_Data) =
              l_Handle.get_steps();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: steps
      }
      {
        // Property: step_data
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(step_data);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, step_data);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_step_data();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, step_data,
              Low::Util::List<RenderStepDataPtr>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_step_data(
              *(Low::Util::List<RenderStepDataPtr> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Util::List<RenderStepDataPtr> *)p_Data) =
              l_Handle.get_step_data();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: step_data
      }
      {
        // Property: ui_canvases
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(ui_canvases);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, ui_canvases);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_ui_canvases();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, ui_canvases,
              Low::Util::List<Low::Renderer::UiCanvas>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Util::List<Low::Renderer::UiCanvas> *)p_Data) =
              l_Handle.get_ui_canvases();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: ui_canvases
      }
      {
        // Property: debug_geometry
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(debug_geometry);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, debug_geometry);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_debug_geometry();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, debug_geometry,
              Low::Util::List<Low::Renderer::DebugGeometryDraw>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Util::List<Low::Renderer::DebugGeometryDraw> *)
                p_Data) = l_Handle.get_debug_geometry();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: debug_geometry
      }
      {
        // Property: camera_dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, camera_dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.is_camera_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            camera_dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_camera_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_camera_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: camera_dirty
      }
      {
        // Property: dimensions_dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions_dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderViewData, dimensions_dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.is_dimensions_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            dimensions_dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_dimensions_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_dimensions_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dimensions_dirty
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderViewData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: add_step
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(add_step);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderStep::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: add_step
      }
      {
        // Function: add_step_by_name
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(add_step_by_name);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_StepName);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: add_step_by_name
      }
      {
        // Function: add_ui_canvas
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(add_ui_canvas);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Canvas);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::UiCanvas::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: add_ui_canvas
      }
      {
        // Function: add_debug_geometry
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(add_debug_geometry);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_DebugGeometryDraw);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: add_debug_geometry
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderView::cleanup()
    {
      Low::Util::List<RenderView> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_RenderView);
      LOW_PROFILE_FREE(type_slots_RenderView);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle RenderView::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    RenderView RenderView::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      RenderView l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderView::TYPE_ID;

      return l_Handle;
    }

    RenderView RenderView::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      RenderView l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = RenderView::TYPE_ID;

      return l_Handle;
    }

    bool RenderView::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == RenderView::TYPE_ID &&
             check_alive(ms_Slots, RenderView::get_capacity());
    }

    uint32_t RenderView::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    RenderView::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    RenderView RenderView::find_by_name(Low::Util::Name p_Name)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
    }

    RenderView RenderView::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      RenderView l_Handle = make(p_Name);
      l_Handle.set_camera_position(get_camera_position());
      l_Handle.set_camera_direction(get_camera_direction());
      l_Handle.set_render_target_handle(get_render_target_handle());
      l_Handle.set_view_info_handle(get_view_info_handle());
      l_Handle.set_dimensions(get_dimensions());
      if (get_render_scene().is_alive()) {
        l_Handle.set_render_scene(get_render_scene());
      }
      if (get_gbuffer_albedo().is_alive()) {
        l_Handle.set_gbuffer_albedo(get_gbuffer_albedo());
      }
      if (get_gbuffer_normals().is_alive()) {
        l_Handle.set_gbuffer_normals(get_gbuffer_normals());
      }
      if (get_gbuffer_depth().is_alive()) {
        l_Handle.set_gbuffer_depth(get_gbuffer_depth());
      }
      if (get_gbuffer_viewposition().is_alive()) {
        l_Handle.set_gbuffer_viewposition(get_gbuffer_viewposition());
      }
      if (get_lit_image().is_alive()) {
        l_Handle.set_lit_image(get_lit_image());
      }
      l_Handle.set_step_data(get_step_data());
      l_Handle.set_camera_dirty(is_camera_dirty());
      l_Handle.set_dimensions_dirty(is_dimensions_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    RenderView RenderView::duplicate(RenderView p_Handle,
                                     Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    RenderView::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
    {
      RenderView l_RenderView = p_Handle.get_id();
      return l_RenderView.duplicate(p_Name);
    }

    void RenderView::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void RenderView::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Yaml::Node &p_Node)
    {
      RenderView l_RenderView = p_Handle.get_id();
      l_RenderView.serialize(p_Node);
    }

    Low::Util::Handle
    RenderView::deserialize(Low::Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void RenderView::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 RenderView::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 RenderView::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void RenderView::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void RenderView::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      RenderView l_RenderView = p_Observer.get_id();
      l_RenderView.notify(p_Observed, p_Observable);
    }

    Low::Math::Vector3 &RenderView::get_camera_position() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_position
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_position

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, camera_position,
                      Low::Math::Vector3);
    }
    void RenderView::set_camera_position(float p_X, float p_Y,
                                         float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_camera_position(p_Val);
    }

    void RenderView::set_camera_position_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_position();
      l_Value.x = p_Value;
      set_camera_position(l_Value);
    }

    void RenderView::set_camera_position_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_position();
      l_Value.y = p_Value;
      set_camera_position(l_Value);
    }

    void RenderView::set_camera_position_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_position();
      l_Value.z = p_Value;
      set_camera_position(l_Value);
    }

    void RenderView::set_camera_position(Low::Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_position
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_position

      if (get_camera_position() != p_Value) {
        // Set dirty flags
        mark_camera_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(RenderView, camera_position, Low::Math::Vector3) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_position
        // LOW_CODEGEN::END::CUSTOM:SETTER_camera_position

        broadcast_observable(N(camera_position));
      }
    }

    Low::Math::Vector3 &RenderView::get_camera_direction() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_direction
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_direction

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, camera_direction,
                      Low::Math::Vector3);
    }
    void RenderView::set_camera_direction(float p_X, float p_Y,
                                          float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_camera_direction(p_Val);
    }

    void RenderView::set_camera_direction_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_direction();
      l_Value.x = p_Value;
      set_camera_direction(l_Value);
    }

    void RenderView::set_camera_direction_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_direction();
      l_Value.y = p_Value;
      set_camera_direction(l_Value);
    }

    void RenderView::set_camera_direction_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_direction();
      l_Value.z = p_Value;
      set_camera_direction(l_Value);
    }

    void RenderView::set_camera_direction(Low::Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_direction
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_direction

      if (get_camera_direction() != p_Value) {
        // Set dirty flags
        mark_camera_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(RenderView, camera_direction, Low::Math::Vector3) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_direction
        // LOW_CODEGEN::END::CUSTOM:SETTER_camera_direction

        broadcast_observable(N(camera_direction));
      }
    }

    uint64_t RenderView::get_render_target_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_target_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_target_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, render_target_handle, uint64_t);
    }
    void RenderView::set_render_target_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_target_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_target_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, render_target_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_target_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_target_handle

      broadcast_observable(N(render_target_handle));
    }

    uint64_t RenderView::get_view_info_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_info_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_view_info_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, view_info_handle, uint64_t);
    }
    void RenderView::set_view_info_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_info_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_info_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, view_info_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_info_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_view_info_handle

      broadcast_observable(N(view_info_handle));
    }

    Low::Math::UVector2 &RenderView::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, dimensions, Low::Math::UVector2);
    }
    void RenderView::set_dimensions(u32 p_X, u32 p_Y)
    {
      Low::Math::UVector2 l_Val(p_X, p_Y);
      set_dimensions(l_Val);
    }

    void RenderView::set_dimensions_x(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.x = p_Value;
      set_dimensions(l_Value);
    }

    void RenderView::set_dimensions_y(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.y = p_Value;
      set_dimensions(l_Value);
    }

    void RenderView::set_dimensions(Low::Math::UVector2 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions

      if (get_dimensions() != p_Value) {
        // Set dirty flags
        mark_dimensions_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(RenderView, dimensions, Low::Math::UVector2) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions
        // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions

        broadcast_observable(N(dimensions));
      }
    }

    Low::Renderer::RenderScene RenderView::get_render_scene() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, render_scene,
                      Low::Renderer::RenderScene);
    }
    void
    RenderView::set_render_scene(Low::Renderer::RenderScene p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, render_scene, Low::Renderer::RenderScene) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene

      broadcast_observable(N(render_scene));
    }

    Low::Renderer::Texture RenderView::get_gbuffer_albedo() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_albedo
      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_albedo

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, gbuffer_albedo,
                      Low::Renderer::Texture);
    }
    void
    RenderView::set_gbuffer_albedo(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_albedo
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_albedo

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, gbuffer_albedo, Low::Renderer::Texture) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_albedo
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_albedo

      broadcast_observable(N(gbuffer_albedo));
    }

    Low::Renderer::Texture RenderView::get_gbuffer_normals() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_normals
      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_normals

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, gbuffer_normals,
                      Low::Renderer::Texture);
    }
    void
    RenderView::set_gbuffer_normals(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_normals
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_normals

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, gbuffer_normals, Low::Renderer::Texture) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_normals
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_normals

      broadcast_observable(N(gbuffer_normals));
    }

    Low::Renderer::Texture RenderView::get_gbuffer_depth() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_depth
      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_depth

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, gbuffer_depth,
                      Low::Renderer::Texture);
    }
    void RenderView::set_gbuffer_depth(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_depth
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_depth

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, gbuffer_depth, Low::Renderer::Texture) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_depth
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_depth

      broadcast_observable(N(gbuffer_depth));
    }

    Low::Renderer::Texture
    RenderView::get_gbuffer_viewposition() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_viewposition
      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_viewposition

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, gbuffer_viewposition,
                      Low::Renderer::Texture);
    }
    void RenderView::set_gbuffer_viewposition(
        Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_viewposition
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_viewposition

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, gbuffer_viewposition,
               Low::Renderer::Texture) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_viewposition
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_viewposition

      broadcast_observable(N(gbuffer_viewposition));
    }

    Low::Renderer::Texture RenderView::get_lit_image() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_lit_image
      // LOW_CODEGEN::END::CUSTOM:GETTER_lit_image

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, lit_image, Low::Renderer::Texture);
    }
    void RenderView::set_lit_image(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_lit_image
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_lit_image

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, lit_image, Low::Renderer::Texture) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_lit_image
      // LOW_CODEGEN::END::CUSTOM:SETTER_lit_image

      broadcast_observable(N(lit_image));
    }

    Low::Util::List<Low::Renderer::RenderStep> &
    RenderView::get_steps() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_steps
      // LOW_CODEGEN::END::CUSTOM:GETTER_steps

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, steps,
                      Low::Util::List<Low::Renderer::RenderStep>);
    }

    Low::Util::List<RenderStepDataPtr> &
    RenderView::get_step_data() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_step_data
      // LOW_CODEGEN::END::CUSTOM:GETTER_step_data

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, step_data,
                      Low::Util::List<RenderStepDataPtr>);
    }
    void RenderView::set_step_data(
        Low::Util::List<RenderStepDataPtr> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_step_data
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_step_data

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, step_data,
               Low::Util::List<RenderStepDataPtr>) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_step_data
      // LOW_CODEGEN::END::CUSTOM:SETTER_step_data

      broadcast_observable(N(step_data));
    }

    Low::Util::List<Low::Renderer::UiCanvas> &
    RenderView::get_ui_canvases() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ui_canvases
      // LOW_CODEGEN::END::CUSTOM:GETTER_ui_canvases

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, ui_canvases,
                      Low::Util::List<Low::Renderer::UiCanvas>);
    }

    Low::Util::List<Low::Renderer::DebugGeometryDraw> &
    RenderView::get_debug_geometry() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_debug_geometry
      // LOW_CODEGEN::END::CUSTOM:GETTER_debug_geometry

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(
          RenderView, debug_geometry,
          Low::Util::List<Low::Renderer::DebugGeometryDraw>);
    }

    bool RenderView::is_camera_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_dirty

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, camera_dirty, bool);
    }
    void RenderView::toggle_camera_dirty()
    {
      set_camera_dirty(!is_camera_dirty());
    }

    void RenderView::set_camera_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_dirty

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, camera_dirty, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_dirty

      broadcast_observable(N(camera_dirty));
    }

    void RenderView::mark_camera_dirty()
    {
      if (!is_camera_dirty()) {
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(RenderView, camera_dirty, bool) = true;
        LOCK_UNLOCK(l_WriteLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_camera_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_camera_dirty
      }
    }

    bool RenderView::is_dimensions_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions_dirty

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, dimensions_dirty, bool);
    }
    void RenderView::toggle_dimensions_dirty()
    {
      set_dimensions_dirty(!is_dimensions_dirty());
    }

    void RenderView::set_dimensions_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions_dirty

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, dimensions_dirty, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions_dirty

      broadcast_observable(N(dimensions_dirty));
    }

    void RenderView::mark_dimensions_dirty()
    {
      if (!is_dimensions_dirty()) {
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(RenderView, dimensions_dirty, bool) = true;
        LOCK_UNLOCK(l_WriteLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dimensions_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_dimensions_dirty
      }
    }

    Low::Util::Name RenderView::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderView, name, Low::Util::Name);
    }
    void RenderView::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderView, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    void RenderView::add_step(Low::Renderer::RenderStep p_Step)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_step
      get_steps().push_back(p_Step);
      LOW_ASSERT(p_Step.prepare(get_id()),
                 "Failed to add renderstep to renderview");
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_step
    }

    void RenderView::add_step_by_name(Low::Util::Name p_StepName)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_step_by_name
      add_step(RenderStep::find_by_name(p_StepName));
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_step_by_name
    }

    void RenderView::add_ui_canvas(Low::Renderer::UiCanvas p_Canvas)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_ui_canvas
      get_ui_canvases().push_back(p_Canvas);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_ui_canvas
    }

    void RenderView::add_debug_geometry(
        Low::Renderer::DebugGeometryDraw &p_DebugGeometryDraw)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_debug_geometry
      get_debug_geometry().push_back(p_DebugGeometryDraw);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_debug_geometry
    }

    uint32_t RenderView::create_instance()
    {
      uint32_t l_Index = 0u;

      for (; l_Index < get_capacity(); ++l_Index) {
        if (!ms_Slots[l_Index].m_Occupied) {
          break;
        }
      }
      if (l_Index >= get_capacity()) {
        increase_budget();
      }
      ms_Slots[l_Index].m_Occupied = true;
      return l_Index;
    }

    void RenderView::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(RenderViewData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderViewData, camera_position) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderViewData, camera_position) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Math::Vector3));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderViewData, camera_direction) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderViewData, camera_direction) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Math::Vector3));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderViewData,
                                     render_target_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderViewData,
                                   render_target_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderViewData, view_info_handle) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderViewData, view_info_handle) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderViewData, dimensions) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderViewData, dimensions) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::UVector2));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderViewData, render_scene) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderViewData, render_scene) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::RenderScene));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderViewData, gbuffer_albedo) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderViewData, gbuffer_albedo) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Texture));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderViewData, gbuffer_normals) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderViewData, gbuffer_normals) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Renderer::Texture));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderViewData, gbuffer_depth) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderViewData, gbuffer_depth) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Texture));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderViewData,
                                     gbuffer_viewposition) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderViewData,
                                   gbuffer_viewposition) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Texture));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderViewData, lit_image) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderViewData, lit_image) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Texture));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          RenderView i_RenderView = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(RenderViewData, steps) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<
                                   Low::Renderer::RenderStep>))])
              Low::Util::List<Low::Renderer::RenderStep>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(
              i_RenderView, RenderView, steps,
              Low::Util::List<Low::Renderer::RenderStep>);
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          RenderView i_RenderView = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(RenderViewData, step_data) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(
                                Low::Util::List<RenderStepDataPtr>))])
              Low::Util::List<RenderStepDataPtr>();
          *i_ValPtr =
              ACCESSOR_TYPE_SOA(i_RenderView, RenderView, step_data,
                                Low::Util::List<RenderStepDataPtr>);
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          RenderView i_RenderView = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(RenderViewData, ui_canvases) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<
                                   Low::Renderer::UiCanvas>))])
              Low::Util::List<Low::Renderer::UiCanvas>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(
              i_RenderView, RenderView, ui_canvases,
              Low::Util::List<Low::Renderer::UiCanvas>);
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          RenderView i_RenderView = *it;

          auto *i_ValPtr =
              new (&l_NewBuffer
                       [offsetof(RenderViewData, debug_geometry) *
                            (l_Capacity + l_CapacityIncrease) +
                        (it->get_index() *
                         sizeof(Low::Util::List<
                                Low::Renderer::DebugGeometryDraw>))])
                  Low::Util::List<Low::Renderer::DebugGeometryDraw>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(
              i_RenderView, RenderView, debug_geometry,
              Low::Util::List<Low::Renderer::DebugGeometryDraw>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderViewData, camera_dirty) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderViewData, camera_dirty) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderViewData, dimensions_dirty) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderViewData, dimensions_dirty) *
                       (l_Capacity)],
            l_Capacity * sizeof(bool));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderViewData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderViewData, name) * (l_Capacity)],
            l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity;
           i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for RenderView from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
