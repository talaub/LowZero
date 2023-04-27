#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

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
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(EntityData);
      }
    };

    struct LOW_CORE_API Entity : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Entity> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Entity();
      Entity(uint64_t p_Id);
      Entity(Entity &p_Copy);

      static Entity make(Low::Util::Name p_Name);
      explicit Entity(const Entity &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
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

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      static Entity find_by_name(Low::Util::Name p_Name);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
                                           Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Entity l_Entity = p_Handle.get_id();
        l_Entity.destroy();
      }

      Util::Map<uint16_t, Util::Handle> &get_components() const;

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      uint64_t get_component(uint16_t p_TypeId);
      void add_component(Util::Handle &p_Component);
      void remove_component(uint16_t p_ComponentType);
      bool has_component(uint16_t p_ComponentType);
      Component::Transform get_transform();

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
    };
  } // namespace Core
} // namespace Low
