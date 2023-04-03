#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererInterface.h"
#include "LowRendererResourceRegistry.h"
#include "LowRendererMesh.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
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
      Math::Quaternion camera_rotation;
      float camera_fov;
      float camera_near_plane;
      float camera_far_plane;
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

      bool is_alive() const;

      static uint32_t get_capacity();

      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      Math::UVector2 &get_dimensions() const;

      Resource::Image get_output_image() const;
      void set_output_image(Resource::Image p_Value);

      Util::List<Util::Handle> &get_steps() const;
      void set_steps(Util::List<Util::Handle> &p_Value);

      ResourceRegistry &get_resources() const;

      Resource::Buffer get_frame_info_buffer() const;

      Interface::PipelineResourceSignature get_resource_signature() const;

      Math::Vector3 &get_camera_position() const;
      void set_camera_position(Math::Vector3 &p_Value);

      Math::Quaternion &get_camera_rotation() const;
      void set_camera_rotation(Math::Quaternion &p_Value);

      float get_camera_fov() const;
      void set_camera_fov(float p_Value);

      float get_camera_near_plane() const;
      void set_camera_near_plane(float p_Value);

      float get_camera_far_plane() const;
      void set_camera_far_plane(float p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static RenderFlow make(Util::Name p_Name, Interface::Context p_Context,
                             Util::Yaml::Node &p_Config);
      void execute();
      void update_dimensions(Math::UVector2 &p_Dimensions);

    private:
      Interface::Context get_context() const;
      void set_context(Interface::Context p_Value);
      void set_frame_info_buffer(Resource::Buffer p_Value);
      void set_resource_signature(Interface::PipelineResourceSignature p_Value);
    };
  } // namespace Renderer
} // namespace Low
