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

    struct LOW_CORE_API PrefabData
    {
      Util::Handle parent;
      Util::List<Util::Handle> children;
      Util::Map<uint16_t, Util::Map<Util::Name, Util::Variant>>
          components;
      Low::Util::UniqueId unique_id;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(PrefabData);
      }
    };

    struct LOW_CORE_API Prefab : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Prefab> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Prefab();
      Prefab(uint64_t p_Id);
      Prefab(Prefab &p_Copy);

      static Prefab make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
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

      static Prefab find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static Prefab find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == Prefab::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
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

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_unique_id(Low::Util::UniqueId p_Value);
    };
  } // namespace Core
} // namespace Low
