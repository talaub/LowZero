#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererMeshState.h"
#include "LowUtilResource.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API SubmeshGeometryData
    {
      MeshState state;
      uint32_t vertex_count;
      uint32_t index_count;
      Low::Util::List<Low::Util::Resource::Vertex> vertices;
      Low::Util::List<uint32_t> indices;
      Low::Math::Matrix4x4 transform;
      Low::Math::Matrix4x4 parent_transform;
      Low::Math::Matrix4x4 local_transform;
      Low::Math::AABB aabb;
      Low::Math::Sphere bounding_sphere;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(SubmeshGeometryData);
      }
    };

    struct LOW_RENDERER2_API SubmeshGeometry
        : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<SubmeshGeometry> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      SubmeshGeometry();
      SubmeshGeometry(uint64_t p_Id);
      SubmeshGeometry(SubmeshGeometry &p_Copy);

      static SubmeshGeometry make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit SubmeshGeometry(const SubmeshGeometry &p_Copy)
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
      static SubmeshGeometry *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static SubmeshGeometry find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      u64 observe(Low::Util::Name p_Observable,
                  Low::Util::Handle p_Observer) const;
      void notify(Low::Util::Handle p_Observed,
                  Low::Util::Name p_Observable);
      void broadcast_observable(Low::Util::Name p_Observable) const;

      static void _notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable);

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      SubmeshGeometry duplicate(Low::Util::Name p_Name) const;
      static SubmeshGeometry duplicate(SubmeshGeometry p_Handle,
                                       Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static SubmeshGeometry find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == SubmeshGeometry::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        SubmeshGeometry l_SubmeshGeometry = p_Handle.get_id();
        l_SubmeshGeometry.destroy();
      }

      MeshState get_state() const;
      void set_state(MeshState p_Value);

      uint32_t get_vertex_count() const;
      void set_vertex_count(uint32_t p_Value);

      uint32_t get_index_count() const;
      void set_index_count(uint32_t p_Value);

      Low::Util::List<Low::Util::Resource::Vertex> &
      get_vertices() const;
      void set_vertices(
          Low::Util::List<Low::Util::Resource::Vertex> &p_Value);

      Low::Util::List<uint32_t> &get_indices() const;
      void set_indices(Low::Util::List<uint32_t> &p_Value);

      Low::Math::Matrix4x4 &get_transform() const;
      void set_transform(Low::Math::Matrix4x4 &p_Value);

      Low::Math::Matrix4x4 &get_parent_transform() const;
      void set_parent_transform(Low::Math::Matrix4x4 &p_Value);

      Low::Math::Matrix4x4 &get_local_transform() const;
      void set_local_transform(Low::Math::Matrix4x4 &p_Value);

      Low::Math::AABB &get_aabb() const;
      void set_aabb(Low::Math::AABB &p_Value);

      Low::Math::Sphere &get_bounding_sphere() const;
      void set_bounding_sphere(Low::Math::Sphere &p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
