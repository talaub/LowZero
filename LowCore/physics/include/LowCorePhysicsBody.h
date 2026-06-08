#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

#include "LowMath.h"
#include "LowCorePhysicsWorld.h"
#include "LowCorePhysicsShape.h"
#include "LowCorePhysicsBodyMotionType.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Physics {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API Body : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          uint64_t backend_id;
          World world;
          Shape shape;
          Low::Util::Name name;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      private:
        static u16 ms_TypeId;

      public:
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<Body> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

      private:
        static Body make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);

      public:
        explicit Body(const Body &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        Body(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        Body() : Low::Util::Handle()
        {
        }
        Body(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        Body &operator=(const Body &) = default;
        Body &operator=(Body &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Body *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Body create_handle_by_index(u32 p_Index);

        static Body find_by_index(uint32_t p_Index);
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

        Body duplicate(Low::Util::Name p_Name) const;
        static Body duplicate(Body p_Handle, Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static Body find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          Body l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Body l_Body = p_Handle.get_id();
          l_Body.destroy();
        }

        uint64_t get_backend_id() const;

        World get_world() const;

        Shape get_shape() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Body make(World p_World, Shape p_Shape,
                         Low::Math::Vector3 p_Position,
                         Low::Math::Quaternion p_Rotation,
                         BodyMotionType p_MotionType, float p_Mass,
                         bool p_Gravity);
        Low::Math::Vector3 get_position();
        void set_position(Low::Math::Vector3 p_Value);
        Low::Math::Quaternion get_rotation();
        void set_rotation(Low::Math::Quaternion p_Value);
        Low::Math::Matrix4x4 &get_transform();
        void set_transform(Low::Math::Matrix4x4 &p_Value);
        Low::Math::Vector3 get_linear_velocity();
        void set_linear_velocity(Low::Math::Vector3 p_Value);
        Low::Math::Vector3 get_angular_velocity();
        void set_angular_velocity(Low::Math::Vector3 p_Value);
        static bool get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex);

      private:
        static u32 ms_Capacity;
        static u32 ms_PageSize;
        static u32 create_instance(u32 &p_PageIndex,
                                   u32 &p_SlotIndex);
        static u32 create_page();
        void set_backend_id(uint64_t p_Value);
        void set_world(World p_Value);
        void set_shape(Shape p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Physics
  } // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
namespace Low {
  namespace Core {
    typedef Physics::Body PhysicsBody;
  }
} // namespace Low
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
