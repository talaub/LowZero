#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreRegion.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    namespace Component {
      struct Transform;
    }
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_CORE_API EntityData
    {
      Util::Map<uint16_t, Util::Handle> components;
      Region region;
      Low::Util::UniqueId unique_id;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(EntityData);
      }
    };

    struct LOW_CORE_API Entity : public Low::Util::Handle
    {
    public:
      static std::shared_mutex ms_BufferMutex;
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Entity> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Entity();
      Entity(uint64_t p_Id);
      Entity(Entity &p_Copy);

      static Entity make(Low::Util::Name p_Name);
      static Low::Util::Handle _make(Low::Util::Name p_Name);
      static Entity make(Low::Util::Name p_Name,
                         Low::Util::UniqueId p_UniqueId);
      explicit Entity(const Entity &p_Copy)
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
      static Entity *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Entity find_by_index(uint32_t p_Index);
      static Low::Util::Handle _find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Entity duplicate(Low::Util::Name p_Name) const;
      static Entity duplicate(Entity p_Handle,
                              Low::Util::Name p_Name);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name);

      static Entity find_by_name(Low::Util::Name p_Name);
      static Low::Util::Handle _find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        READ_LOCK(l_Lock);
        return p_Handle.get_type() == Entity::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Entity l_Entity = p_Handle.get_id();
        l_Entity.destroy();
      }

      Util::Map<uint16_t, Util::Handle> &get_components() const;

      Region get_region() const;
      void set_region(Region p_Value);

      Low::Util::UniqueId get_unique_id() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      static Entity make(Util::Name p_Name, Region p_Region);
      uint64_t get_component(uint16_t p_TypeId) const;
      void add_component(Low::Util::Handle &p_Component);
      void remove_component(uint16_t p_ComponentType);
      bool has_component(uint16_t p_ComponentType);
      Low::Core::Component::Transform get_transform() const;
      void serialize(Low::Util::Yaml::Node &p_Node,
                     bool p_AddHandles) const;
      void serialize_hierarchy(Util::Yaml::Node &p_Node,
                               bool p_AddHandles) const;
      static Entity &
      deserialize_hierarchy(Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_unique_id(Low::Util::UniqueId p_Value);

      // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
      // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Core
} // namespace Low
