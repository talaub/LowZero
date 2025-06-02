#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererMeshResourceState.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct MeshInfo;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API SubmeshData
    {
      MeshResourceState state;
      uint32_t meshinfo_count;
      uint32_t uploaded_meshinfo_count;
      Low::Util::List<MeshInfo> meshinfos;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(SubmeshData);
      }
    };

    struct LOW_RENDERER2_API Submesh : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Submesh> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Submesh();
      Submesh(uint64_t p_Id);
      Submesh(Submesh &p_Copy);

      static Submesh make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit Submesh(const Submesh &p_Copy)
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
      static Submesh *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Submesh find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Submesh duplicate(Low::Util::Name p_Name) const;
      static Submesh duplicate(Submesh p_Handle,
                               Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Submesh find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == Submesh::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Submesh l_Submesh = p_Handle.get_id();
        l_Submesh.destroy();
      }

      MeshResourceState get_state() const;
      void set_state(MeshResourceState p_Value);

      uint32_t get_meshinfo_count() const;
      void set_meshinfo_count(uint32_t p_Value);

      uint32_t get_uploaded_meshinfo_count() const;
      void set_uploaded_meshinfo_count(uint32_t p_Value);

      Low::Util::List<MeshInfo> &get_meshinfos() const;
      void set_meshinfos(Low::Util::List<MeshInfo> &p_Value);

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
