#include "LowRendererRenderView.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererUiCanvas.h"
#include "LowRendererVkImage.h"
#include "LowRendererVkViewInfo.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t RenderView::TYPE_ID = 53;
    uint32_t RenderView::ms_Capacity = 0u;
    uint32_t RenderView::ms_PageSize = 0u;
    Low::Util::SharedMutex RenderView::ms_LivingMutex;
    Low::Util::SharedMutex RenderView::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        RenderView::ms_PagesLock(RenderView::ms_PagesMutex,
                                 std::defer_lock);
    Low::Util::List<RenderView> RenderView::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        RenderView::ms_Pages;

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
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      RenderView l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = RenderView::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView,
                                 camera_position, Low::Math::Vector3))
          Low::Math::Vector3();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderView, camera_direction, Low::Math::Vector3))
          Low::Math::Vector3();
      ACCESSOR_TYPE_SOA(l_Handle, RenderView, camera_fov, float) =
          0.0f;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView, dimensions,
                                 Low::Math::UVector2))
          Low::Math::UVector2();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderView, desired_dimensions,
          Low::Math::UVector2)) Low::Math::UVector2();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView, render_scene,
                                 Low::Renderer::RenderScene))
          Low::Renderer::RenderScene();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView, gbuffer_albedo,
                                 Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderView, gbuffer_normals,
          Low::Renderer::Texture)) Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView, gbuffer_depth,
                                 Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderView, gbuffer_viewposition,
          Low::Renderer::Texture)) Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView, object_map,
                                 Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView, lit_image,
                                 Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView, blurred_image,
                                 Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderView, steps,
          Low::Util::List<Low::Renderer::RenderStep>))
          Low::Util::List<Low::Renderer::RenderStep>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderView, step_data,
                                 Low::Util::List<RenderStepDataPtr>))
          Low::Util::List<RenderStepDataPtr>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderView, ui_canvases,
          Low::Util::List<Low::Renderer::UiCanvas>))
          Low::Util::List<Low::Renderer::UiCanvas>();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderView, debug_geometry,
          Low::Util::List<Low::Renderer::DebugGeometryDraw>))
          Low::Util::List<Low::Renderer::DebugGeometryDraw>();
      ACCESSOR_TYPE_SOA(l_Handle, RenderView, camera_dirty, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, RenderView, dimensions_dirty,
                        bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, RenderView, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_dimensions(500, 500);
      l_Handle.set_camera_fov(45.0f);
      l_Handle.set_camera_position(0, 0, 0);
      l_Handle.set_camera_direction(0, 0, -1);
      l_Handle.set_camera_dirty(true);
      l_Handle.set_dimensions_dirty(true);
      l_Handle.get_step_data().resize(RenderStep::get_capacity());
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderView::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<RenderView> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        for (auto it = get_steps().begin(); it != get_steps().end();
             ++it) {
          it->teardown(get_id());
          free(get_step_data()[it->get_index()]);
        }
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Page->slots[l_SlotIndex].m_Occupied = false;
      l_Page->slots[l_SlotIndex].m_Generation++;

      ms_PagesLock.lock();
      Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
      l_LivingLock.unlock();
    }

    void RenderView::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(RenderView));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, RenderView::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

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
            offsetof(RenderView::Data, camera_position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, camera_direction);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          *((Low::Math::Vector3 *)p_Data) =
              l_Handle.get_camera_direction();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: camera_direction
      }
      {
        // Property: camera_fov
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_fov);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderView::Data, camera_fov);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          l_Handle.get_camera_fov();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            camera_fov, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_camera_fov(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          *((float *)p_Data) = l_Handle.get_camera_fov();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: camera_fov
      }
      {
        // Property: render_target_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_target_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderView::Data, render_target_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, view_info_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          l_Handle.get_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderView, dimensions, Low::Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_actual_dimensions(
              *(Low::Math::UVector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          *((Low::Math::UVector2 *)p_Data) =
              l_Handle.get_dimensions();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dimensions
      }
      {
        // Property: desired_dimensions
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(desired_dimensions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderView::Data, desired_dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          l_Handle.get_desired_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            desired_dimensions,
                                            Low::Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_dimensions(*(Low::Math::UVector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          *((Low::Math::UVector2 *)p_Data) =
              l_Handle.get_desired_dimensions();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: desired_dimensions
      }
      {
        // Property: render_scene
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_scene);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderView::Data, render_scene);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::RenderScene::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, gbuffer_albedo);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, gbuffer_normals);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, gbuffer_depth);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, gbuffer_viewposition);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_gbuffer_viewposition();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gbuffer_viewposition
      }
      {
        // Property: object_map
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(object_map);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderView::Data, object_map);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          l_Handle.get_object_map();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            object_map,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_object_map(*(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_object_map();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: object_map
      }
      {
        // Property: lit_image
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(lit_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderView::Data, lit_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_lit_image();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: lit_image
      }
      {
        // Property: blurred_image
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(blurred_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderView::Data, blurred_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          l_Handle.get_blurred_image();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderView,
                                            blurred_image,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderView l_Handle = p_Handle.get_id();
          l_Handle.set_blurred_image(
              *(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_blurred_image();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: blurred_image
      }
      {
        // Property: steps
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(steps);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderView::Data, steps);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, step_data);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, ui_canvases);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, debug_geometry);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, camera_dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
            offsetof(RenderView::Data, dimensions_dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(RenderView::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderView l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<RenderView> l_HandleLock(l_Handle);
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
      {
        // Function: read_object_id_px
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(read_object_id_px);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_PixelPosition);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: read_object_id_px
      }
      {
        // Function: make_default
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_default);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType =
            Low::Renderer::RenderView::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_default
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderView::cleanup()
    {
      Low::Util::List<RenderView> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      ms_PagesLock.lock();
      for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
        Low::Util::Instances::Page *i_Page = *it;
        free(i_Page->buffer);
        free(i_Page->slots);
        free(i_Page->lockWords);
        delete i_Page;
        it = ms_Pages.erase(it);
      }

      ms_Capacity = 0;

      ms_PagesLock.unlock();
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
      l_Handle.m_Data.m_Type = RenderView::TYPE_ID;

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
        l_Handle.m_Data.m_Generation = 0;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Handle.m_Data.m_Generation =
          l_Page->slots[l_SlotIndex].m_Generation;

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
      if (m_Data.m_Type != RenderView::TYPE_ID) {
        return false;
      }
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      if (!get_page_for_index(get_index(), l_PageIndex,
                              l_SlotIndex)) {
        return false;
      }
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      return m_Data.m_Type == RenderView::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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

      Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
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
      l_Handle.set_camera_fov(get_camera_fov());
      l_Handle.set_render_target_handle(get_render_target_handle());
      l_Handle.set_view_info_handle(get_view_info_handle());
      l_Handle.set_actual_dimensions(get_dimensions());
      l_Handle.set_dimensions(get_desired_dimensions());
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
      if (get_object_map().is_alive()) {
        l_Handle.set_object_map(get_object_map());
      }
      if (get_lit_image().is_alive()) {
        l_Handle.set_lit_image(get_lit_image());
      }
      if (get_blurred_image().is_alive()) {
        l_Handle.set_blurred_image(get_blurred_image());
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
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_position
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_position

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
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_position
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_position

      if (get_camera_position() != p_Value) {
        // Set dirty flags
        mark_camera_dirty();

        // Set new value
        TYPE_SOA(RenderView, camera_position, Low::Math::Vector3) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_position
        // LOW_CODEGEN::END::CUSTOM:SETTER_camera_position

        broadcast_observable(N(camera_position));
      }
    }

    Low::Math::Vector3 &RenderView::get_camera_direction() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_direction
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_direction

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
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_direction
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_direction

      if (get_camera_direction() != p_Value) {
        // Set dirty flags
        mark_camera_dirty();

        // Set new value
        TYPE_SOA(RenderView, camera_direction, Low::Math::Vector3) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_direction
        // LOW_CODEGEN::END::CUSTOM:SETTER_camera_direction

        broadcast_observable(N(camera_direction));
      }
    }

    float RenderView::get_camera_fov() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_fov
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_fov

      return TYPE_SOA(RenderView, camera_fov, float);
    }
    void RenderView::set_camera_fov(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_fov
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_fov

      if (get_camera_fov() != p_Value) {
        // Set dirty flags
        mark_camera_dirty();

        // Set new value
        TYPE_SOA(RenderView, camera_fov, float) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_fov
        // LOW_CODEGEN::END::CUSTOM:SETTER_camera_fov

        broadcast_observable(N(camera_fov));
      }
    }

    uint64_t RenderView::get_render_target_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_target_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_target_handle

      return TYPE_SOA(RenderView, render_target_handle, uint64_t);
    }
    void RenderView::set_render_target_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_target_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_target_handle

      // Set new value
      TYPE_SOA(RenderView, render_target_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_target_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_target_handle

      broadcast_observable(N(render_target_handle));
    }

    uint64_t RenderView::get_view_info_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_info_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_view_info_handle

      return TYPE_SOA(RenderView, view_info_handle, uint64_t);
    }
    void RenderView::set_view_info_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_info_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_info_handle

      // Set new value
      TYPE_SOA(RenderView, view_info_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_info_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_view_info_handle

      broadcast_observable(N(view_info_handle));
    }

    Low::Math::UVector2 &RenderView::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions

      return TYPE_SOA(RenderView, dimensions, Low::Math::UVector2);
    }
    void RenderView::set_actual_dimensions(u32 p_X, u32 p_Y)
    {
      Low::Math::UVector2 l_Val(p_X, p_Y);
      set_actual_dimensions(l_Val);
    }

    void RenderView::set_actual_dimensions_x(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.x = p_Value;
      set_actual_dimensions(l_Value);
    }

    void RenderView::set_actual_dimensions_y(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.y = p_Value;
      set_actual_dimensions(l_Value);
    }

    void
    RenderView::set_actual_dimensions(Low::Math::UVector2 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions

      // Set new value
      TYPE_SOA(RenderView, dimensions, Low::Math::UVector2) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions

      broadcast_observable(N(dimensions));
    }

    Low::Math::UVector2 &RenderView::get_desired_dimensions() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_desired_dimensions
      // LOW_CODEGEN::END::CUSTOM:GETTER_desired_dimensions

      return TYPE_SOA(RenderView, desired_dimensions,
                      Low::Math::UVector2);
    }
    void RenderView::set_dimensions(u32 p_X, u32 p_Y)
    {
      Low::Math::UVector2 l_Val(p_X, p_Y);
      set_dimensions(l_Val);
    }

    void RenderView::set_dimensions_x(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_desired_dimensions();
      l_Value.x = p_Value;
      set_dimensions(l_Value);
    }

    void RenderView::set_dimensions_y(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_desired_dimensions();
      l_Value.y = p_Value;
      set_dimensions(l_Value);
    }

    void RenderView::set_dimensions(Low::Math::UVector2 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_desired_dimensions
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_desired_dimensions

      if (get_desired_dimensions() != p_Value) {
        // Set dirty flags
        mark_dimensions_dirty();

        // Set new value
        TYPE_SOA(RenderView, desired_dimensions,
                 Low::Math::UVector2) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_desired_dimensions
        // LOW_CODEGEN::END::CUSTOM:SETTER_desired_dimensions

        broadcast_observable(N(desired_dimensions));
      }
    }

    Low::Renderer::RenderScene RenderView::get_render_scene() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene

      return TYPE_SOA(RenderView, render_scene,
                      Low::Renderer::RenderScene);
    }
    void
    RenderView::set_render_scene(Low::Renderer::RenderScene p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene

      // Set new value
      TYPE_SOA(RenderView, render_scene, Low::Renderer::RenderScene) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene

      broadcast_observable(N(render_scene));
    }

    Low::Renderer::Texture RenderView::get_gbuffer_albedo() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_albedo
      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_albedo

      return TYPE_SOA(RenderView, gbuffer_albedo,
                      Low::Renderer::Texture);
    }
    void
    RenderView::set_gbuffer_albedo(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_albedo
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_albedo

      // Set new value
      TYPE_SOA(RenderView, gbuffer_albedo, Low::Renderer::Texture) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_albedo
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_albedo

      broadcast_observable(N(gbuffer_albedo));
    }

    Low::Renderer::Texture RenderView::get_gbuffer_normals() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_normals
      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_normals

      return TYPE_SOA(RenderView, gbuffer_normals,
                      Low::Renderer::Texture);
    }
    void
    RenderView::set_gbuffer_normals(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_normals
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_normals

      // Set new value
      TYPE_SOA(RenderView, gbuffer_normals, Low::Renderer::Texture) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_normals
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_normals

      broadcast_observable(N(gbuffer_normals));
    }

    Low::Renderer::Texture RenderView::get_gbuffer_depth() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_depth
      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_depth

      return TYPE_SOA(RenderView, gbuffer_depth,
                      Low::Renderer::Texture);
    }
    void RenderView::set_gbuffer_depth(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_depth
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_depth

      // Set new value
      TYPE_SOA(RenderView, gbuffer_depth, Low::Renderer::Texture) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_depth
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_depth

      broadcast_observable(N(gbuffer_depth));
    }

    Low::Renderer::Texture
    RenderView::get_gbuffer_viewposition() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gbuffer_viewposition
      // LOW_CODEGEN::END::CUSTOM:GETTER_gbuffer_viewposition

      return TYPE_SOA(RenderView, gbuffer_viewposition,
                      Low::Renderer::Texture);
    }
    void RenderView::set_gbuffer_viewposition(
        Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gbuffer_viewposition
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gbuffer_viewposition

      // Set new value
      TYPE_SOA(RenderView, gbuffer_viewposition,
               Low::Renderer::Texture) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gbuffer_viewposition
      // LOW_CODEGEN::END::CUSTOM:SETTER_gbuffer_viewposition

      broadcast_observable(N(gbuffer_viewposition));
    }

    Low::Renderer::Texture RenderView::get_object_map() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_object_map
      // LOW_CODEGEN::END::CUSTOM:GETTER_object_map

      return TYPE_SOA(RenderView, object_map, Low::Renderer::Texture);
    }
    void RenderView::set_object_map(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_object_map
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_object_map

      // Set new value
      TYPE_SOA(RenderView, object_map, Low::Renderer::Texture) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_object_map
      // LOW_CODEGEN::END::CUSTOM:SETTER_object_map

      broadcast_observable(N(object_map));
    }

    Low::Renderer::Texture RenderView::get_lit_image() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_lit_image
      // LOW_CODEGEN::END::CUSTOM:GETTER_lit_image

      return TYPE_SOA(RenderView, lit_image, Low::Renderer::Texture);
    }
    void RenderView::set_lit_image(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_lit_image
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_lit_image

      // Set new value
      TYPE_SOA(RenderView, lit_image, Low::Renderer::Texture) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_lit_image
      // LOW_CODEGEN::END::CUSTOM:SETTER_lit_image

      broadcast_observable(N(lit_image));
    }

    Low::Renderer::Texture RenderView::get_blurred_image() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_blurred_image
      // LOW_CODEGEN::END::CUSTOM:GETTER_blurred_image

      return TYPE_SOA(RenderView, blurred_image,
                      Low::Renderer::Texture);
    }
    void RenderView::set_blurred_image(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_blurred_image
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_blurred_image

      // Set new value
      TYPE_SOA(RenderView, blurred_image, Low::Renderer::Texture) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_blurred_image
      // LOW_CODEGEN::END::CUSTOM:SETTER_blurred_image

      broadcast_observable(N(blurred_image));
    }

    Low::Util::List<Low::Renderer::RenderStep> &
    RenderView::get_steps() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_steps
      // LOW_CODEGEN::END::CUSTOM:GETTER_steps

      return TYPE_SOA(RenderView, steps,
                      Low::Util::List<Low::Renderer::RenderStep>);
    }

    Low::Util::List<RenderStepDataPtr> &
    RenderView::get_step_data() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_step_data
      // LOW_CODEGEN::END::CUSTOM:GETTER_step_data

      return TYPE_SOA(RenderView, step_data,
                      Low::Util::List<RenderStepDataPtr>);
    }
    void RenderView::set_step_data(
        Low::Util::List<RenderStepDataPtr> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_step_data
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_step_data

      // Set new value
      TYPE_SOA(RenderView, step_data,
               Low::Util::List<RenderStepDataPtr>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_step_data
      // LOW_CODEGEN::END::CUSTOM:SETTER_step_data

      broadcast_observable(N(step_data));
    }

    Low::Util::List<Low::Renderer::UiCanvas> &
    RenderView::get_ui_canvases() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ui_canvases
      // LOW_CODEGEN::END::CUSTOM:GETTER_ui_canvases

      return TYPE_SOA(RenderView, ui_canvases,
                      Low::Util::List<Low::Renderer::UiCanvas>);
    }

    Low::Util::List<Low::Renderer::DebugGeometryDraw> &
    RenderView::get_debug_geometry() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_debug_geometry
      // LOW_CODEGEN::END::CUSTOM:GETTER_debug_geometry

      return TYPE_SOA(
          RenderView, debug_geometry,
          Low::Util::List<Low::Renderer::DebugGeometryDraw>);
    }

    bool RenderView::is_camera_dirty() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_dirty

      return TYPE_SOA(RenderView, camera_dirty, bool);
    }
    void RenderView::toggle_camera_dirty()
    {
      set_camera_dirty(!is_camera_dirty());
    }

    void RenderView::set_camera_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_dirty

      // Set new value
      TYPE_SOA(RenderView, camera_dirty, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_dirty

      broadcast_observable(N(camera_dirty));
    }

    void RenderView::mark_camera_dirty()
    {
      if (!is_camera_dirty()) {
        TYPE_SOA(RenderView, camera_dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_camera_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_camera_dirty
      }
    }

    bool RenderView::is_dimensions_dirty() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions_dirty

      return TYPE_SOA(RenderView, dimensions_dirty, bool);
    }
    void RenderView::toggle_dimensions_dirty()
    {
      set_dimensions_dirty(!is_dimensions_dirty());
    }

    void RenderView::set_dimensions_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions_dirty

      // Set new value
      TYPE_SOA(RenderView, dimensions_dirty, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions_dirty

      broadcast_observable(N(dimensions_dirty));
    }

    void RenderView::mark_dimensions_dirty()
    {
      if (!is_dimensions_dirty()) {
        TYPE_SOA(RenderView, dimensions_dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dimensions_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_dimensions_dirty
      }
    }

    Low::Util::Name RenderView::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(RenderView, name, Low::Util::Name);
    }
    void RenderView::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderView> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(RenderView, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    void RenderView::add_step(Low::Renderer::RenderStep p_Step)
    {
      Low::Util::HandleLock<RenderView> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_step
      get_steps().push_back(p_Step);
      LOW_ASSERT(p_Step.prepare(get_id()),
                 "Failed to add renderstep to renderview");
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_step
    }

    void RenderView::add_step_by_name(Low::Util::Name p_StepName)
    {
      Low::Util::HandleLock<RenderView> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_step_by_name
      add_step(RenderStep::find_by_name(p_StepName));
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_step_by_name
    }

    void RenderView::add_ui_canvas(Low::Renderer::UiCanvas p_Canvas)
    {
      Low::Util::HandleLock<RenderView> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_ui_canvas
      get_ui_canvases().push_back(p_Canvas);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_ui_canvas
    }

    void RenderView::add_debug_geometry(
        Low::Renderer::DebugGeometryDraw &p_DebugGeometryDraw)
    {
      Low::Util::HandleLock<RenderView> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_debug_geometry
      get_debug_geometry().push_back(p_DebugGeometryDraw);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_debug_geometry
    }

    uint32_t RenderView::read_object_id_px(
        const Low::Math::UVector2 p_PixelPosition)
    {
      Low::Util::HandleLock<RenderView> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_read_object_id_px
      if (!get_object_map().is_alive() ||
          !get_object_map().get_gpu().is_alive()) {
        return LOW_UINT32_MAX;
      }
      Vulkan::ViewInfo l_ViewInfo = get_view_info_handle();
      Vulkan::Image l_Image =
          get_object_map().get_gpu().get_data_handle();

      const u32 l_Width = l_Image.get_allocated_image().extent.width;
      const u32 l_Height =
          l_Image.get_allocated_image().extent.height;

      if (p_PixelPosition.x > l_Width ||
          p_PixelPosition.y > l_Height) {
        return LOW_UINT32_MAX;
      }

      return ((u32 *)l_ViewInfo.get_object_id_buffer()
                  .info.pMappedData)[l_Width * p_PixelPosition.y +
                                     p_PixelPosition.x];

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_read_object_id_px
    }

    Low::Renderer::RenderView
    RenderView::make_default(Low::Util::Name p_Name)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_default
      RenderView l_RenderView = make(p_Name);

      l_RenderView.add_step_by_name(RENDERSTEP_SOLID_MATERIAL_NAME);
      l_RenderView.add_step_by_name(RENDERSTEP_LIGHTCULLING_NAME);
      l_RenderView.add_step_by_name(RENDERSTEP_SSAO_NAME);
      l_RenderView.add_step_by_name(RENDERSTEP_LIGHTING_NAME);
      l_RenderView.add_step_by_name(RENDERSTEP_DEBUG_GEOMETRY_NAME);
      l_RenderView.add_step_by_name(RENDERSTEP_UI_NAME);
      l_RenderView.add_step_by_name(RENDERSTEP_OBJECT_ID_COPY);
      l_RenderView.add_step_by_name(RENDERSTEP_BLUR);

      return l_RenderView;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_default
    }

    uint32_t RenderView::create_instance(
        u32 &p_PageIndex, u32 &p_SlotIndex,
        Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      u32 l_Index = 0;
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      bool l_FoundIndex = false;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

      for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
           ++l_PageIndex) {
        Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
            ms_Pages[l_PageIndex]->mutex);
        for (l_SlotIndex = 0;
             l_SlotIndex < ms_Pages[l_PageIndex]->size;
             ++l_SlotIndex) {
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
            l_FoundIndex = true;
            l_PageLock = std::move(i_PageLock);
            break;
          }
          l_Index++;
        }
        if (l_FoundIndex) {
          break;
        }
      }
      if (!l_FoundIndex) {
        l_SlotIndex = 0;
        l_PageIndex = create_page();
        Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
            ms_Pages[l_PageIndex]->mutex);
        l_PageLock = std::move(l_NewLock);
      }
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      p_PageLock = std::move(l_PageLock);
      LOCK_UNLOCK(l_PagesLock);
      return l_Index;
    }

    u32 RenderView::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for RenderView.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, RenderView::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool RenderView::get_page_for_index(const u32 p_Index,
                                        u32 &p_PageIndex,
                                        u32 &p_SlotIndex)
    {
      if (p_Index >= get_capacity()) {
        p_PageIndex = LOW_UINT32_MAX;
        p_SlotIndex = LOW_UINT32_MAX;
        return false;
      }
      p_PageIndex = p_Index / ms_PageSize;
      if (p_PageIndex > (ms_Pages.size() - 1)) {
        return false;
      }
      p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);
      return true;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
