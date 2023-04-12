#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererExposedObjects.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API MeshResourceData
    {
      Util::String path;
      Renderer::Mesh renderer_mesh;
      uint32_t reference_count;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MeshResourceData);
      }
    };

    struct LOW_CORE_API MeshResource : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<MeshResource> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      MeshResource();
      MeshResource(uint64_t p_Id);
      MeshResource(MeshResource &p_Copy);

    private:
      static MeshResource make(Low::Util::Name p_Name);

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

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MeshResource l_MeshResource = p_Handle.get_id();
        l_MeshResource.destroy();
      }

      Util::String &get_path() const;

      Renderer::Mesh get_renderer_mesh() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static MeshResource make(Util::String &p_Path);
      bool is_loaded();
      void load();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_path(Util::String &p_Value);
      void set_renderer_mesh(Renderer::Mesh p_Value);
      uint32_t get_reference_count() const;
      void set_reference_count(uint32_t p_Value);
    };
  } // namespace Core
} // namespace Low
