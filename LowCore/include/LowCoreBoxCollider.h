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

      struct LOW_CORE_API BoxCollider : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Low::Math::Vector3 center;
          Low::Math::Quaternion rotation;
          Low::Math::Vector3 half_extents;
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

        static Low::Util::List<BoxCollider> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static BoxCollider make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        static BoxCollider make(Low::Core::Entity p_Entity,
                                Low::Util::UniqueId p_UniqueId);
        explicit BoxCollider(const BoxCollider &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        BoxCollider(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        BoxCollider() : Low::Util::Handle()
        {
        }
        BoxCollider(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        BoxCollider &operator=(const BoxCollider &) = default;
        BoxCollider &operator=(BoxCollider &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static BoxCollider *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static BoxCollider create_handle_by_index(u32 p_Index);

        static BoxCollider find_by_index(uint32_t p_Index);
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

        BoxCollider duplicate(Low::Core::Entity p_Entity) const;
        static BoxCollider duplicate(BoxCollider p_Handle,
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
          BoxCollider l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          BoxCollider l_BoxCollider = p_Handle.get_id();
          l_BoxCollider.destroy();
        }

        Low::Math::Vector3 get_center() const;
        void set_center(Low::Math::Vector3 p_Value);
        void set_center(float p_X, float p_Y, float p_Z);
        void set_center_x(float p_Value);
        void set_center_y(float p_Value);
        void set_center_z(float p_Value);

        Low::Math::Quaternion get_rotation() const;
        void set_rotation(Low::Math::Quaternion p_Value);

        Low::Math::Vector3 get_half_extents() const;
        void set_half_extents(Low::Math::Vector3 p_Value);
        void set_half_extents(float p_X, float p_Y, float p_Z);
        void set_half_extents_x(float p_Value);
        void set_half_extents_y(float p_Value);
        void set_half_extents_z(float p_Value);

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
        static Low::Util::Set<Low::Core::Component::BoxCollider>
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
