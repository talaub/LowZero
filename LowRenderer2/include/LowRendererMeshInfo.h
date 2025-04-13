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
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API MeshInfoData
    {
      MeshResourceState state;
      uint32_t vertex_count;
      uint32_t index_count;
      uint32_t uploaded_vertex_count;
      uint32_t uploaded_index_count;
      uint32_t vertex_start;
      uint32_t index_start;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MeshInfoData);
      }
    };

    struct LOW_RENDERER2_API MeshInfo : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<MeshInfo> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      MeshInfo();
      MeshInfo(uint64_t p_Id);
      MeshInfo(MeshInfo &p_Copy);

      static MeshInfo make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit MeshInfo(const MeshInfo &p_Copy)
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
      static MeshInfo *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static MeshInfo find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      MeshInfo duplicate(Low::Util::Name p_Name) const;
      static MeshInfo duplicate(MeshInfo p_Handle,
                                Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static MeshInfo find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == MeshInfo::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MeshInfo l_MeshInfo = p_Handle.get_id();
        l_MeshInfo.destroy();
      }

      MeshResourceState get_state() const;
      void set_state(MeshResourceState p_Value);

      uint32_t get_vertex_count() const;
      void set_vertex_count(uint32_t p_Value);

      uint32_t get_index_count() const;
      void set_index_count(uint32_t p_Value);

      uint32_t get_uploaded_vertex_count() const;
      void set_uploaded_vertex_count(uint32_t p_Value);

      uint32_t get_uploaded_index_count() const;
      void set_uploaded_index_count(uint32_t p_Value);

      uint32_t get_vertex_start() const;
      void set_vertex_start(uint32_t p_Value);

      uint32_t get_index_start() const;
      void set_index_start(uint32_t p_Value);

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
