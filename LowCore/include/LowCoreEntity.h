#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct LOW_EXPORT EntityData
    {
      Util::Map<uint16_t, Util::Handle> components;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(EntityData);
      }
    };

    struct LOW_EXPORT Entity : public Low::Util::Handle
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

      bool is_alive() const;

      static uint32_t get_capacity();

      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.check_alive(ms_Slots, get_capacity());
      }

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);

      uint64_t get_component(uint16_t p_TypeId);

    private:
      Util::Map<uint16_t, Util::Handle> &get_components() const;
    };
  } // namespace Core
} // namespace Low