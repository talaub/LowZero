#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

#include "LowCoreEntity.h"

#include "LowMath.h"
#include "LowCorePhysicsShape.h"
#include "LowCorePhysicsBody.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API ConvexHullCollider
          : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Low::Util::List<Low::Math::Vector3> points;
          bool trigger;
          Low::Core::Physics::Shape shape;
          Low::Core::Physics::Body static_body;
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

        static Low::Util::List<ConvexHullCollider> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static ConvexHullCollider make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        static ConvexHullCollider
        make(Low::Core::Entity p_Entity,
             Low::Util::UniqueId p_UniqueId);
        explicit ConvexHullCollider(const ConvexHullCollider &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        ConvexHullCollider(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        ConvexHullCollider() : Low::Util::Handle()
        {
        }
        ConvexHullCollider(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        ConvexHullCollider &
        operator=(const ConvexHullCollider &) = default;
        ConvexHullCollider &
        operator=(ConvexHullCollider &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static ConvexHullCollider *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static ConvexHullCollider create_handle_by_index(u32 p_Index);

        static ConvexHullCollider find_by_index(uint32_t p_Index);
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

        ConvexHullCollider
        duplicate(Low::Core::Entity p_Entity) const;
        static ConvexHullCollider
        duplicate(ConvexHullCollider p_Handle,
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
          ConvexHullCollider l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          ConvexHullCollider l_ConvexHullCollider = p_Handle.get_id();
          l_ConvexHullCollider.destroy();
        }

        Low::Util::List<Low::Math::Vector3> &get_points() const;
        void set_points(Low::Util::List<Low::Math::Vector3> &p_Value);

        bool is_trigger() const;
        void set_trigger(bool p_Value);
        void toggle_trigger();

        Low::Core::Physics::Shape get_shape() const;

        Low::Core::Physics::Body get_static_body() const;

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
        void set_shape(Low::Core::Physics::Shape p_Value);
        void set_static_body(Low::Core::Physics::Body p_Value);
        void set_initialized(bool p_Value);
        void toggle_initialized();
        void set_unique_id(Low::Util::UniqueId p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      public:
        static Low::Util::Set<
            Low::Core::Component::ConvexHullCollider>
            ms_Dirty;
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Component
  } // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
