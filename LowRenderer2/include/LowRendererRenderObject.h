#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererMeshResource.h"
#include "LowRendererMaterial.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct RenderScene;
    struct DrawCommand;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API RenderObjectData
    {
      Low::Math::Matrix4x4 world_transform;
      Low::Renderer::MeshResource mesh_resource;
      bool uploaded;
      uint32_t slot;
      uint64_t render_scene_handle;
      Low::Renderer::Material material;
      Low::Util::List<DrawCommand> draw_commands;
      bool dirty;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(RenderObjectData);
      }
    };

    struct LOW_RENDERER2_API RenderObject : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<RenderObject> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      RenderObject();
      RenderObject(uint64_t p_Id);
      RenderObject(RenderObject &p_Copy);

    private:
      static RenderObject make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit RenderObject(const RenderObject &p_Copy)
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
      static RenderObject *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static RenderObject find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      RenderObject duplicate(Low::Util::Name p_Name) const;
      static RenderObject duplicate(RenderObject p_Handle,
                                    Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static RenderObject find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == RenderObject::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        RenderObject l_RenderObject = p_Handle.get_id();
        l_RenderObject.destroy();
      }

      Low::Math::Matrix4x4 &get_world_transform() const;
      void set_world_transform(Low::Math::Matrix4x4 &p_Value);

      Low::Renderer::MeshResource get_mesh_resource() const;

      bool is_uploaded() const;
      void set_uploaded(bool p_Value);
      void toggle_uploaded();

      uint32_t get_slot() const;
      void set_slot(uint32_t p_Value);

      uint64_t get_render_scene_handle() const;

      Low::Renderer::Material get_material() const;
      void set_material(Low::Renderer::Material p_Value);

      Low::Util::List<DrawCommand> &get_draw_commands() const;

      bool is_dirty() const;
      void set_dirty(bool p_Value);
      void toggle_dirty();
      void mark_dirty();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static RenderObject
      make(RenderScene p_RenderScene,
           Low::Renderer::MeshResource p_MeshResource);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_mesh_resource(Low::Renderer::MeshResource p_Value);
      void set_render_scene_handle(uint64_t p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
    public:
      static Low::Util::Set<Low::Renderer::RenderObject> ms_Dirty;
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
