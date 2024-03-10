#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreMeshResource.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API MeshAssetData
    {
      MeshResource lod0;
      uint32_t reference_count;
      Low::Util::UniqueId unique_id;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(MeshAssetData);
      }
    };

    struct LOW_CORE_API MeshAsset : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<MeshAsset> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      MeshAsset();
      MeshAsset(uint64_t p_Id);
      MeshAsset(MeshAsset &p_Copy);

      static MeshAsset make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit MeshAsset(const MeshAsset &p_Copy)
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
      static MeshAsset *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static MeshAsset find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      MeshAsset duplicate(Low::Util::Name p_Name) const;
      static MeshAsset duplicate(MeshAsset p_Handle,
                                 Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static MeshAsset find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == MeshAsset::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MeshAsset l_MeshAsset = p_Handle.get_id();
        l_MeshAsset.destroy();
      }

      MeshResource get_lod0() const;
      void set_lod0(MeshResource p_Value);

      Low::Util::UniqueId get_unique_id() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      bool is_loaded();
      void load();
      void unload();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      uint32_t get_reference_count() const;
      void set_reference_count(uint32_t p_Value);
      void set_unique_id(Low::Util::UniqueId p_Value);
      void _unload();
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Core
} // namespace Low
