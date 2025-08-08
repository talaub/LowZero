#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowUtilResource.h"
#include "LowRendererSubmeshGeometry.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API MeshGeometryData
    {
      uint32_t submesh_count;
      Low::Util::List<SubmeshGeometry> submeshes;
      Low::Math::AABB aabb;
      Low::Math::Sphere bounding_sphere;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MeshGeometryData);
      }
    };

    struct LOW_RENDERER2_API MeshGeometry : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<MeshGeometry> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      MeshGeometry();
      MeshGeometry(uint64_t p_Id);
      MeshGeometry(MeshGeometry &p_Copy);

      static MeshGeometry make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit MeshGeometry(const MeshGeometry &p_Copy)
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
      static MeshGeometry *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static MeshGeometry find_by_index(uint32_t p_Index);
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

      MeshGeometry duplicate(Low::Util::Name p_Name) const;
      static MeshGeometry duplicate(MeshGeometry p_Handle,
                                    Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static MeshGeometry find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == MeshGeometry::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MeshGeometry l_MeshGeometry = p_Handle.get_id();
        l_MeshGeometry.destroy();
      }

      uint32_t get_submesh_count() const;
      void set_submesh_count(uint32_t p_Value);

      Low::Util::List<SubmeshGeometry> &get_submeshes() const;
      void set_submeshes(Low::Util::List<SubmeshGeometry> &p_Value);

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
