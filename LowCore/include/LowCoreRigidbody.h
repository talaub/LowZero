#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreEntity.h"

#include "LowMath.h"
#include "LowCorePhysicsObjects.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API RigidbodyData
      {
        bool fixed;
        bool gravity;
        float mass;
        bool initialized;
        PhysicsRigidDynamic rigid_dynamic;
        PhysicsShape physics_shape;
        Math::Shape shape;
        Low::Core::Entity entity;
        Low::Util::UniqueId unique_id;

        static size_t get_size()
        {
          return sizeof(RigidbodyData);
        }
      };

      struct LOW_CORE_API Rigidbody : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Rigidbody> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Rigidbody();
        Rigidbody(uint64_t p_Id);
        Rigidbody(Rigidbody &p_Copy);

        static Rigidbody make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        explicit Rigidbody(const Rigidbody &p_Copy)
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
        static Rigidbody *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Rigidbody find_by_index(uint32_t p_Index);

        bool is_alive() const;

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
                                             Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          return p_Handle.get_type() == Rigidbody::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Rigidbody l_Rigidbody = p_Handle.get_id();
          l_Rigidbody.destroy();
        }

        bool is_fixed() const;
        void set_fixed(bool p_Value);

        bool is_gravity() const;
        void set_gravity(bool p_Value);

        float get_mass() const;
        void set_mass(float p_Value);

        bool is_initialized() const;

        PhysicsRigidDynamic &get_rigid_dynamic() const;

        Math::Shape &get_shape() const;
        void set_shape(Math::Shape &p_Value);

        Low::Core::Entity get_entity() const;
        void set_entity(Low::Core::Entity p_Value);

        Low::Util::UniqueId get_unique_id() const;

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
        void set_initialized(bool p_Value);
        PhysicsShape &get_physics_shape() const;
        void set_unique_id(Low::Util::UniqueId p_Value);
      };
    } // namespace Component
  }   // namespace Core
} // namespace Low
