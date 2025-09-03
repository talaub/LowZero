#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreEntity.h"

#include "LowMath.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API TransformData
      {
        Low::Math::Vector3 position;
        Low::Math::Quaternion rotation;
        Low::Math::Vector3 scale;
        uint64_t parent;
        uint64_t parent_uid;
        Low::Util::List<uint64_t> children;
        Low::Math::Vector3 world_position;
        Low::Math::Quaternion world_rotation;
        Low::Math::Vector3 world_scale;
        Low::Math::Matrix4x4 world_matrix;
        bool world_updated;
        Low::Core::Entity entity;
        Low::Util::UniqueId unique_id;
        bool dirty;
        bool world_dirty;

        static size_t get_size()
        {
          return sizeof(TransformData);
        }
      };

      struct LOW_CORE_API Transform : public Low::Util::Handle
      {
      public:
        static std::shared_mutex ms_BufferMutex;
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Transform> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Transform();
        Transform(uint64_t p_Id);
        Transform(Transform &p_Copy);

        static Transform make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        static Transform make(Low::Core::Entity p_Entity,
                              Low::Util::UniqueId p_UniqueId);
        explicit Transform(const Transform &p_Copy)
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
        static Transform *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Transform create_handle_by_index(u32 p_Index);

        static Transform find_by_index(uint32_t p_Index);
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

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        Transform duplicate(Low::Core::Entity p_Entity) const;
        static Transform duplicate(Transform p_Handle,
                                   Low::Core::Entity p_Entity);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Handle p_Entity);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Yaml::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          READ_LOCK(l_Lock);
          return p_Handle.get_type() == Transform::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Transform l_Transform = p_Handle.get_id();
          l_Transform.destroy();
        }

        Low::Math::Vector3 &position() const;
        void position(Low::Math::Vector3 &p_Value);
        void position(float p_X, float p_Y, float p_Z);
        void position_x(float p_Value);
        void position_y(float p_Value);
        void position_z(float p_Value);

        Low::Math::Quaternion &rotation() const;
        void rotation(Low::Math::Quaternion &p_Value);

        Low::Math::Vector3 &scale() const;
        void scale(Low::Math::Vector3 &p_Value);
        void scale(float p_X, float p_Y, float p_Z);
        void scale_x(float p_Value);
        void scale_y(float p_Value);
        void scale_z(float p_Value);

        uint64_t get_parent() const;
        void set_parent(uint64_t p_Value);

        uint64_t get_parent_uid() const;

        Low::Util::List<uint64_t> &get_children() const;

        Low::Math::Vector3 &get_world_position();

        Low::Math::Quaternion &get_world_rotation();

        Low::Math::Vector3 &get_world_scale();

        Low::Math::Matrix4x4 &get_world_matrix();

        bool is_world_updated() const;
        void set_world_updated(bool p_Value);
        void toggle_world_updated();

        Low::Core::Entity get_entity() const;
        void set_entity(Low::Core::Entity p_Value);

        Low::Util::UniqueId get_unique_id() const;

        bool is_dirty() const;
        void set_dirty(bool p_Value);
        void toggle_dirty();
        void mark_dirty();

        bool is_world_dirty() const;
        void set_world_dirty(bool p_Value);
        void toggle_world_dirty();
        void mark_world_dirty();

        void recalculate_world_transform();

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
        void set_parent_uid(uint64_t p_Value);
        void set_world_position(Low::Math::Vector3 &p_Value);
        void set_world_position(float p_X, float p_Y, float p_Z);
        void set_world_position_x(float p_Value);
        void set_world_position_y(float p_Value);
        void set_world_position_z(float p_Value);
        void set_world_rotation(Low::Math::Quaternion &p_Value);
        void set_world_scale(Low::Math::Vector3 &p_Value);
        void set_world_scale(float p_X, float p_Y, float p_Z);
        void set_world_scale_x(float p_Value);
        void set_world_scale_y(float p_Value);
        void set_world_scale_z(float p_Value);
        void set_world_matrix(Low::Math::Matrix4x4 &p_Value);
        void set_unique_id(Low::Util::UniqueId p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Component
  } // namespace Core
} // namespace Low
