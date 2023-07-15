#include "LowRendererRenderFlow.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowRendererComputeStep.h"
#include "LowRendererGraphicsStep.h"

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t RenderFlow::TYPE_ID = 9;
    uint32_t RenderFlow::ms_Capacity = 0u;
    uint8_t *RenderFlow::ms_Buffer = 0;
    Low::Util::Instances::Slot *RenderFlow::ms_Slots = 0;
    Low::Util::List<RenderFlow> RenderFlow::ms_LivingInstances =
        Low::Util::List<RenderFlow>();

    RenderFlow::RenderFlow() : Low::Util::Handle(0ull)
    {
    }
    RenderFlow::RenderFlow(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    RenderFlow::RenderFlow(RenderFlow &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle RenderFlow::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    RenderFlow RenderFlow::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      RenderFlow l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderFlow::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, context,
                              Interface::Context)) Interface::Context();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, dimensions, Math::UVector2))
          Math::UVector2();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, output_image,
                              Resource::Image)) Resource::Image();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, steps,
                              Util::List<Util::Handle>))
          Util::List<Util::Handle>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, resources,
                              ResourceRegistry)) ResourceRegistry();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, frame_info_buffer,
                              Resource::Buffer)) Resource::Buffer();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, resource_signature,
                              Interface::PipelineResourceSignature))
          Interface::PipelineResourceSignature();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, camera_position,
                              Math::Vector3)) Math::Vector3();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, camera_direction,
                              Math::Vector3)) Math::Vector3();
      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, camera_fov, float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, camera_near_plane, float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, camera_far_plane, float) = 0.0f;
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, projection_matrix,
                              Math::Matrix4x4)) Math::Matrix4x4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, view_matrix,
                              Math::Matrix4x4)) Math::Matrix4x4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, directional_light,
                              DirectionalLight)) DirectionalLight();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, point_lights,
                              Util::List<PointLight>)) Util::List<PointLight>();
      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderFlow::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const RenderFlow *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void RenderFlow::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(RenderFlow));

      initialize_buffer(&ms_Buffer, RenderFlowData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_RenderFlow);
      LOW_PROFILE_ALLOC(type_slots_RenderFlow);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(RenderFlow);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &RenderFlow::is_alive;
      l_TypeInfo.destroy = &RenderFlow::destroy;
      l_TypeInfo.serialize = &RenderFlow::serialize;
      l_TypeInfo.deserialize = &RenderFlow::deserialize;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &RenderFlow::_make;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &RenderFlow::living_instances);
      l_TypeInfo.get_living_count = &RenderFlow::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(context);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, context);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Interface::Context::TYPE_ID;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow, dimensions,
                                            Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(output_image);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, output_image);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Resource::Image::TYPE_ID;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_output_image();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow, output_image,
                                            Resource::Image);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_output_image(*(Resource::Image *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(steps);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, steps);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_steps();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow, steps,
                                            Util::List<Util::Handle>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_steps(*(Util::List<Util::Handle> *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resources);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, resources);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_resources();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow, resources,
                                            ResourceRegistry);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(frame_info_buffer);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, frame_info_buffer);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Resource::Buffer::TYPE_ID;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_frame_info_buffer();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, frame_info_buffer, Resource::Buffer);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource_signature);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderFlowData, resource_signature);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Interface::PipelineResourceSignature::TYPE_ID;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_resource_signature();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, resource_signature,
              Interface::PipelineResourceSignature);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_position);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, camera_position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_camera_position();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            camera_position, Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_position(*(Math::Vector3 *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_direction);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, camera_direction);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_camera_direction();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            camera_direction, Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_direction(*(Math::Vector3 *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_fov);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, camera_fov);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_camera_fov();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow, camera_fov,
                                            float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_fov(*(float *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_near_plane);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, camera_near_plane);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_camera_near_plane();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            camera_near_plane, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_near_plane(*(float *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(camera_far_plane);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, camera_far_plane);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_camera_far_plane();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            camera_far_plane, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_camera_far_plane(*(float *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(projection_matrix);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, projection_matrix);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_projection_matrix();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow,
                                            projection_matrix, Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(view_matrix);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, view_matrix);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_view_matrix();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow, view_matrix,
                                            Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(directional_light);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, directional_light);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_directional_light();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderFlow, directional_light, DirectionalLight);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_directional_light(*(DirectionalLight *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(point_lights);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, point_lights);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_point_lights();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow, point_lights,
                                            Util::List<PointLight>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderFlowData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderFlow, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderFlow l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderFlow::cleanup()
    {
      Low::Util::List<RenderFlow> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_RenderFlow);
      LOW_PROFILE_FREE(type_slots_RenderFlow);
    }

    RenderFlow RenderFlow::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      RenderFlow l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderFlow::TYPE_ID;

      return l_Handle;
    }

    bool RenderFlow::is_alive() const
    {
      return m_Data.m_Type == RenderFlow::TYPE_ID &&
             check_alive(ms_Slots, RenderFlow::get_capacity());
    }

    uint32_t RenderFlow::get_capacity()
    {
      return ms_Capacity;
    }

    RenderFlow RenderFlow::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
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
        get_frame_info_buffer().serialize(p_Node["frame_info_buffer"]);
      }
      if (get_resource_signature().is_alive()) {
        get_resource_signature().serialize(p_Node["resource_signature"]);
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

    Low::Util::Handle RenderFlow::deserialize(Low::Util::Yaml::Node &p_Node,
                                              Low::Util::Handle p_Creator)
    {
      RenderFlow l_Handle = RenderFlow::make(N(RenderFlow));

      if (p_Node["context"]) {
        l_Handle.set_context(Interface::Context::deserialize(p_Node["context"],
                                                             l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["dimensions"]) {
      }
      if (p_Node["output_image"]) {
        l_Handle.set_output_image(Resource::Image::deserialize(
                                      p_Node["output_image"], l_Handle.get_id())
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
        l_Handle.set_camera_near_plane(p_Node["camera_near_plane"].as<float>());
      }
      if (p_Node["camera_far_plane"]) {
        l_Handle.set_camera_far_plane(p_Node["camera_far_plane"].as<float>());
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

    Interface::Context RenderFlow::get_context() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_context
      // LOW_CODEGEN::END::CUSTOM:GETTER_context

      return TYPE_SOA(RenderFlow, context, Interface::Context);
    }
    void RenderFlow::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_context
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_context

      // Set new value
      TYPE_SOA(RenderFlow, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
      // LOW_CODEGEN::END::CUSTOM:SETTER_context
    }

    Math::UVector2 &RenderFlow::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions

      return TYPE_SOA(RenderFlow, dimensions, Math::UVector2);
    }

    Resource::Image RenderFlow::get_output_image() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_output_image
      // LOW_CODEGEN::END::CUSTOM:GETTER_output_image

      return TYPE_SOA(RenderFlow, output_image, Resource::Image);
    }
    void RenderFlow::set_output_image(Resource::Image p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_output_image
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_output_image

      // Set new value
      TYPE_SOA(RenderFlow, output_image, Resource::Image) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_output_image
      // LOW_CODEGEN::END::CUSTOM:SETTER_output_image
    }

    Util::List<Util::Handle> &RenderFlow::get_steps() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_steps
      // LOW_CODEGEN::END::CUSTOM:GETTER_steps

      return TYPE_SOA(RenderFlow, steps, Util::List<Util::Handle>);
    }
    void RenderFlow::set_steps(Util::List<Util::Handle> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_steps
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_steps

      // Set new value
      TYPE_SOA(RenderFlow, steps, Util::List<Util::Handle>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_steps
      // LOW_CODEGEN::END::CUSTOM:SETTER_steps
    }

    ResourceRegistry &RenderFlow::get_resources() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resources
      // LOW_CODEGEN::END::CUSTOM:GETTER_resources

      return TYPE_SOA(RenderFlow, resources, ResourceRegistry);
    }

    Resource::Buffer RenderFlow::get_frame_info_buffer() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_frame_info_buffer
      // LOW_CODEGEN::END::CUSTOM:GETTER_frame_info_buffer

      return TYPE_SOA(RenderFlow, frame_info_buffer, Resource::Buffer);
    }
    void RenderFlow::set_frame_info_buffer(Resource::Buffer p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_frame_info_buffer
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_frame_info_buffer

      // Set new value
      TYPE_SOA(RenderFlow, frame_info_buffer, Resource::Buffer) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_frame_info_buffer
      // LOW_CODEGEN::END::CUSTOM:SETTER_frame_info_buffer
    }

    Interface::PipelineResourceSignature
    RenderFlow::get_resource_signature() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource_signature
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource_signature

      return TYPE_SOA(RenderFlow, resource_signature,
                      Interface::PipelineResourceSignature);
    }
    void RenderFlow::set_resource_signature(
        Interface::PipelineResourceSignature p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource_signature
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource_signature

      // Set new value
      TYPE_SOA(RenderFlow, resource_signature,
               Interface::PipelineResourceSignature) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource_signature
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource_signature
    }

    Math::Vector3 &RenderFlow::get_camera_position() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_position
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_position

      return TYPE_SOA(RenderFlow, camera_position, Math::Vector3);
    }
    void RenderFlow::set_camera_position(Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_position
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_position

      // Set new value
      TYPE_SOA(RenderFlow, camera_position, Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_position
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_position
    }

    Math::Vector3 &RenderFlow::get_camera_direction() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_direction
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_direction

      return TYPE_SOA(RenderFlow, camera_direction, Math::Vector3);
    }
    void RenderFlow::set_camera_direction(Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_direction
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_direction

      // Set new value
      TYPE_SOA(RenderFlow, camera_direction, Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_direction
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_direction
    }

    float RenderFlow::get_camera_fov() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_fov
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_fov

      return TYPE_SOA(RenderFlow, camera_fov, float);
    }
    void RenderFlow::set_camera_fov(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_fov
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_fov

      // Set new value
      TYPE_SOA(RenderFlow, camera_fov, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_fov
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_fov
    }

    float RenderFlow::get_camera_near_plane() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_near_plane
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_near_plane

      return TYPE_SOA(RenderFlow, camera_near_plane, float);
    }
    void RenderFlow::set_camera_near_plane(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_near_plane
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_near_plane

      // Set new value
      TYPE_SOA(RenderFlow, camera_near_plane, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_near_plane
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_near_plane
    }

    float RenderFlow::get_camera_far_plane() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_camera_far_plane
      // LOW_CODEGEN::END::CUSTOM:GETTER_camera_far_plane

      return TYPE_SOA(RenderFlow, camera_far_plane, float);
    }
    void RenderFlow::set_camera_far_plane(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_camera_far_plane
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_camera_far_plane

      // Set new value
      TYPE_SOA(RenderFlow, camera_far_plane, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_far_plane
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_far_plane
    }

    Math::Matrix4x4 &RenderFlow::get_projection_matrix() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_projection_matrix
      // LOW_CODEGEN::END::CUSTOM:GETTER_projection_matrix

      return TYPE_SOA(RenderFlow, projection_matrix, Math::Matrix4x4);
    }
    void RenderFlow::set_projection_matrix(Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_projection_matrix
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_projection_matrix

      // Set new value
      TYPE_SOA(RenderFlow, projection_matrix, Math::Matrix4x4) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_projection_matrix
      // LOW_CODEGEN::END::CUSTOM:SETTER_projection_matrix
    }

    Math::Matrix4x4 &RenderFlow::get_view_matrix() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_matrix
      // LOW_CODEGEN::END::CUSTOM:GETTER_view_matrix

      return TYPE_SOA(RenderFlow, view_matrix, Math::Matrix4x4);
    }
    void RenderFlow::set_view_matrix(Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_matrix
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_matrix

      // Set new value
      TYPE_SOA(RenderFlow, view_matrix, Math::Matrix4x4) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_matrix
      // LOW_CODEGEN::END::CUSTOM:SETTER_view_matrix
    }

    DirectionalLight &RenderFlow::get_directional_light() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_directional_light
      // LOW_CODEGEN::END::CUSTOM:GETTER_directional_light

      return TYPE_SOA(RenderFlow, directional_light, DirectionalLight);
    }
    void RenderFlow::set_directional_light(DirectionalLight &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_directional_light
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_directional_light

      // Set new value
      TYPE_SOA(RenderFlow, directional_light, DirectionalLight) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_directional_light
      // LOW_CODEGEN::END::CUSTOM:SETTER_directional_light
    }

    Util::List<PointLight> &RenderFlow::get_point_lights() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_point_lights
      // LOW_CODEGEN::END::CUSTOM:GETTER_point_lights

      return TYPE_SOA(RenderFlow, point_lights, Util::List<PointLight>);
    }

    Low::Util::Name RenderFlow::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(RenderFlow, name, Low::Util::Name);
    }
    void RenderFlow::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(RenderFlow, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    RenderFlow RenderFlow::make(Util::Name p_Name, Interface::Context p_Context,
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
        l_RenderFlow.set_camera_position(Math::Vector3(0.0f, 0.0f, 0.0f));
        l_RenderFlow.set_camera_direction(Math::Vector3(0.0f, 0.0f, 1.0f));
      }

      {
        Backend::BufferCreateParams l_Params;
        l_Params.bufferSize = sizeof(RenderFlowFrameInfo);
        l_Params.context = &p_Context.get_context();
        l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT;
        l_Params.data = nullptr;

        l_RenderFlow.set_frame_info_buffer(
            Resource::Buffer::make(N(RenderFlowFrameInfoBuffer), l_Params));
      }

      Util::List<ResourceConfig> l_ResourceConfigs;
      if (p_Config["resources"]) {
        parse_resource_configs(p_Config["resources"], l_ResourceConfigs);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_directional_light_info);
        l_ResourceConfig.buffer.size = sizeof(DirectionalLightShaderInfo);
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_point_light_info);
        l_ResourceConfig.buffer.size =
            16 + (sizeof(PointLightShaderInfo) * LOW_RENDERER_POINTLIGHT_COUNT);
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_particle_draw_info);
        l_ResourceConfig.buffer.usageFlags = LOW_RENDERER_BUFFER_USAGE_INDIRECT;
        l_ResourceConfig.buffer.size =
            Backend::callbacks().get_draw_indexed_indirect_info_size() *
            LOW_RENDERER_MAX_PARTICLES;
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_particle_render_info);
        l_ResourceConfig.buffer.size =
            Backend::callbacks().get_draw_indexed_indirect_info_size() *
            LOW_RENDERER_MAX_PARTICLES; // TODO: Change size
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }
      {
        ResourceConfig l_ResourceConfig;
        l_ResourceConfig.arraySize = 1;
        l_ResourceConfig.type = ResourceType::BUFFER;
        l_ResourceConfig.name = N(_particle_draw_count);
        l_ResourceConfig.buffer.usageFlags = LOW_RENDERER_BUFFER_USAGE_INDIRECT;
        l_ResourceConfig.buffer.size = sizeof(uint32_t);
        l_ResourceConfigs.push_back(l_ResourceConfig);
      }

      l_RenderFlow.get_resources().initialize(l_ResourceConfigs, p_Context,
                                              l_RenderFlow);

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

        l_RenderFlow.get_resource_signature().set_constant_buffer_resource(
            N(u_RenderFlowFrameInfo), 0, l_RenderFlow.get_frame_info_buffer());
        l_RenderFlow.get_resource_signature().set_constant_buffer_resource(
            N(u_DirectionalLightInfo), 0,
            l_RenderFlow.get_resources().get_buffer_resource(
                N(_directional_light_info)));
        l_RenderFlow.get_resource_signature().set_constant_buffer_resource(
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

      for (auto it = p_Config["steps"].begin(); it != p_Config["steps"].end();
           ++it) {
        Util::Yaml::Node i_StepEntry = *it;
        Util::Name i_StepName = LOW_YAML_AS_NAME(i_StepEntry["name"]);

        bool i_Found = false;
        for (ComputeStepConfig i_Config :
             ComputeStepConfig::ms_LivingInstances) {
          if (i_Config.get_name() == i_StepName) {
            i_Found = true;
            ComputeStep i_Step =
                ComputeStep::make(i_Config.get_name(), p_Context, i_Config);
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
              GraphicsStep i_Step =
                  GraphicsStep::make(i_Config.get_name(), p_Context, i_Config);
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
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
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

      l_ProjectionMatrix[1][1] *= -1.0f; // Convert from OpenGL y-axis to
                                         // Vulkan y-axis

      Math::Matrix4x4 l_ViewMatrix = glm::lookAt(
          get_camera_position(), get_camera_position() + get_camera_direction(),
          LOW_VECTOR3_UP);

      set_projection_matrix(l_ProjectionMatrix);
      set_view_matrix(l_ViewMatrix);

      {
        Math::Vector2 l_InverseDimensions = {1.0f / ((float)get_dimensions().x),
                                             1.0f /
                                                 ((float)get_dimensions().y)};

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
          i_GraphicsStep.execute(*this, l_ProjectionMatrix, l_ViewMatrix);
        } else {
          LOW_ASSERT(false, "Unknown step type");
        }
      }

      if (get_context().is_debug_enabled()) {
        LOW_RENDERER_END_RENDERDOC_SECTION(get_context().get_context());
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
    }

    void RenderFlow::update_dimensions(Math::UVector2 &p_Dimensions)
    {
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

    void RenderFlow::register_renderobject(RenderObject &p_RenderObject)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_register_renderobject
      for (Util::Handle i_Step : get_steps()) {
        if (i_Step.get_type() == GraphicsStep::TYPE_ID) {
          GraphicsStep i_GraphicsStep = i_Step.get_id();
          i_GraphicsStep.register_renderobject(p_RenderObject);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_register_renderobject
    }

    uint32_t RenderFlow::create_instance()
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

    void RenderFlow::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(RenderFlowData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, context) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, context) * (l_Capacity)],
               l_Capacity * sizeof(Interface::Context));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, dimensions) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, dimensions) * (l_Capacity)],
               l_Capacity * sizeof(Math::UVector2));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderFlowData, output_image) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderFlowData, output_image) * (l_Capacity)],
            l_Capacity * sizeof(Resource::Image));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr =
              new (&l_NewBuffer[offsetof(RenderFlowData, steps) *
                                    (l_Capacity + l_CapacityIncrease) +
                                (it->get_index() *
                                 sizeof(Util::List<Util::Handle>))])
                  Util::List<Util::Handle>();
          *i_ValPtr = it->get_steps();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, resources) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, resources) * (l_Capacity)],
               l_Capacity * sizeof(ResourceRegistry));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, frame_info_buffer) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, frame_info_buffer) *
                          (l_Capacity)],
               l_Capacity * sizeof(Resource::Buffer));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, resource_signature) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, resource_signature) *
                          (l_Capacity)],
               l_Capacity * sizeof(Interface::PipelineResourceSignature));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, camera_position) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, camera_position) *
                          (l_Capacity)],
               l_Capacity * sizeof(Math::Vector3));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, camera_direction) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, camera_direction) *
                          (l_Capacity)],
               l_Capacity * sizeof(Math::Vector3));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, camera_fov) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, camera_fov) * (l_Capacity)],
               l_Capacity * sizeof(float));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, camera_near_plane) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, camera_near_plane) *
                          (l_Capacity)],
               l_Capacity * sizeof(float));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, camera_far_plane) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, camera_far_plane) *
                          (l_Capacity)],
               l_Capacity * sizeof(float));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, projection_matrix) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, projection_matrix) *
                          (l_Capacity)],
               l_Capacity * sizeof(Math::Matrix4x4));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, view_matrix) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, view_matrix) * (l_Capacity)],
               l_Capacity * sizeof(Math::Matrix4x4));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, directional_light) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, directional_light) *
                          (l_Capacity)],
               l_Capacity * sizeof(DirectionalLight));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(RenderFlowData, point_lights) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() * sizeof(Util::List<PointLight>))])
              Util::List<PointLight>();
          *i_ValPtr = it->get_point_lights();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderFlowData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderFlowData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for RenderFlow from "
                    << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Renderer
} // namespace Low
