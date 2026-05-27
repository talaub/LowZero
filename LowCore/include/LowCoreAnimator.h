#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

#include "LowCoreEntity.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererSkeletalRenderObject.h"
#include "LowRendererSkinningPose.h"
#include "LowRendererAnimationClip.h"
#include "LowRendererSkinningInstance.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API Animator : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          Low::Renderer::SkeletalRenderObject render_object;
          Low::Renderer::SkinningPose pose;
          Low::Renderer::SkinningInstance skinning_instance;
          Low::Renderer::AnimationClip active_clip;
          float animation_progress;
          Low::Renderer::Skeleton skeleton;
          Low::Core::Entity entity;
          Low::Util::UniqueId unique_id;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      private:
        static u16 ms_TypeId;

      public:
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<Animator> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static Animator make(Low::Core::Entity p_Entity);
        static Low::Util::Handle _make(Low::Util::Handle p_Entity);
        static Animator make(Low::Core::Entity p_Entity,
                             Low::Util::UniqueId p_UniqueId);
        explicit Animator(const Animator &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        Animator(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        Animator() : Low::Util::Handle()
        {
        }
        Animator(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        Animator &operator=(const Animator &) = default;
        Animator &operator=(Animator &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Animator *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Animator create_handle_by_index(u32 p_Index);

        static Animator find_by_index(uint32_t p_Index);
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

        Animator duplicate(Low::Core::Entity p_Entity) const;
        static Animator duplicate(Animator p_Handle,
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
          Animator l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Animator l_Animator = p_Handle.get_id();
          l_Animator.destroy();
        }

        Low::Renderer::SkeletalRenderObject get_render_object() const;
        void set_render_object(
            Low::Renderer::SkeletalRenderObject p_Value);

        Low::Renderer::SkinningPose get_pose() const;
        void set_pose(Low::Renderer::SkinningPose p_Value);

        Low::Renderer::SkinningInstance get_skinning_instance() const;
        void set_skinning_instance(
            Low::Renderer::SkinningInstance p_Value);

        Low::Renderer::AnimationClip get_active_clip() const;
        void set_active_clip(Low::Renderer::AnimationClip p_Value);

        float get_animation_progress() const;
        void set_animation_progress(float p_Value);

        Low::Renderer::Skeleton get_skeleton() const;
        void set_skeleton(Low::Renderer::Skeleton p_Value);

        Low::Core::Entity get_entity() const;
        void set_entity(Low::Core::Entity p_Value);

        Low::Util::UniqueId get_unique_id() const;

        static bool get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex);

      private:
        static u32 ms_Capacity;
        static u32 ms_PageSize;
        static u32 create_instance(u32 &p_PageIndex,
                                   u32 &p_SlotIndex);
        static u32 create_page();
        void set_unique_id(Low::Util::UniqueId p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Component
  } // namespace Core
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
