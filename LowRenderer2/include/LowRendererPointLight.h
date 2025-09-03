#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct RenderScene;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API PointLightData
    {
      Low::Math::Vector3 world_position;
      Low::Math::ColorRGB color;
      float intensity;
      float range;
      uint64_t render_scene_handle;
      uint32_t slot;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(PointLightData);
      }
    };

    struct LOW_RENDERER2_API PointLight : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<PointLight> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      PointLight();
      PointLight(uint64_t p_Id);
      PointLight(PointLight &p_Copy);

    private:
      static PointLight make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit PointLight(const PointLight &p_Copy)
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
      static PointLight *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static PointLight create_handle_by_index(u32 p_Index);

      static PointLight find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Function<void(Low::Util::Handle,
                                           Low::Util::Name)>
                      p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      PointLight duplicate(Low::Util::Name p_Name) const;
      static PointLight duplicate(PointLight p_Handle,
                                  Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static PointLight find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == PointLight::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        PointLight l_PointLight = p_Handle.get_id();
        l_PointLight.destroy();
      }

      Low::Math::Vector3 &get_world_position() const;
      void set_world_position(Low::Math::Vector3 &p_Value);
      void set_world_position(float p_X, float p_Y, float p_Z);
      void set_world_position_x(float p_Value);
      void set_world_position_y(float p_Value);
      void set_world_position_z(float p_Value);

      Low::Math::ColorRGB &get_color() const;
      void set_color(Low::Math::ColorRGB &p_Value);

      float get_intensity() const;
      void set_intensity(float p_Value);

      float get_range() const;
      void set_range(float p_Value);

      uint64_t get_render_scene_handle() const;

      uint32_t get_slot() const;
      void set_slot(uint32_t p_Value);

      void mark_dirty();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static PointLight
      make(Low::Renderer::RenderScene p_RenderScene);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_render_scene_handle(uint64_t p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
    public:
      static Low::Util::Set<Low::Renderer::PointLight> ms_Dirty;
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
