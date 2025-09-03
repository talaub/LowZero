#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererGpuSubmesh.h"
#include "LowRendererMaterial.h"
#include "LowRendererRenderObject.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct RenderScene;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API DrawCommandData
    {
      Low::Math::Matrix4x4 world_transform;
      Low::Renderer::GpuSubmesh submesh;
      uint32_t slot;
      Low::Renderer::RenderObject render_object;
      Low::Renderer::Material material;
      bool uploaded;
      uint64_t render_scene_handle;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(DrawCommandData);
      }
    };

    struct LOW_RENDERER2_API DrawCommand : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<DrawCommand> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      DrawCommand();
      DrawCommand(uint64_t p_Id);
      DrawCommand(DrawCommand &p_Copy);

    private:
      static DrawCommand make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit DrawCommand(const DrawCommand &p_Copy)
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
      static DrawCommand *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static DrawCommand create_handle_by_index(u32 p_Index);

      static DrawCommand find_by_index(uint32_t p_Index);
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

      DrawCommand duplicate(Low::Util::Name p_Name) const;
      static DrawCommand duplicate(DrawCommand p_Handle,
                                   Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static DrawCommand find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == DrawCommand::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        DrawCommand l_DrawCommand = p_Handle.get_id();
        l_DrawCommand.destroy();
      }

      Low::Math::Matrix4x4 &get_world_transform() const;
      void set_world_transform(Low::Math::Matrix4x4 &p_Value);

      Low::Renderer::GpuSubmesh get_submesh() const;

      uint32_t get_slot() const;
      void set_slot(uint32_t p_Value);

      Low::Renderer::RenderObject get_render_object() const;

      Low::Renderer::Material get_material() const;
      void set_material(Low::Renderer::Material p_Value);

      bool is_uploaded() const;
      void set_uploaded(bool p_Value);
      void toggle_uploaded();

      uint64_t get_render_scene_handle() const;
      void set_render_scene_handle(uint64_t p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static DrawCommand
      make(Low::Renderer::RenderObject p_RenderObject,
           Low::Renderer::RenderScene p_RenderScene,
           Low::Renderer::GpuSubmesh p_Submesh);
      uint64_t get_sort_index() const;

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_submesh(Low::Renderer::GpuSubmesh p_Value);
      void set_render_object(Low::Renderer::RenderObject p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
    public:
      static Low::Util::Set<Low::Renderer::DrawCommand> ms_Dirty;
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
