#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

#include "LowCoreEntity.h"

#include "LowMath.h"
#include "LowCorePhysicsBody.h"
#include "LowCorePhysicsBodyMotionType.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API Rigidbody : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Low::Core::Physics::BodyMotionType motion_type;
          bool gravity;
          float mass;
          Low::Core::Physics::Body body;
          bool initialized;
          Low::Core::Entity entity;
          Low::Util::UniqueId unique_id;
          bool dirty;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      private:
        static u16 ms_TypeId;

      public:
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<Rigidbody> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static Rigidbody make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        static Rigidbody make(Low::Core::Entity p_Entity,
                              Low::Util::UniqueId p_UniqueId);
        explicit Rigidbody(const Rigidbody &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        Rigidbody(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        Rigidbody() : Low::Util::Handle()
        {
        }
        Rigidbody(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        Rigidbody &operator=(const Rigidbody &) = default;
        Rigidbody &operator=(Rigidbody &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Rigidbody *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Rigidbody create_handle_by_index(u32 p_Index);

        static Rigidbody find_by_index(uint32_t p_Index);
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

        void serialize(Low::Util::Serial::Node &p_Node) const;

        Rigidbody duplicate(Low::Core::Entity p_Entity) const;
        static Rigidbody duplicate(Rigidbody p_Handle,
                                   Low::Core::Entity p_Entity);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Handle p_Entity);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          Rigidbody l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Rigidbody l_Rigidbody = p_Handle.get_id();
          l_Rigidbody.destroy();
        }

        Low::Core::Physics::BodyMotionType get_motion_type() const;
        void
        set_motion_type(Low::Core::Physics::BodyMotionType p_Value);

        bool is_gravity() const;
        void set_gravity(bool p_Value);
        void toggle_gravity();

        float get_mass() const;
        void set_mass(float p_Value);

        Low::Core::Physics::Body get_body() const;

        bool is_initialized() const;

        Low::Core::Entity get_entity() const;
        void set_entity(Low::Core::Entity p_Value);

        Low::Util::UniqueId get_unique_id() const;

        bool is_dirty() const;
        void set_dirty(bool p_Value);
        void toggle_dirty();
        void mark_dirty();

        void rebuild();
        static bool get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex);

      private:
        static u32 ms_Capacity;
        static u32 ms_PageSize;
        static u32 create_instance(u32 &p_PageIndex,
                                   u32 &p_SlotIndex);
        static u32 create_page();
        void set_body(Low::Core::Physics::Body p_Value);
        void set_initialized(bool p_Value);
        void toggle_initialized();
        void set_unique_id(Low::Util::UniqueId p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      public:
        static Low::Util::Set<Low::Core::Component::Rigidbody>
            ms_Dirty;
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Component
  }   // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
