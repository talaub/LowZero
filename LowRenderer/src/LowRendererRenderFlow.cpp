#include "LowRendererRenderFlow.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowRendererComputeStep.h"
#include "LowRendererGraphicsStep.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t RenderFlow::TYPE_ID = 9;
    uint32_t RenderFlow::ms_Capacity = 0u;
    uint32_t RenderFlow::ms_PageSize = 0u;
    Low::Util::SharedMutex RenderFlow::ms_LivingMutex;
    Low::Util::SharedMutex RenderFlow::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        RenderFlow::ms_PagesLock(RenderFlow::ms_PagesMutex,
                                 std::defer_lock);
    Low::Util::List<RenderFlow> RenderFlow::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        RenderFlow::ms_Pages;

    RenderFlow::RenderFlow() : Low::Util::Handle(0ull)
    {
    }
    RenderFlow::RenderFlow(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    RenderFlow::RenderFlow(RenderFlow &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle RenderFlow::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    RenderFlow RenderFlow::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      RenderFlow l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = RenderFlow::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow, context,
                                 Interface::Context))
          Interface::Context();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow, dimensions,
                                 Math::UVector2)) Math::UVector2();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow, output_image,
                                 Resource::Image)) Resource::Image();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow, steps,
                                 Util::List<Util::Handle>))
          Util::List<Util::Handle>();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow, resources,
                                 ResourceRegistry))
          ResourceRegistry();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow,
                                 frame_info_buffer, Resource::Buffer))
          Resource::Buffer();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, RenderFlow, resource_signature,
          Interface::PipelineResourceSignature))
          Interface::PipelineResourceSignature();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow,
                                 camera_position, Math::Vector3))
          Math::Vector3();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow,
                                 camera_direction, Math::Vector3))
          Math::Vector3();
      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, camera_fov, float) =
          0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, camera_near_plane,
                        float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, camera_far_plane,
                        float) = 0.0f;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow,
                                 projection_matrix, Math::Matrix4x4))
          Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow, view_matrix,
                                 Math::Matrix4x4)) Math::Matrix4x4();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow,
                                 directional_light, DirectionalLight))
          DirectionalLight();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, RenderFlow, point_lights,
                                 Util::List<PointLight>))
          Util::List<PointLight>();
      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderFlow::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<RenderFlow> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

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

    void RenderFlow::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer),
                                                    N(RenderFlow));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, RenderFlow::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(RenderFlow);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &RenderFlow::is_alive;
      l_TypeInfo.destroy = &RenderFlow::destroy;
      l_TypeInfo.serialize = &RenderFlow::serialize;
      l_TypeInfo.deserialize = &RenderFlow::deserialize;
      l_TypeInfo.find_by_index = &RenderFlow::_find_by_index;
      l_TypeInfo.notify = &RenderFlow::_notify;
      l_TypeInfo.find_by_name = &RenderFlow::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &RenderFlow::_make;
      l_TypeInfo.duplicate_default = &RenderFlow::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &RenderFlow::living_instances);
      l_TypeInfo.get_living_count = &RenderFlow::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: context
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, context);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Interface::Context::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: context
      }
      {
        // Property: dimensions
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, dimensions, Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Math::UVector2 *)p_Data) = l_Handle.get_dimensions();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dimensions
      }
      {
        // Property: output_image
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(output_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, output_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Resource::Image::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_output_image();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, output_image, Resource::Image);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_output_image(*(Resource::Image *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Resource::Image *)p_Data) = l_Handle.get_output_image();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: output_image
      }
      {
        // Property: steps
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(steps);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlow::Data, steps);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_steps();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, steps, Util::List<Util::Handle>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_steps(*(Util::List<Util::Handle> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Util::List<Util::Handle> *)p_Data) =
              l_Handle.get_steps();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: steps
      }
      {
        // Property: resources
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resources);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, resources);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_resources();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, resources, ResourceRegistry);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((ResourceRegistry *)p_Data) = l_Handle.get_resources();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resources
      }
      {
        // Property: frame_info_buffer
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(frame_info_buffer);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, frame_info_buffer);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Resource::Buffer::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_frame_info_buffer();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            frame_info_buffer,
                                            Resource::Buffer);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Resource::Buffer *)p_Data) =
              l_Handle.get_frame_info_buffer();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: frame_info_buffer
      }
      {
        // Property: resource_signature
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource_signature);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, resource_signature);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Interface::PipelineResourceSignature::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_resource_signature();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, resource_signature,
              Interface::PipelineResourceSignature);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Interface::PipelineResourceSignature *)p_Data) =
              l_Handle.get_resource_signature();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource_signature
      }
      {
        // Property: camera_position
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_position);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, camera_position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_camera_position();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, camera_position, Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_position(*(Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Math::Vector3 *)p_Data) = l_Handle.get_camera_position();
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
            offsetof(RenderFlow::Data, camera_direction);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_camera_direction();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, camera_direction, Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_direction(*(Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Math::Vector3 *)p_Data) =
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
            offsetof(RenderFlow::Data, camera_fov);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_camera_fov();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            camera_fov, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_fov(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((float *)p_Data) = l_Handle.get_camera_fov();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: camera_fov
      }
      {
        // Property: camera_near_plane
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_near_plane);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, camera_near_plane);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_camera_near_plane();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            camera_near_plane, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_near_plane(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((float *)p_Data) = l_Handle.get_camera_near_plane();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: camera_near_plane
      }
      {
        // Property: camera_far_plane
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_far_plane);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, camera_far_plane);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_camera_far_plane();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            camera_far_plane, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_far_plane(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((float *)p_Data) = l_Handle.get_camera_far_plane();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: camera_far_plane
      }
      {
        // Property: projection_matrix
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(projection_matrix);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, projection_matrix);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_projection_matrix();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            projection_matrix,
                                            Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Math::Matrix4x4 *)p_Data) =
              l_Handle.get_projection_matrix();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: projection_matrix
      }
      {
        // Property: view_matrix
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(view_matrix);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, view_matrix);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_view_matrix();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, view_matrix, Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Math::Matrix4x4 *)p_Data) = l_Handle.get_view_matrix();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: view_matrix
      }
      {
        // Property: directional_light
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(directional_light);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, directional_light);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_directional_light();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            directional_light,
                                            DirectionalLight);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_directional_light(*(DirectionalLight *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((DirectionalLight *)p_Data) =
              l_Handle.get_directional_light();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: directional_light
      }
      {
        // Property: point_lights
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(point_lights);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlow::Data, point_lights);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_point_lights();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            point_lights,
                                            Util::List<PointLight>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Util::List<PointLight> *)p_Data) =
              l_Handle.get_point_lights();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: point_lights
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlow::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderFlow l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<RenderFlow> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = RenderFlow::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Context);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Interface::Context::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Config);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: clear_renderbojects
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(clear_renderbojects);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: clear_renderbojects
      }
      {
        // Function: execute
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(execute);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: execute
      }
      {
        // Function: update_dimensions
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(update_dimensions);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Dimensions);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: update_dimensions
      }
      {
        // Function: register_renderobject
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(register_renderobject);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderObject);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: register_renderobject
      }
      {
        // Function: get_previous_output_image
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_previous_output_image);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Resource::Image::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Step);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_previous_output_image
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderFlow::cleanup()
    {
      Low::Util::List<RenderFlow> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle RenderFlow::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    RenderFlow RenderFlow::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      RenderFlow l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = RenderFlow::TYPE_ID;

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

    RenderFlow RenderFlow::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      RenderFlow l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = RenderFlow::TYPE_ID;

      return l_Handle;
    }

    bool RenderFlow::is_alive() const
    {
      if (m_Data.m_Type != RenderFlow::TYPE_ID) {
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
      return m_Data.m_Type == RenderFlow::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t RenderFlow::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    RenderFlow::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    RenderFlow RenderFlow::find_by_name(Low::Util::Name p_Name)
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

    RenderFlow RenderFlow::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      RenderFlow l_Handle = make(p_Name);
      if (get_context().is_alive()) {
        l_Handle.set_context(get_context());
      }
      if (get_output_image().is_alive()) {
        l_Handle.set_output_image(get_output_image());
      }
      l_Handle.set_steps(get_steps());
      if (get_frame_info_buffer().is_alive()) {
        l_Handle.set_frame_info_buffer(get_frame_info_buffer());
      }
      if (get_resource_signature().is_alive()) {
        l_Handle.set_resource_signature(get_resource_signature());
      }
      l_Handle.set_camera_position(get_camera_position());
      l_Handle.set_camera_direction(get_camera_direction());
      l_Handle.set_camera_fov(get_camera_fov());
      l_Handle.set_camera_near_plane(get_camera_near_plane());
      l_Handle.set_camera_far_plane(get_camera_far_plane());
      l_Handle.set_projection_matrix(get_projection_matrix());
      l_Handle.set_view_matrix(get_view_matrix());
      l_Handle.set_directional_light(get_directional_light());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    RenderFlow RenderFlow::duplicate(RenderFlow p_Handle,
                                     Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    RenderFlow::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
    {
      RenderFlow l_RenderFlow = p_Handle.get_id();
      return l_RenderFlow.duplicate(p_Name);
    }

    void RenderFlow::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      if (get_context().is_alive()) {
        get_context().serialize(p_Node["context"]);
      }
      if (get_output_image().is_alive()) {
        get_output_image().serialize(p_Node["output_image"]);
      }
      if (get_frame_info_buffer().is_alive()) {
        get_frame_info_buffer().serialize(
            p_Node["frame_info_buffer"]);
      }
      if (get_resource_signature().is_alive()) {
        get_resource_signature().serialize(
            p_Node["resource_signature"]);
      }
      Low::Util::Serialization::serialize(p_Node["camera_position"],
                                          get_camera_position());
      Low::Util::Serialization::serialize(p_Node["camera_direction"],
                                          get_camera_direction());
      p_Node["camera_fov"] = get_camera_fov();
      p_Node["camera_near_plane"] = get_camera_near_plane();
      p_Node["camera_far_plane"] = get_camera_far_plane();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void RenderFlow::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Yaml::Node &p_Node)
    {
      RenderFlow l_RenderFlow = p_Handle.get_id();
      l_RenderFlow.serialize(p_Node);
    }

    Low::Util::Handle
    RenderFlow::deserialize(Low::Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator)
    {
      RenderFlow l_Handle = RenderFlow::make(N(RenderFlow));

      if (p_Node["context"]) {
        l_Handle.set_context(Interface::Context::deserialize(
                                 p_Node["context"], l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["dimensions"]) {
      }
      if (p_Node["output_image"]) {
        l_Handle.set_output_image(
            Resource::Image::deserialize(p_Node["output_image"],
                                         l_Handle.get_id())
                .get_id());
      }
      if (p_Node["steps"]) {
      }
      if (p_Node["resources"]) {
      }
      if (p_Node["frame_info_buffer"]) {
        l_Handle.set_frame_info_buffer(
            Resource::Buffer::deserialize(p_Node["frame_info_buffer"],
                                          l_Handle.get_id())
                .get_id());
      }
      if (p_Node["resource_signature"]) {
        l_Handle.set_resource_signature(
            Interface::PipelineResourceSignature::deserialize(
                p_Node["resource_signature"], l_Handle.get_id())
                .get_id());
      }
      if (p_Node["camera_position"]) {
        l_Handle.set_camera_position(
            Low::Util::Serialization::deserialize_vector3(
                p_Node["camera_position"]));
      }
      if (p_Node["camera_direction"]) {
        l_Handle.set_camera_direction(
            Low::Util::Serialization::deserialize_vector3(
                p_Node["camera_direction"]));
      }
      if (p_Node["camera_fov"]) {
        l_Handle.set_camera_fov(p_Node["camera_fov"].as<float>());
      }
      if (p_Node["camera_near_plane"]) {
        l_Handle.set_camera_near_plane(
            p_Node["camera_near_plane"].as<float>());
      }
      if (p_Node["camera_far_plane"]) {
        l_Handle.set_camera_far_plane(
            p_Node["camera_far_plane"].as<float>());
      }
      if (p_Node["projection_matrix"]) {
      }
      if (p_Node["view_matrix"]) {
      }
      if (p_Node["directional_light"]) {
      }
      if (p_Node["point_lights"]) {
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void RenderFlow::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 RenderFlow::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 RenderFlow::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void RenderFlow::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void RenderFlow::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
    {
      RenderFlow l_RenderFlow = p_Observer.get_id();
      l_RenderFlow.notify(p_Observed, p_Observable);
    }

    Interface::Context RenderFlow::get_context() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context

      // LOW_CODEGEN::END::CUSTOM:GETTER_context

      return TYPE_SOA(RenderFlow, context, Interface::Context);
    }
    void RenderFlow::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_context

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_context

      // Set new value
      TYPE_SOA(RenderFlow, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context

      // LOW_CODEGEN::END::CUSTOM:SETTER_context

      broadcast_observable(N(context));
    }

    Math::UVector2 &RenderFlow::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions

      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions

      return TYPE_SOA(RenderFlow, dimensions, Math::UVector2);
    }

    Resource::Image RenderFlow::get_output_image() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:GETTER_output_image

      return TYPE_SOA(RenderFlow, output_image, Resource::Image);
    }
    void RenderFlow::set_output_image(Resource::Image p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_output_image

      // Set new value
      TYPE_SOA(RenderFlow, output_image, Resource::Image) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_output_image

      // LOW_CODEGEN::END::CUSTOM:SETTER_output_image

      broadcast_observable(N(output_image));
    }

    Util::List<Util::Handle> &RenderFlow::get_steps() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_steps

      // LOW_CODEGEN::END::CUSTOM:GETTER_steps

      return TYPE_SOA(RenderFlow, steps, Util::List<Util::Handle>);
    }
    void RenderFlow::set_steps(Util::List<Util::Handle> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_steps

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_steps

      // Set new value
      TYPE_SOA(RenderFlow, steps, Util::List<Util::Handle>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_steps

      // LOW_CODEGEN::END::CUSTOM:SETTER_steps

      broadcast_observable(N(steps));
    }

    ResourceRegistry &RenderFlow::get_resources() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resources

      // LOW_CODEGEN::END::CUSTOM:GETTER_resources

      return TYPE_SOA(RenderFlow, resources, ResourceRegistry);
    }

    Resource::Buffer RenderFlow::get_frame_info_buffer() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_frame_info_buffer

      // LOW_CODEGEN::END::CUSTOM:GETTER_frame_info_buffer

      return TYPE_SOA(RenderFlow, frame_info_buffer,
                      Resource::Buffer);
    }
    void RenderFlow::set_frame_info_buffer(Resource::Buffer p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_frame_info_buffer

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_frame_info_buffer

      // Set new value
      TYPE_SOA(RenderFlow, frame_info_buffer, Resource::Buffer) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_frame_info_buffer

      // LOW_CODEGEN::END::CUSTOM:SETTER_frame_info_buffer

      broadcast_observable(N(frame_info_buffer));
    }

    Interface::PipelineResourceSignature
    RenderFlow::get_resource_signature() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource_signature

      // LOW_CODEGEN::END::CUSTOM:GETTER_resource_signature

      return TYPE_SOA(RenderFlow, resource_signature,
                      Interface::PipelineResourceSignature);
    }
    void RenderFlow::set_resource_signature(
        Interface::PipelineResourceSignature p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource_signature

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource_signature

      // Set new value
      TYPE_SOA(RenderFlow, resource_signature,
               Interface::PipelineResourceSignature) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource_signature

      // LOW_CODEGEN::END::CUSTOM:SETTER_resource_signature

      broadcast_observable(N(resource_signature));
    }

    Math::Vector3 &RenderFlow::get_camera_position() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_position

      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_position

      return TYPE_SOA(RenderFlow, camera_position, Math::Vector3);
    }
    void RenderFlow::set_camera_position(float p_X, float p_Y,
                                         float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_camera_position(p_Val);
    }

    void RenderFlow::set_camera_position_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_position();
      l_Value.x = p_Value;
      set_camera_position(l_Value);
    }

    void RenderFlow::set_camera_position_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_position();
      l_Value.y = p_Value;
      set_camera_position(l_Value);
    }

    void RenderFlow::set_camera_position_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_position();
      l_Value.z = p_Value;
      set_camera_position(l_Value);
    }

    void RenderFlow::set_camera_position(Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_position

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_position

      // Set new value
      TYPE_SOA(RenderFlow, camera_position, Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_position

      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_position

      broadcast_observable(N(camera_position));
    }

    Math::Vector3 &RenderFlow::get_camera_direction() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_direction

      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_direction

      return TYPE_SOA(RenderFlow, camera_direction, Math::Vector3);
    }
    void RenderFlow::set_camera_direction(float p_X, float p_Y,
                                          float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_camera_direction(p_Val);
    }

    void RenderFlow::set_camera_direction_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_direction();
      l_Value.x = p_Value;
      set_camera_direction(l_Value);
    }

    void RenderFlow::set_camera_direction_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_direction();
      l_Value.y = p_Value;
      set_camera_direction(l_Value);
    }

    void RenderFlow::set_camera_direction_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_camera_direction();
      l_Value.z = p_Value;
      set_camera_direction(l_Value);
    }

    void RenderFlow::set_camera_direction(Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_direction

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_direction

      // Set new value
      TYPE_SOA(RenderFlow, camera_direction, Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_direction

      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_direction

      broadcast_observable(N(camera_direction));
    }

    float RenderFlow::get_camera_fov() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_fov

      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_fov

      return TYPE_SOA(RenderFlow, camera_fov, float);
    }
    void RenderFlow::set_camera_fov(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_fov

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_fov

      // Set new value
      TYPE_SOA(RenderFlow, camera_fov, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_fov

      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_fov

      broadcast_observable(N(camera_fov));
    }

    float RenderFlow::get_camera_near_plane() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_near_plane

      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_near_plane

      return TYPE_SOA(RenderFlow, camera_near_plane, float);
    }
    void RenderFlow::set_camera_near_plane(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_near_plane

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_near_plane

      // Set new value
      TYPE_SOA(RenderFlow, camera_near_plane, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_near_plane

      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_near_plane

      broadcast_observable(N(camera_near_plane));
    }

    float RenderFlow::get_camera_far_plane() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_far_plane

      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_far_plane

      return TYPE_SOA(RenderFlow, camera_far_plane, float);
    }
    void RenderFlow::set_camera_far_plane(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_far_plane

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_far_plane

      // Set new value
      TYPE_SOA(RenderFlow, camera_far_plane, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_far_plane

      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_far_plane

      broadcast_observable(N(camera_far_plane));
    }

    Math::Matrix4x4 &RenderFlow::get_projection_matrix() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_projection_matrix

      // LOW_CODEGEN::END::CUSTOM:GETTER_projection_matrix

      return TYPE_SOA(RenderFlow, projection_matrix, Math::Matrix4x4);
    }
    void RenderFlow::set_projection_matrix(Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_projection_matrix

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_projection_matrix

      // Set new value
      TYPE_SOA(RenderFlow, projection_matrix, Math::Matrix4x4) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_projection_matrix

      // LOW_CODEGEN::END::CUSTOM:SETTER_projection_matrix

      broadcast_observable(N(projection_matrix));
    }

    Math::Matrix4x4 &RenderFlow::get_view_matrix() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_matrix

      // LOW_CODEGEN::END::CUSTOM:GETTER_view_matrix

      return TYPE_SOA(RenderFlow, view_matrix, Math::Matrix4x4);
    }
    void RenderFlow::set_view_matrix(Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_matrix

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_matrix

      // Set new value
      TYPE_SOA(RenderFlow, view_matrix, Math::Matrix4x4) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_matrix

      // LOW_CODEGEN::END::CUSTOM:SETTER_view_matrix

      broadcast_observable(N(view_matrix));
    }

    DirectionalLight &RenderFlow::get_directional_light() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_directional_light

      // LOW_CODEGEN::END::CUSTOM:GETTER_directional_light

      return TYPE_SOA(RenderFlow, directional_light,
                      DirectionalLight);
    }
    void RenderFlow::set_directional_light(DirectionalLight &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_directional_light

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_directional_light

      // Set new value
      TYPE_SOA(RenderFlow, directional_light, DirectionalLight) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_directional_light

      // LOW_CODEGEN::END::CUSTOM:SETTER_directional_light

      broadcast_observable(N(directional_light));
    }

    Util::List<PointLight> &RenderFlow::get_point_lights() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_point_lights

      // LOW_CODEGEN::END::CUSTOM:GETTER_point_lights

      return TYPE_SOA(RenderFlow, point_lights,
                      Util::List<PointLight>);
    }

    Low::Util::Name RenderFlow::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(RenderFlow, name, Low::Util::Name);
    }
    void RenderFlow::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(RenderFlow, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    RenderFlow RenderFlow::make(Util::Name p_Name,
                                Interface::Context p_Context,
                                Util::Yaml::Node &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

      RenderFlow l_RenderFlow = RenderFlow::make(p_Name);
      l_RenderFlow.get_dimensions() = p_Context.get_dimensions();
      l_RenderFlow.set_context(p_Context);

      {
        l_RenderFlow.set_camera_far_plane(100.0f);
        l_RenderFlow.set_camera_near_plane(0.1f);
        l_RenderFlow.set_camera_fov(45.f);
        l_RenderFlow.set_camera_position(
            Math::Vector3(0.0f, 0.0f, 0.0f));
        l_RenderFlow.set_camera_direction(
            Math::Vector3(0.0f, 0.0f, 1.0f));
      }

      {
        Backend::BufferCreateParams l_Params;
        l_Params.bufferSize = sizeof(RenderFlowFrameInfo);
        l_Params.context = &p_Context.get_context();
        l_Params.usageFlags =
            LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT;
        l_Params.data = nullptr;

        l_RenderFlow.set_frame_info_buffer(Resource::Buffer::make(
            N(RenderFlowFrameInfoBuffer), l_Params));
      }

      Util::List<ResourceConfig> l_ResourceConfigs;
      if (p_Config["resources"]) {
        parse_resource_configs(p_Config["resources"],
                               l_ResourceConfigs);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_directional_light_info);
        l_ResourceConfig.buffer.size =
            sizeof(DirectionalLightShaderInfo);
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_point_light_info);
        l_ResourceConfig.buffer.size =
            16 + (sizeof(PointLightShaderInfo) *
                  LOW_RENDERER_POINTLIGHT_COUNT);
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_particle_draw_info);
        l_ResourceConfig.buffer.usageFlags =
            LOW_RENDERER_BUFFER_USAGE_INDIRECT;
        l_ResourceConfig.buffer.size =
            Backend::callbacks()
                .get_draw_indexed_indirect_info_size() *
            LOW_RENDERER_MAX_PARTICLES;
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_particle_render_info);
        l_ResourceConfig.buffer.size =
            Backend::callbacks()
                .get_draw_indexed_indirect_info_size() *
            LOW_RENDERER_MAX_PARTICLES; // TODO: Change size
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_particle_draw_count);
        l_ResourceConfig.buffer.usageFlags =
            LOW_RENDERER_BUFFER_USAGE_INDIRECT;
        l_ResourceConfig.buffer.size = sizeof(uint32_t);
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }

      l_RenderFlow.get_resources().initialize(
          l_ResourceConfigs, p_Context, l_RenderFlow);

      {
        Util::List<Backend::PipelineResourceDescription> l_Resources;

        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_RenderFlowFrameInfo);
          l_Resource.step = Backend::ResourcePipelineStep::ALL;
          l_Resource.type = Backend::ResourceType::CONSTANT_BUFFER;
          l_Resource.arraySize = 1;
          l_Resources.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_DirectionalLightInfo);
          l_Resource.step = Backend::ResourcePipelineStep::ALL;
          l_Resource.type = Backend::ResourceType::CONSTANT_BUFFER;
          l_Resource.arraySize = 1;
          l_Resources.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_PointLightInfo);
          l_Resource.step = Backend::ResourcePipelineStep::ALL;
          l_Resource.type = Backend::ResourceType::CONSTANT_BUFFER;
          l_Resource.arraySize = 1;
          l_Resources.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_ParticleDrawBuffer);
          l_Resource.step = Backend::ResourcePipelineStep::ALL;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_Resource.arraySize = 1;
          l_Resources.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_ParticleRenderBuffer);
          l_Resource.step = Backend::ResourcePipelineStep::ALL;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_Resource.arraySize = 1;
          l_Resources.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_ParticleDrawCountBuffer);
          l_Resource.step = Backend::ResourcePipelineStep::ALL;
          l_Resource.type = Backend::ResourceType::BUFFER;
          l_Resource.arraySize = 1;
          l_Resources.push_back(l_Resource);
        }

        l_RenderFlow.set_resource_signature(
            Interface::PipelineResourceSignature::make(
                N(RenderFlowSignature), p_Context, 1, l_Resources));

        l_RenderFlow.get_resource_signature()
            .set_constant_buffer_resource(
                N(u_RenderFlowFrameInfo), 0,
                l_RenderFlow.get_frame_info_buffer());
        l_RenderFlow.get_resource_signature()
            .set_constant_buffer_resource(
                N(u_DirectionalLightInfo), 0,
                l_RenderFlow.get_resources().get_buffer_resource(
                    N(_directional_light_info)));
        l_RenderFlow.get_resource_signature()
            .set_constant_buffer_resource(
                N(u_PointLightInfo), 0,
                l_RenderFlow.get_resources().get_buffer_resource(
                    N(_point_light_info)));

        l_RenderFlow.get_resource_signature().set_buffer_resource(
            N(u_ParticleDrawBuffer), 0,
            l_RenderFlow.get_resources().get_buffer_resource(
                N(_particle_draw_info)));
        l_RenderFlow.get_resource_signature().set_buffer_resource(
            N(u_ParticleRenderBuffer), 0,
            l_RenderFlow.get_resources().get_buffer_resource(
                N(_particle_render_info)));
        l_RenderFlow.get_resource_signature().set_buffer_resource(
            N(u_ParticleDrawCountBuffer), 0,
            l_RenderFlow.get_resources().get_buffer_resource(
                N(_particle_draw_count)));
      }

      for (auto it = p_Config["steps"].begin();
           it != p_Config["steps"].end(); ++it) {
        Util::Yaml::Node i_StepEntry = *it;
        Util::Name i_StepName = LOW_YAML_AS_NAME(i_StepEntry["name"]);

        bool i_Found = false;
        for (ComputeStepConfig i_Config :
             ComputeStepConfig::ms_LivingInstances) {
          if (i_Config.get_name() == i_StepName) {
            i_Found = true;
            ComputeStep i_Step = ComputeStep::make(
                i_Config.get_name(), p_Context, i_Config);
            i_Step.prepare(l_RenderFlow);

            l_RenderFlow.get_steps().push_back(i_Step);
            break;
          }
        }

        if (!i_Found) {
          for (GraphicsStepConfig i_Config :
               GraphicsStepConfig ::ms_LivingInstances) {
            if (i_Config.get_name() == i_StepName) {
              i_Found = true;
              GraphicsStep i_Step = GraphicsStep::make(
                  i_Config.get_name(), p_Context, i_Config);
              i_Step.prepare(l_RenderFlow);

              l_RenderFlow.get_steps().push_back(i_Step);
              break;
            }
          }
        }

        LOW_ASSERT(i_Found, "Could not find renderstep");
      }

      l_RenderFlow.set_output_image(
          l_RenderFlow.get_resources().get_image_resource(
              LOW_YAML_AS_NAME(p_Config["output_image"])));

      return l_RenderFlow;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    void RenderFlow::clear_renderbojects()
    {
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_clear_renderbojects

      for (Util::Handle i_Step : get_steps()) {
        if (i_Step.get_type() == GraphicsStep::TYPE_ID) {
          GraphicsStep i_GraphicsStep = i_Step.get_id();
          i_GraphicsStep.clear_renderobjects();
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_clear_renderbojects
    }

    void RenderFlow::execute()
    {
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute

      Util::String l_ProfileString = get_name().c_str();
      l_ProfileString += " (RenderFlow execute)";
      LOW_PROFILE_CPU("Renderer", l_ProfileString.c_str());
      if (get_context().is_debug_enabled()) {
        Util::String l_RenderDocLabel =
            Util::String("RenderFlow - ") + get_name().c_str();
        LOW_RENDERER_BEGIN_RENDERDOC_SECTION(
            get_context().get_context(), l_RenderDocLabel,
            Math::Color(0.341f, 0.4249f, 0.2341f, 1.0f));
      }

      Math::Matrix4x4 l_ProjectionMatrix = glm::perspective(
          glm::radians(get_camera_fov()),
          ((float)get_dimensions().x) / ((float)get_dimensions().y),
          get_camera_near_plane(), get_camera_far_plane());

      l_ProjectionMatrix[1][1] *= -1.0f; // Convert from OpenGL y-axis
                                         // to Vulkan y-axis

      Math::Matrix4x4 l_ViewMatrix =
          glm::lookAt(get_camera_position(),
                      get_camera_position() + get_camera_direction(),
                      LOW_VECTOR3_UP);

      set_projection_matrix(l_ProjectionMatrix);
      set_view_matrix(l_ViewMatrix);

      {
        Math::Vector2 l_InverseDimensions = {
            1.0f / ((float)get_dimensions().x),
            1.0f / ((float)get_dimensions().y)};

        RenderFlowFrameInfo l_FrameInfo;
        l_FrameInfo.cameraPosition = get_camera_position();
        l_FrameInfo.inverseDimensions = l_InverseDimensions;
        l_FrameInfo.projectionMatrix = l_ProjectionMatrix;
        l_FrameInfo.viewMatrix = l_ViewMatrix; //

        get_frame_info_buffer().set(&l_FrameInfo);
      }

      get_resource_signature().commit();

      for (Util::Handle i_Step : get_steps()) {
        if (i_Step.get_type() == ComputeStep::TYPE_ID) {
          ComputeStep i_ComputeStep = i_Step.get_id();
          i_ComputeStep.execute(*this);
        } else if (i_Step.get_type() == GraphicsStep::TYPE_ID) {
          GraphicsStep i_GraphicsStep = i_Step.get_id();
          i_GraphicsStep.execute(*this, l_ProjectionMatrix,
                                 l_ViewMatrix);
        } else {
          LOW_ASSERT(false, "Unknown step type");
        }
      }

      if (get_context().is_debug_enabled()) {
        LOW_RENDERER_END_RENDERDOC_SECTION(
            get_context().get_context());
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

    void RenderFlow::update_dimensions(Math::UVector2 &p_Dimensions)
    {
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_dimensions

      get_dimensions() = p_Dimensions;

      get_resources().update_dimensions(*this);

      for (Util::Handle i_Step : get_steps()) {
        if (i_Step.get_type() == ComputeStep::TYPE_ID) {
          ComputeStep i_ComputeStep = i_Step.get_id();
          i_ComputeStep.update_dimensions(*this);
        } else if (i_Step.get_type() == GraphicsStep::TYPE_ID) {
          GraphicsStep i_GraphicsStep = i_Step.get_id();
          i_GraphicsStep.update_dimensions(*this);
        } else {
          LOW_ASSERT(false, "Unknown step type");
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_dimensions
    }

    void
    RenderFlow::register_renderobject(RenderObject &p_RenderObject)
    {
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_register_renderobject

      for (Util::Handle i_Step : get_steps()) {
        if (i_Step.get_type() == GraphicsStep::TYPE_ID) {
          GraphicsStep i_GraphicsStep = i_Step.get_id();
          i_GraphicsStep.register_renderobject(p_RenderObject);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_register_renderobject
    }

    Resource::Image
    RenderFlow::get_previous_output_image(Util::Handle p_Step)
    {
      Low::Util::HandleLock<RenderFlow> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_previous_output_image

      for (uint32_t i = 0u; i < get_steps().size(); ++i) {
        if (p_Step.get_id() == get_steps()[i].get_id()) {
          if (i == 0u) {
            return 0ull;
          }
          Util::Handle i_Step = get_steps()[i - 1];
          if (i_Step.get_type() == ComputeStep::TYPE_ID) {
            ComputeStep i_ComputeStep = i_Step.get_id();
            return i_ComputeStep.get_output_image();
          } else if (i_Step.get_type() == GraphicsStep::TYPE_ID) {
            GraphicsStep i_GraphicsStep = i_Step.get_id();
            return i_GraphicsStep.get_output_image();
          }
          LOW_ASSERT(false, "Unknown renderstep type");
        }
      }

      // If the current renderstep could not be found just
      // return the last output image
      if (!get_steps().empty()) {
        Util::Handle l_Step = get_steps().back();
        if (l_Step.get_type() == ComputeStep::TYPE_ID) {
          ComputeStep l_ComputeStep = l_Step.get_id();
          return l_ComputeStep.get_output_image();
        } else if (l_Step.get_type() == GraphicsStep::TYPE_ID) {
          GraphicsStep l_GraphicsStep = l_Step.get_id();
          return l_GraphicsStep.get_output_image();
        }
      }
      return 0;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_previous_output_image
    }

    uint32_t RenderFlow::create_instance(
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

    u32 RenderFlow::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for RenderFlow.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, RenderFlow::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool RenderFlow::get_page_for_index(const u32 p_Index,
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
