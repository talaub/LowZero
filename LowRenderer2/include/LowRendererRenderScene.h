#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererGlobals.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct DrawCommand;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API RenderScene : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Low::Util::List<DrawCommand> draw_commands;
        Low::Util::Set<u32> pointlight_deleted_slots;
        uint64_t data_handle;
        Low::Math::Vector3 directional_light_direction;
        Low::Math::ColorRGB directional_light_color;
        float directional_light_intensity;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(Data);
        }
      };

    public:
      static Low::Util::UniqueLock<Low::Util::SharedMutex>
          ms_PagesLock;
      static Low::Util::SharedMutex ms_PagesMutex;
      static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

      static Low::Util::List<RenderScene> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      RenderScene();
      RenderScene(uint64_t p_Id);
      RenderScene(RenderScene &p_Copy);

      static RenderScene make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit RenderScene(const RenderScene &p_Copy)
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
      static RenderScene *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static RenderScene create_handle_by_index(u32 p_Index);

      static RenderScene find_by_index(uint32_t p_Index);
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

      RenderScene duplicate(Low::Util::Name p_Name) const;
      static RenderScene duplicate(RenderScene p_Handle,
                                   Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static RenderScene find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        RenderScene l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        RenderScene l_RenderScene = p_Handle.get_id();
        l_RenderScene.destroy();
      }

      Low::Util::List<DrawCommand> &get_draw_commands() const;

      Low::Util::Set<u32> &get_pointlight_deleted_slots() const;

      uint64_t get_data_handle() const;
      void set_data_handle(uint64_t p_Value);

      Low::Math::Vector3 &get_directional_light_direction() const;
      void
      set_directional_light_direction(Low::Math::Vector3 &p_Value);
      void set_directional_light_direction(float p_X, float p_Y,
                                           float p_Z);
      void set_directional_light_direction_x(float p_Value);
      void set_directional_light_direction_y(float p_Value);
      void set_directional_light_direction_z(float p_Value);

      Low::Math::ColorRGB &get_directional_light_color() const;
      void set_directional_light_color(Low::Math::ColorRGB &p_Value);
      void set_directional_light_color(float p_X, float p_Y,
                                       float p_Z);
      void set_directional_light_color_x(float p_Value);
      void set_directional_light_color_y(float p_Value);
      void set_directional_light_color_z(float p_Value);

      float get_directional_light_intensity() const;
      void set_directional_light_intensity(float p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      bool
      insert_draw_command(Low::Renderer::DrawCommand p_DrawCommand);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);
      static u32 create_page();
      void set_pointlight_deleted_slots(Low::Util::Set<u32> &p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
