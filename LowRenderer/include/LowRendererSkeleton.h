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

    struct LOW_RENDERER_API Skeleton : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Bone root_bone;
        uint32_t bone_count;
        Util::List<SkeletalAnimation> animations;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(Data);
        }
      };

    public:
      static Low::Util::UniqueLock<Low::Util::SharedMutex>
          ms_PagesLock;
      static Low::Util::SharedMutex ms_PagesMutex;
      static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

      static Low::Util::List<Skeleton> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Skeleton();
      Skeleton(uint64_t p_Id);
      Skeleton(Skeleton &p_Copy);

      static Skeleton make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      explicit Skeleton(const Skeleton &p_Copy)
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
      static Skeleton *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Skeleton create_handle_by_index(u32 p_Index);

      static Skeleton find_by_index(uint32_t p_Index);
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

      Skeleton duplicate(Low::Util::Name p_Name) const;
      static Skeleton duplicate(Skeleton p_Handle,
                                Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Skeleton find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        Skeleton l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
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

      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock);
      static u32 create_page();

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low
