#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererMeshState.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API GpuSubmesh : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        MeshState state;
        uint32_t uploaded_vertex_count;
        uint32_t uploaded_index_count;
        uint32_t vertex_count;
        uint32_t index_count;
        uint32_t vertex_start;
        uint32_t index_start;
        Low::Math::Matrix4x4 transform;
        Low::Math::Matrix4x4 parent_transform;
        Low::Math::Matrix4x4 local_transform;
        Low::Math::AABB aabb;
        Low::Math::Sphere bounding_sphere;
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

      static Low::Util::List<GpuSubmesh> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      GpuSubmesh();
      GpuSubmesh(uint64_t p_Id);
      GpuSubmesh(GpuSubmesh &p_Copy);

      static GpuSubmesh make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit GpuSubmesh(const GpuSubmesh &p_Copy)
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
      static GpuSubmesh *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static GpuSubmesh create_handle_by_index(u32 p_Index);

      static GpuSubmesh find_by_index(uint32_t p_Index);
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

      GpuSubmesh duplicate(Low::Util::Name p_Name) const;
      static GpuSubmesh duplicate(GpuSubmesh p_Handle,
                                  Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static GpuSubmesh find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        GpuSubmesh l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        GpuSubmesh l_GpuSubmesh = p_Handle.get_id();
        l_GpuSubmesh.destroy();
      }

      MeshState get_state() const;
      void set_state(MeshState p_Value);

      uint32_t get_uploaded_vertex_count() const;
      void set_uploaded_vertex_count(uint32_t p_Value);

      uint32_t get_uploaded_index_count() const;
      void set_uploaded_index_count(uint32_t p_Value);

      uint32_t get_vertex_count() const;
      void set_vertex_count(uint32_t p_Value);

      uint32_t get_index_count() const;
      void set_index_count(uint32_t p_Value);

      uint32_t get_vertex_start() const;
      void set_vertex_start(uint32_t p_Value);

      uint32_t get_index_start() const;
      void set_index_start(uint32_t p_Value);

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

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
