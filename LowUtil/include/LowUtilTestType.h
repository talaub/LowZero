#pragma once

#include "LowUtilApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    struct LOW_EXPORT TestTypeData
    {
      float age;
      bool happy;
      Low::Util::Name name;

      static size_t get_size()
      {
        return sizeof(TestTypeData);
      }
    };

    struct LOW_EXPORT TestType : public Low::Util::Handle
    {
      friend void Low::Util::Instances::initialize();

    private:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<TestType> ms_LivingInstances;

    public:
      const static uint16_t TYPE_ID;

      TestType();
      TestType(uint64_t p_Id);
      TestType(TestType &p_Copy);

      static TestType make(Low::Util::Name p_Name);
      void destroy();

      static void cleanup();

      static uint32_t living_count()
      {
        return static_cast<uint32_t>(ms_LivingInstances.size());
      }
      static TestType *living_instances()
      {
        return ms_LivingInstances.data();
      }

      bool is_alive() const;

      static uint32_t get_capacity();

      float get_age() const;
      void set_age(float p_Value);

      bool is_happy() const;
      void set_happy(bool p_Value);

      Low::Util::Name get_name() const;
      void set_name(Low::Util::Name p_Value);
    };
  } // namespace Util
} // namespace Low
