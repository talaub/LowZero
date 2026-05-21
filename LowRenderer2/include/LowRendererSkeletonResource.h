#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    struct SkeletonResourceConfig
    {
      Util::String path;
      Util::String data_path;
      u64 skeleton_id;
      Util::Name name;
      u32 bone_count;
    };
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_RENDERER2_API SkeletonResource
        : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Util::String path;
        Util::String data_path;
        uint64_t skeleton_id;
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

      static Low::Util::List<SkeletonResource> ms_LivingInstances;

      const static Low::Util::TypeIdentifier IDENTIFIER;

      [[nodiscard]] static u16 type_id()
      {
        return ms_TypeId;
      }

    private:
      static SkeletonResource make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);

    public:
      explicit SkeletonResource(const SkeletonResource &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      void destroy();

      static void initialize();
      static void cleanup();

      SkeletonResource(u64 p_Id) : Low::Util::Handle(p_Id)
      {
      }
      SkeletonResource() : Low::Util::Handle()
      {
      }
      SkeletonResource(Low::Util::Handle p_Handle)
          : Low::Util::Handle(p_Handle.get_id())
      {
      }

      using Handle::operator=;

      SkeletonResource &operator=(const SkeletonResource &) = default;
      SkeletonResource &
      operator=(SkeletonResource &&) noexcept = default;

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static SkeletonResource *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static SkeletonResource create_handle_by_index(u32 p_Index);

      static SkeletonResource find_by_index(uint32_t p_Index);
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

      SkeletonResource duplicate(Low::Util::Name p_Name) const;
      static SkeletonResource duplicate(SkeletonResource p_Handle,
                                        Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static SkeletonResource find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Serial::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Serial::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        SkeletonResource l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        SkeletonResource l_SkeletonResource = p_Handle.get_id();
        l_SkeletonResource.destroy();
      }

      Util::String get_path() const;

      Util::String get_data_path() const;

      uint64_t get_skeleton_id() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static SkeletonResource make(Util::String p_Path);
      static SkeletonResource find_by_path(Util::String p_Path);
      static SkeletonResource
      make_from_config(const SkeletonResourceConfig &p_Config);
      static bool get_page_for_index(const u32 p_Index,
                                     u32 &p_PageIndex,
                                     u32 &p_SlotIndex);

    private:
      static u32 ms_Capacity;
      static u32 ms_PageSize;
      static u32 create_instance(u32 &p_PageIndex, u32 &p_SlotIndex);
      static u32 create_page();
      void set_path(Util::String p_Value);
      void set_path(const char *p_Value);
      void set_data_path(Util::String p_Value);
      void set_data_path(const char *p_Value);
      void set_skeleton_id(uint64_t p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Renderer
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
