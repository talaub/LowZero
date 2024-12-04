#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererInterface.h"
#include "LowRendererResourceRegistry.h"
#include "LowRendererMesh.h"
#include "LowRendererLights.h"
#include "LowRendererExposedObjects.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

#define LOW_RENDERER_POINTLIGHT_COUNT 8
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    struct RenderFlowFrameInfo
    {
      alignas(16) Math::Vector2 inverseDimensions;
      alignas(16) Math::Vector3 cameraPosition;
      alignas(16) Math::Matrix4x4 projectionMatrix;
      alignas(16) Math::Matrix4x4 viewMatrix;
    };
    struct DirectionalLightShaderInfo
    {
      alignas(16) Math::Matrix4x4 lightSpaceMatrix;
      alignas(16) Math::Vector4 atlasBounds;
      alignas(16) Math::Vector3 direction;
      alignas(16) Math::Vector3 color;
    };
    struct PointLightShaderInfo
    {
      alignas(16) Math::Vector3 position;
      alignas(16) Math::Vector3 color;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API RenderFlowData
    {
      Interface::Context context;
      Math::UVector2 dimensions;
      Resource::Image output_image;
      Util::List<Util::Handle> steps;
      ResourceRegistry resources;
      Resource::Buffer frame_info_buffer;
      Interface::PipelineResourceSignature resource_signature;
      Math::Vector3 camera_position;
      Math::Vector3 camera_direction;
      float camera_fov;
      float camera_near_plane;
      float camera_far_plane;
      Math::Matrix4x4 projection_matrix;
      Math::Matrix4x4 view_matrix;
      DirectionalLight directional_light;
      Util::List<PointLight> point_lights;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(RenderFlowData);
      }
    };

    struct LOW_RENDERER_API RenderFlow : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<RenderFlow> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      RenderFlow();
      RenderFlow(uint64_t p_Id);
      RenderFlow(RenderFlow &p_Copy);

    private:
      static RenderFlow make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit RenderFlow(const RenderFlow &p_Copy)
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
      static RenderFlow *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static RenderFlow find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      RenderFlow duplicate(Low::Util::Name p_Name) const;
      static RenderFlow duplicate(RenderFlow p_Handle,
                                  Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static RenderFlow find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == RenderFlow::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        RenderFlow l_RenderFlow = p_Handle.get_id();
        l_RenderFlow.destroy();
      }

      Math::UVector2 &get_dimensions() const;

      Resource::Image get_output_image() const;
      void set_output_image(Resource::Image p_Value);

      Util::List<Util::Handle> &get_steps() const;
      void set_steps(Util::List<Util::Handle> &p_Value);

      ResourceRegistry &get_resources() const;

      Resource::Buffer get_frame_info_buffer() const;

      Interface::PipelineResourceSignature
      get_resource_signature() const;

      Math::Vector3 &get_camera_position() const;
      void set_camera_position(Math::Vector3 &p_Value);

      Math::Vector3 &get_camera_direction() const;
      void set_camera_direction(Math::Vector3 &p_Value);

      float get_camera_fov() const;
      void set_camera_fov(float p_Value);

      float get_camera_near_plane() const;
      void set_camera_near_plane(float p_Value);

      float get_camera_far_plane() const;
      void set_camera_far_plane(float p_Value);

      Math::Matrix4x4 &get_projection_matrix() const;

      Math::Matrix4x4 &get_view_matrix() const;

      DirectionalLight &get_directional_light() const;
      void set_directional_light(DirectionalLight &p_Value);

      Util::List<PointLight> &get_point_lights() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static RenderFlow make(Util::Name p_Name,
                             Interface::Context p_Context,
                             Util::Yaml::Node &p_Config);
      void clear_renderbojects();
      void execute();
      void update_dimensions(Math::UVector2 &p_Dimensions);
      void register_renderobject(RenderObject &p_RenderObject);
      Resource::Image get_previous_output_image(Util::Handle p_Step);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      Interface::Context get_context() const;
      void set_context(Interface::Context p_Value);
      void set_frame_info_buffer(Resource::Buffer p_Value);
      void set_resource_signature(
          Interface::PipelineResourceSignature p_Value);
      void set_projection_matrix(Math::Matrix4x4 &p_Value);
      void set_view_matrix(Math::Matrix4x4 &p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
