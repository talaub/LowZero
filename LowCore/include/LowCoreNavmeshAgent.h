#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreEntity.h"

#include "LowMath.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API NavmeshAgentData
      {
        float speed;
        int agent_index;
        Low::Core::Entity entity;
        Low::Util::UniqueId unique_id;

        static size_t get_size()
        {
          return sizeof(NavmeshAgentData);
        }
      };

      struct LOW_CORE_API NavmeshAgent : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<NavmeshAgent> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        NavmeshAgent();
        NavmeshAgent(uint64_t p_Id);
        NavmeshAgent(NavmeshAgent &p_Copy);

        static NavmeshAgent make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        explicit NavmeshAgent(const NavmeshAgent &p_Copy)
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
        static NavmeshAgent *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static NavmeshAgent find_by_index(uint32_t p_Index);

        bool is_alive() const;

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
                                             Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          return p_Handle.get_type() == NavmeshAgent::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          NavmeshAgent l_NavmeshAgent = p_Handle.get_id();
          l_NavmeshAgent.destroy();
        }

        float get_speed() const;
        void set_speed(float p_Value);

        int get_agent_index() const;
        void set_agent_index(int p_Value);

        Low::Core::Entity get_entity() const;
        void set_entity(Low::Core::Entity p_Value);

        Low::Util::UniqueId get_unique_id() const;

        void set_target_position(Math::Vector3 &p_TargetPosition);

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
        void set_unique_id(Low::Util::UniqueId p_Value);
      };
    } // namespace Component
  }   // namespace Core
} // namespace Low
