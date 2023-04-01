#include "LowRendererRenderFlow.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowRendererComputeStep.h"
#include "LowRendererGraphicsStep.h"

namespace Low {
  namespace Renderer {
    const uint16_t RenderFlow::TYPE_ID = 1;
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

    RenderFlow RenderFlow::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      RenderFlowData *l_DataPtr =
          (RenderFlowData *)&ms_Buffer[l_Index * sizeof(RenderFlowData)];
      new (l_DataPtr) RenderFlowData();

      RenderFlow l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderFlow::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, RenderFlow, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

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
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void RenderFlow::initialize()
    {
      initialize_buffer(&ms_Buffer, RenderFlowData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_RenderFlow);
      LOW_PROFILE_ALLOC(type_slots_RenderFlow);
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

    bool RenderFlow::is_alive() const
    {
      return m_Data.m_Type == RenderFlow::TYPE_ID &&
             check_alive(ms_Slots, RenderFlow::get_capacity());
    }

    uint32_t RenderFlow::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity =
            Low::Util::Config::get_capacity(N(LowRenderer), N(RenderFlow));
      }
      return l_Capacity;
    }

    Interface::Context RenderFlow::get_context() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, context, Interface::Context);
    }
    void RenderFlow::set_context(Interface::Context p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, context, Interface::Context) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_context
      // LOW_CODEGEN::END::CUSTOM:SETTER_context
    }

    Math::UVector2 &RenderFlow::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, dimensions, Math::UVector2);
    }

    Resource::Image RenderFlow::get_output_image() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, output_image, Resource::Image);
    }
    void RenderFlow::set_output_image(Resource::Image p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, output_image, Resource::Image) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_output_image
      // LOW_CODEGEN::END::CUSTOM:SETTER_output_image
    }

    Util::List<Util::Handle> &RenderFlow::get_steps() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, steps, Util::List<Util::Handle>);
    }
    void RenderFlow::set_steps(Util::List<Util::Handle> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, steps, Util::List<Util::Handle>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_steps
      // LOW_CODEGEN::END::CUSTOM:SETTER_steps
    }

    ResourceRegistry &RenderFlow::get_resources() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, resources, ResourceRegistry);
    }

    Resource::Buffer RenderFlow::get_frame_info_buffer() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, frame_info_buffer, Resource::Buffer);
    }
    void RenderFlow::set_frame_info_buffer(Resource::Buffer p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, frame_info_buffer, Resource::Buffer) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_frame_info_buffer
      // LOW_CODEGEN::END::CUSTOM:SETTER_frame_info_buffer
    }

    Interface::PipelineResourceSignature
    RenderFlow::get_resource_signature() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, resource_signature,
                      Interface::PipelineResourceSignature);
    }
    void RenderFlow::set_resource_signature(
        Interface::PipelineResourceSignature p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, resource_signature,
               Interface::PipelineResourceSignature) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource_signature
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource_signature
    }

    Math::Vector3 &RenderFlow::get_camera_position() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, camera_position, Math::Vector3);
    }
    void RenderFlow::set_camera_position(Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, camera_position, Math::Vector3) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_position
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_position
    }

    Math::Quaternion &RenderFlow::get_camera_rotation() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, camera_rotation, Math::Quaternion);
    }
    void RenderFlow::set_camera_rotation(Math::Quaternion &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, camera_rotation, Math::Quaternion) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_rotation
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_rotation
    }

    float RenderFlow::get_camera_fov() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, camera_fov, float);
    }
    void RenderFlow::set_camera_fov(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, camera_fov, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_fov
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_fov
    }

    float RenderFlow::get_camera_near_plane() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, camera_near_plane, float);
    }
    void RenderFlow::set_camera_near_plane(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, camera_near_plane, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_near_plane
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_near_plane
    }

    float RenderFlow::get_camera_far_plane() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, camera_far_plane, float);
    }
    void RenderFlow::set_camera_far_plane(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(RenderFlow, camera_far_plane, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_camera_far_plane
      // LOW_CODEGEN::END::CUSTOM:SETTER_camera_far_plane
    }

    Low::Util::Name RenderFlow::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(RenderFlow, name, Low::Util::Name);
    }
    void RenderFlow::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

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
        l_RenderFlow.set_camera_rotation(
            Math::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
      }

      {
        Backend::BufferCreateParams l_Params;
        l_Params.bufferSize = sizeof(Math::Vector2);
        l_Params.context = &p_Context.get_context();
        l_Params.usageFlags = LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT;

        Math::Vector2 l_InverseDimensions = {
            1.0f / ((float)l_RenderFlow.get_dimensions().x),
            1.0f / ((float)l_RenderFlow.get_dimensions().y)};

        l_Params.data = &l_InverseDimensions;

        l_RenderFlow.set_frame_info_buffer(
            Resource::Buffer::make(N(RenderFlowFrameInfoBuffer), l_Params));
      }

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

        l_RenderFlow.set_resource_signature(
            Interface::PipelineResourceSignature::make(
                N(RenderFlowSignature), p_Context, 1, l_Resources));

        l_RenderFlow.get_resource_signature().set_constant_buffer_resource(
            N(u_RenderFlowFrameInfo), 0, l_RenderFlow.get_frame_info_buffer());
      }

      if (p_Config["resources"]) {
        Util::List<ResourceConfig> l_ResourceConfigs;
        parse_resource_configs(p_Config["resources"], l_ResourceConfigs);
        l_RenderFlow.get_resources().initialize(l_ResourceConfigs, p_Context,
                                                l_RenderFlow);
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

      get_resource_signature().commit();

      Math::Matrix4x4 l_ProjectionMatrix = glm::perspective(
          glm::radians(get_camera_fov()),
          ((float)get_dimensions().x) / ((float)get_dimensions().y),
          get_camera_near_plane(), get_camera_far_plane());

      l_ProjectionMatrix[1][1] *=
          -1; // Convert from OpenGL y-axis to Vulkan y-axis
      l_ProjectionMatrix[0][0] *= -1; // Convert to left handed system

      Math::Matrix4x4 l_ViewMatrix =
          glm::lookAt(get_camera_position(),
                      Math::VectorUtil::direction(get_camera_rotation()),
                      Math::Vector3(0.f, 1.f, 0.f));

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

      Math::Vector2 l_InverseDimensions = {1.0f / ((float)get_dimensions().x),
                                           1.0f / ((float)get_dimensions().y)};

      get_frame_info_buffer().set(&l_InverseDimensions);

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

  } // namespace Renderer
} // namespace Low
