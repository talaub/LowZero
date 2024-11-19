#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowUtilResource.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API SkeletalAnimationData
    {
      float duration;
      float ticks_per_second;
      Util::List<Util::Resource::AnimationChannel> channels;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(SkeletalAnimationData);
      }
    };

    struct LOW_RENDERER_API SkeletalAnimation
        : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<SkeletalAnimation> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      SkeletalAnimation();
      SkeletalAnimation(uint64_t p_Id);
      SkeletalAnimation(SkeletalAnimation &p_Copy);

      static SkeletalAnimation make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit SkeletalAnimation(const SkeletalAnimation &p_Copy)
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
      static SkeletalAnimation *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static SkeletalAnimation find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      SkeletalAnimation duplicate(Low::Util::Name p_Name) const;
      static SkeletalAnimation duplicate(SkeletalAnimation p_Handle,
                                         Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static SkeletalAnimation find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == SkeletalAnimation::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        SkeletalAnimation l_SkeletalAnimation = p_Handle.get_id();
        l_SkeletalAnimation.destroy();
      }

      float get_duration() const;
      void set_duration(float p_Value);

      float get_ticks_per_second() const;
      void set_ticks_per_second(float p_Value);

      Util::List<Util::Resource::AnimationChannel> &
      get_channels() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
