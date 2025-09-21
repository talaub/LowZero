#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreEntity.h"
#include "LowUtilVariant.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    struct Entity;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API Prefab : public Low::Util::Handle
    {
    public:
      struct Data
      {
      public:
        Util::Handle parent;
        Util::List<Util::Handle> children;
        Util::Map<uint16_t, Util::Map<Util::Name, Util::Variant>>
            components;
        Low::Util::UniqueId unique_id;
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

      static Low::Util::List<Prefab> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Prefab();
      Prefab(uint64_t p_Id);
      Prefab(Prefab &p_Copy);

      static Prefab make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      static Prefab make(Low::Util::Name p_Name,
                         Low::Util::UniqueId p_UniqueId);
      explicit Prefab(const Prefab &p_Copy)
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
      static Prefab *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Prefab create_handle_by_index(u32 p_Index);

      static Prefab find_by_index(uint32_t p_Index);
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

      Prefab duplicate(Low::Util::Name p_Name) const;
      static Prefab duplicate(Prefab p_Handle,
                              Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Prefab find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        Prefab l_Handle = p_Handle.get_id();
        return l_Handle.is_alive();
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Prefab l_Prefab = p_Handle.get_id();
        l_Prefab.destroy();
      }

      Util::Handle get_parent() const;
      void set_parent(Util::Handle p_Value);

      Util::List<Util::Handle> &get_children() const;
      void set_children(Util::List<Util::Handle> &p_Value);

      Util::Map<uint16_t, Util::Map<Util::Name, Util::Variant>> &
      get_components() const;
      void set_components(
          Util::Map<uint16_t, Util::Map<Util::Name, Util::Variant>>
              &p_Value);

      Low::Util::UniqueId get_unique_id() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Prefab make(Entity &p_Entity);
      Entity spawn(Region p_Region);
      bool compare_property(Util::Handle p_Component,
                            Util::Name p_PropertyName);
      void apply(Util::Handle p_Component, Util::Name p_PropertyName);
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
      void set_unique_id(Low::Util::UniqueId p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Core
} // namespace Low
