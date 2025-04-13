#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererGlobals.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API RenderViewData
    {
      Low::Math::Vector3 camera_position;
      Low::Math::Vector3 camera_direction;
      uint64_t render_target_handle;
      Low::Util::List<RenderEntry> render_entries;
      uint64_t view_info_handle;
      Low::Math::UVector2 dimensions;
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

      Low::Util::List<RenderEntry> &get_render_entries() const;

      uint64_t get_view_info_handle() const;
      void set_view_info_handle(uint64_t p_Value);

      Low::Math::UVector2 &get_dimensions() const;
      void set_dimensions(Low::Math::UVector2 &p_Value);
      void set_dimensions(float p_X, float p_Y);
      void set_dimensions_x(float p_Value);
      void set_dimensions_y(float p_Value);

      bool is_camera_dirty() const;
      void set_camera_dirty(bool p_Value);

      bool is_dimensions_dirty() const;
      void set_dimensions_dirty(bool p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      bool add_render_entry(RenderObject p_RenderObject,
                            uint32_t p_Slot, MeshInfo p_MeshInfo);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      bool insert_render_entry(RenderEntry &p_RenderEntry);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
