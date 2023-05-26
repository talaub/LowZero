#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowMath.h"
#include "LowRendererSkeletalAnimation.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct Bone
    {
      Util::Name name;
      uint32_t index;
      Math::Matrix4x4 localTransformation;
      Math::Matrix4x4 parentTransformation;
      Math::Matrix4x4 offset;
      Util::List<Bone> children;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER_API SkeletonData
    {
      Bone root_bone;
      uint32_t bone_count;
      Util::List<SkeletalAnimation> animations;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(SkeletonData);
      }
    };

    struct LOW_RENDERER_API Skeleton : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Skeleton> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Skeleton();
      Skeleton(uint64_t p_Id);
      Skeleton(Skeleton &p_Copy);

      static Skeleton make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit Skeleton(const Skeleton &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static Skeleton *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Skeleton find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static Skeleton find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
                                           Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == Skeleton::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Skeleton l_Skeleton = p_Handle.get_id();
        l_Skeleton.destroy();
      }

      Bone &get_root_bone() const;
      void set_root_bone(Bone &p_Value);

      uint32_t get_bone_count() const;
      void set_bone_count(uint32_t p_Value);

      Util::List<SkeletalAnimation> &get_animations() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
    };
  } // namespace Renderer
} // namespace Low
