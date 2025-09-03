#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererMeshResource.h"
#include "LowRendererMeshGeometry.h"
#include "LowRendererGpuMesh.h"
#include "LowRendererMeshState.h"
#include "LowRendererEditorImage.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API MeshData
    {
      Low::Renderer::MeshResource resource;
      MeshState state;
      Low::Renderer::MeshGeometry geometry;
      Low::Renderer::GpuMesh gpu;
      bool unloadable;
      Low::Util::Set<u64> references;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MeshData);
      }
    };

    struct LOW_RENDERER2_API Mesh : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Mesh> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Mesh();
      Mesh(uint64_t p_Id);
      Mesh(Mesh &p_Copy);

      static Mesh make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit Mesh(const Mesh &p_Copy)
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
      static Mesh *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Mesh create_handle_by_index(u32 p_Index);

      static Mesh find_by_index(uint32_t p_Index);
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

      void reference(const u64 p_Id);
      void dereference(const u64 p_Id);
      u32 references() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Mesh duplicate(Low::Util::Name p_Name) const;
      static Mesh duplicate(Mesh p_Handle, Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Mesh find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == Mesh::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Mesh l_Mesh = p_Handle.get_id();
        l_Mesh.destroy();
      }

      Low::Renderer::MeshResource get_resource() const;
      void set_resource(Low::Renderer::MeshResource p_Value);

      MeshState get_state() const;
      void set_state(MeshState p_Value);

      Low::Renderer::MeshGeometry get_geometry() const;
      void set_geometry(Low::Renderer::MeshGeometry p_Value);

      Low::Renderer::GpuMesh get_gpu() const;
      void set_gpu(Low::Renderer::GpuMesh p_Value);

      bool is_unloadable() const;
      void set_unloadable(bool p_Value);
      void toggle_unloadable();

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Mesh
      make_from_resource_config(MeshResourceConfig &p_Config);
      EditorImage get_editor_image();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      Low::Util::Set<u64> &get_references() const;

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
