#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreEntity.h"

#include "LowCoreMeshAsset.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API MeshRendererData
      {
        MeshAsset mesh;
        Low::Core::Entity entity;

        static size_t get_size()
        {
          return sizeof(MeshRendererData);
        }
      };

      struct LOW_CORE_API MeshRenderer : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<MeshRenderer> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        MeshRenderer();
        MeshRenderer(uint64_t p_Id);
        MeshRenderer(MeshRenderer &p_Copy);

        static MeshRenderer make(Low::Core::Entity p_Entity);
        explicit MeshRenderer(const MeshRenderer &p_Copy)
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
        static MeshRenderer *living_instances()
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
          MeshRenderer l_MeshRenderer = p_Handle.get_id();
          l_MeshRenderer.destroy();
        }

        MeshAsset get_mesh() const;
        void set_mesh(MeshAsset p_Value);

        Low::Core::Entity get_entity() const;
        void set_entity(Low::Core::Entity p_Value);

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
      };
    } // namespace Component
  }   // namespace Core
} // namespace Low
