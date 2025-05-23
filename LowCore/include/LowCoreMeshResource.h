#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererExposedObjects.h"
#include "LowCoreResource.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    struct Submesh
    {
      Util::Name name;
      Math::Matrix4x4 transformation;
      Renderer::Mesh mesh;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API MeshResourceData
    {
      Util::String path;
      Util::List<Submesh> submeshes;
      uint32_t reference_count;
      Renderer::Skeleton skeleton;
      ResourceState state;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MeshResourceData);
      }
    };

    struct LOW_CORE_API MeshResource : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<MeshResource> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      MeshResource();
      MeshResource(uint64_t p_Id);
      MeshResource(MeshResource &p_Copy);

    private:
      static MeshResource make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit MeshResource(const MeshResource &p_Copy)
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
      static MeshResource *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static MeshResource find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      MeshResource duplicate(Low::Util::Name p_Name) const;
      static MeshResource duplicate(MeshResource p_Handle,
                                    Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static MeshResource find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == MeshResource::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MeshResource l_MeshResource = p_Handle.get_id();
        l_MeshResource.destroy();
      }

      Util::String &get_path() const;

      Util::List<Submesh> &get_submeshes() const;

      Renderer::Skeleton get_skeleton() const;

      ResourceState get_state() const;
      void set_state(ResourceState p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static MeshResource make(Util::String &p_Path);
      bool is_loaded();
      void load();
      void _load(uint32_t p_MeshIndex);
      void _internal_load(Low::Util::Resource::Mesh &p_Mesh);
      void unload();
      static void update();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_path(Util::String &p_Value);
      void set_path(const char *p_Value);
      uint32_t get_reference_count() const;
      void set_reference_count(uint32_t p_Value);
      void set_skeleton(Renderer::Skeleton p_Value);
      void _unload();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Core
} // namespace Low
