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
    struct DrawCommand;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API RenderSceneData
    {
      Low::Util::List<DrawCommand> draw_commands;
      Low::Util::Set<u32> pointlight_deleted_slots;
      uint64_t data_handle;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(RenderSceneData);
      }
    };

    struct LOW_RENDERER2_API RenderScene : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

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

      static RenderScene find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

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
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == RenderScene::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
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

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      bool
      insert_draw_command(Low::Renderer::DrawCommand p_DrawCommand);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_pointlight_deleted_slots(Low::Util::Set<u32> &p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
