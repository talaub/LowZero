#include "LowCorePhysicsBodyMotionType.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Core {
    namespace Physics {
      namespace BodyMotionTypeEnumHelper {
        void initialize()
        {
          Low::Util::RTTI::EnumInfo l_EnumInfo;
          l_EnumInfo.name = N(BodyMotionType);
          l_EnumInfo.enumId = 14;
          l_EnumInfo.entry_name = &_entry_name;
          l_EnumInfo.entry_value = &_entry_value;

          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Static);
            l_Entry.value = 0;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Kinematic);
            l_Entry.value = 1;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Dynamic);
            l_Entry.value = 2;

            l_EnumInfo.entries.push_back(l_Entry);
          }

          Low::Util::register_enum_info(14, l_EnumInfo);
        }

        void cleanup()
        {
        }

        Low::Util::Name
        entry_name(Low::Core::Physics::BodyMotionType p_Value)
        {
          if (p_Value == BodyMotionType::STATIC) {
            return N(Static);
          }
          if (p_Value == BodyMotionType::KINEMATIC) {
            return N(Kinematic);
          }
          if (p_Value == BodyMotionType::DYNAMIC) {
            return N(Dynamic);
          }

          LOW_ASSERT(false,
                     "Could not find entry in enum BodyMotionType.");
          return N(EMPTY);
        }

        Low::Util::Name _entry_name(uint8_t p_Value)
        {
          Low::Core::Physics::BodyMotionType l_Enum =
              static_cast<Low::Core::Physics::BodyMotionType>(
                  p_Value);
          return entry_name(l_Enum);
        }

        Low::Core::Physics::BodyMotionType
        entry_value(Low::Util::Name p_Name)
        {
          if (p_Name == N(Static)) {
            return Low::Core::Physics::BodyMotionType::STATIC;
          }
          if (p_Name == N(Kinematic)) {
            return Low::Core::Physics::BodyMotionType::KINEMATIC;
          }
          if (p_Name == N(Dynamic)) {
            return Low::Core::Physics::BodyMotionType::DYNAMIC;
          }

          LOW_ASSERT(false,
                     "Could not find entry in enum BodyMotionType.");
          return static_cast<Low::Core::Physics::BodyMotionType>(0);
        }

        uint8_t _entry_value(Low::Util::Name p_Name)
        {
          return static_cast<uint8_t>(entry_value(p_Name));
        }

        u16 get_enum_id()
        {
          return 14;
        }

        u8 get_entry_count()
        {
          return 3;
        }
      } // namespace BodyMotionTypeEnumHelper
    } // namespace Physics
  } // namespace Core
} // namespace Low
