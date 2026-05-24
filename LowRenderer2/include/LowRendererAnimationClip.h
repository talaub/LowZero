#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowRendererAnimationClipResource.h"
#include "LowRendererAnimationClipState.h"
#include "LowRendererSkeleton.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    namespace BinSerial {
      struct AnimClipFileHeader
      {
        char magic[12];
        u32 version;
        u32 channel_count;
        float duration;
        float ticks_per_second;
      };

      struct AnimChannelHeader
      {
        u32 bone_index;
        u32 position_count;
        u32 rotation_count;
        u32 scale_count;
      };

      struct VecKey
      {
        float time;
        Math::Vector3 value;
      };

      struct QuatKey
      {
        float time;
        Math::Quaternion value;
      };
    } // namespace BinSerial

    struct AnimationChannel
    {
      u32 bone_index;
      Util::List<BinSerial::VecKey> positions;
      Util::List<BinSerial::QuatKey> rotations;
      Util::List<BinSerial::VecKey> scales;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API AnimationClip : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Low::Renderer::AnimationClipState state;
        Low::Renderer::AnimationClipResource resource;
        Low::Renderer::Skeleton skeleton;
        Util::List<AnimationChannel> channels;
        float duration;
        float ticks_per_second;
        Low::Util::Set<u64> references;
        Low::Util::UniqueId unique_id;
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

      static Low::Util::List<AnimationClip> ms_LivingInstances;

      const static Low::Util::TypeIdentifier IDENTIFIER;

      [[nodiscard]] static u16 type_id()
      {
        return ms_TypeId;
      }

      static AnimationClip make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      static AnimationClip make(Low::Util::Name p_Name,
                                Low::Util::UniqueId p_UniqueId);
      explicit AnimationClip(const AnimationClip &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      AnimationClip(u64 p_Id) : Low::Util::Handle(p_Id)
      {
      }
      AnimationClip() : Low::Util::Handle()
      {
      }
      AnimationClip(Low::Util::Handle p_Handle)
          : Low::Util::Handle(p_Handle.get_id())
      {
      }

      using Handle::operator=;

      AnimationClip &operator=(const AnimationClip &) = default;
      AnimationClip &operator=(AnimationClip &&) noexcept = default;

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static AnimationClip *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static AnimationClip create_handle_by_index(u32 p_Index);

      static AnimationClip find_by_index(uint32_t p_Index);
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

      void reference(const u64 p_Id);
      void dereference(const u64 p_Id);
      u32 references() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Serial::Node &p_Node) const;

      AnimationClip duplicate(Low::Util::Name p_Name) const;
      static AnimationClip duplicate(AnimationClip p_Handle,
                                     Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static AnimationClip find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Serial::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        AnimationClip l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        AnimationClip l_AnimationClip = p_Handle.get_id();
        l_AnimationClip.destroy();
      }

      Low::Renderer::AnimationClipState get_state() const;
      void set_state(Low::Renderer::AnimationClipState p_Value);

      Low::Renderer::AnimationClipResource get_resource() const;
      void set_resource(Low::Renderer::AnimationClipResource p_Value);

      Low::Renderer::Skeleton get_skeleton() const;
      void set_skeleton(Low::Renderer::Skeleton p_Value);

      Util::List<AnimationChannel> &get_channels() const;

      float get_duration() const;
      void set_duration(float p_Value);

      float get_ticks_per_second() const;
      void set_ticks_per_second(float p_Value);

      Low::Util::UniqueId get_unique_id() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static AnimationClip make_from_resource_config(
          const AnimationClipResourceConfig &p_Config);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(u32 &p_PageIndex, u32 &p_SlotIndex);
      static u32 create_page();
      Low::Util::Set<u64> &get_references() const;
      void set_unique_id(Low::Util::UniqueId p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
