#include "LowCoreScriptAssetGenerator.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Core {
    namespace Scripting {
      namespace AssetGeneratorEnumHelper {
        void initialize()
        {
          Low::Util::RTTI::EnumInfo l_EnumInfo;
          l_EnumInfo.name = N(AssetGenerator);
          l_EnumInfo.enumId = 9;
          l_EnumInfo.entry_name = &_entry_name;
          l_EnumInfo.entry_value = &_entry_value;

          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(UserAuthored);
            l_Entry.value = 0;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(VisualScript);
            l_Entry.value = 1;

            l_EnumInfo.entries.push_back(l_Entry);
          }

          Low::Util::register_enum_info(9, l_EnumInfo);
        }

        void cleanup()
        {
        }

        Low::Util::Name
        entry_name(Low::Core::Scripting::AssetGenerator p_Value)
        {
          if (p_Value == AssetGenerator::USERAUTHORED) {
            return N(UserAuthored);
          }
          if (p_Value == AssetGenerator::VISUALSCRIPT) {
            return N(VisualScript);
          }

          LOW_ASSERT(false,
                     "Could not find entry in enum AssetGenerator.");
          return N(EMPTY);
        }

        Low::Util::Name _entry_name(uint8_t p_Value)
        {
          Low::Core::Scripting::AssetGenerator l_Enum =
              static_cast<Low::Core::Scripting::AssetGenerator>(
                  p_Value);
          return entry_name(l_Enum);
        }

        Low::Core::Scripting::AssetGenerator
        entry_value(Low::Util::Name p_Name)
        {
          if (p_Name == N(UserAuthored)) {
            return Low::Core::Scripting::AssetGenerator::USERAUTHORED;
          }
          if (p_Name == N(VisualScript)) {
            return Low::Core::Scripting::AssetGenerator::VISUALSCRIPT;
          }

          LOW_ASSERT(false,
                     "Could not find entry in enum AssetGenerator.");
          return static_cast<Low::Core::Scripting::AssetGenerator>(0);
        }

        uint8_t _entry_value(Low::Util::Name p_Name)
        {
          return static_cast<uint8_t>(entry_value(p_Name));
        }

        u16 get_enum_id()
        {
          return 9;
        }

        u8 get_entry_count()
        {
          return 2;
        }
      } // namespace AssetGeneratorEnumHelper
    } // namespace Scripting
  } // namespace Core
} // namespace Low
