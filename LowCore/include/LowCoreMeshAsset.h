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

      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        MeshAsset l_MeshAsset = p_Handle.get_id();
        l_MeshAsset.destroy();
      }

      MeshResource get_lod0() const;
      void set_lod0(MeshResource p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
    };
  } // namespace Core
} // namespace Low
