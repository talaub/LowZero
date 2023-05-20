#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API SkeletalAnimationData
    {
      uint32_t skeletal_animation_start;
      uint32_t channel_start;
      uint32_t channel_count;
      uint32_t position_key_start;
      uint32_t position_key_count;
      uint32_t rotation_key_start;
      uint32_t rotation_key_count;
      uint32_t scale_key_start;
      uint32_t scale_key_count;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(SkeletalAnimationData);
      }
    };

    struct LOW_RENDERER_API SkeletalAnimation : public Low::Util::Handle
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

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static SkeletalAnimation find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
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

      uint32_t get_skeletal_animation_start() const;
      void set_skeletal_animation_start(uint32_t p_Value);

      uint32_t get_channel_start() const;
      void set_channel_start(uint32_t p_Value);

      uint32_t get_channel_count() const;
      void set_channel_count(uint32_t p_Value);

      uint32_t get_position_key_start() const;
      void set_position_key_start(uint32_t p_Value);

      uint32_t get_position_key_count() const;
      void set_position_key_count(uint32_t p_Value);

      uint32_t get_rotation_key_start() const;
      void set_rotation_key_start(uint32_t p_Value);

      uint32_t get_rotation_key_count() const;
      void set_rotation_key_count(uint32_t p_Value);

      uint32_t get_scale_key_start() const;
      void set_scale_key_start(uint32_t p_Value);

      uint32_t get_scale_key_count() const;
      void set_scale_key_count(uint32_t p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
    };
  } // namespace Renderer
} // namespace Low
