#include "LowUtilTestType.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Util {
    const uint16_t TestType::TYPE_ID = 1;
    uint8_t *TestType::ms_Buffer = 0;
    Low::Util::Instances::Slot *TestType::ms_Slots = 0;
    Low::Util::List<TestType> TestType::ms_LivingInstances =
        Low::Util::List<TestType>();

    TestType::TestType() : Low::Util::Handle(0ull)
    {
    }
    TestType::TestType(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    TestType::TestType(TestType &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    TestType TestType::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = Low::Util::Instances::create_instance(
          ms_Buffer, ms_Slots, get_capacity());

      TestType l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = TestType::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, TestType, happy, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, TestType, dirty, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, TestType, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      return l_Handle;
    }

    void TestType::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const TestType *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void TestType::cleanup()
    {
      TestType *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);
    }

    bool TestType::is_alive() const
    {
      return m_Data.m_Type == TestType::TYPE_ID &&
             check_alive(ms_Slots, TestType::get_capacity());
    }

    uint32_t TestType::get_capacity()
    {
      static uint32_t l_Capacity = 0u;
      if (l_Capacity == 0u) {
        l_Capacity = Low::Util::Config::get_capacity(N(TestType));
      }
      return l_Capacity;
    }

    float TestType::get_age() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(TestType, age, float);
    }
    void TestType::set_age(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      if (get_age() != p_Value) {
        // Set dirty flags
        TYPE_SOA(TestType, dirty, bool) = true;

        // Set new value
        TYPE_SOA(TestType, age, float) = p_Value;
      }
    }

    bool TestType::is_happy() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(TestType, happy, bool);
    }
    void TestType::set_happy(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      if (is_happy() != p_Value) {
        // Set new value
        TYPE_SOA(TestType, happy, bool) = p_Value;
      }
    }

    bool TestType::is_dirty() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(TestType, dirty, bool);
    }
    void TestType::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      if (is_dirty() != p_Value) {
        // Set new value
        TYPE_SOA(TestType, dirty, bool) = p_Value;
      }
    }

    Low::Util::Name TestType::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(TestType, name, Low::Util::Name);
    }
    void TestType::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      if (get_name() != p_Value) {
        // Set new value
        TYPE_SOA(TestType, name, Low::Util::Name) = p_Value;
      }
    }

  } // namespace Util
} // namespace Low
