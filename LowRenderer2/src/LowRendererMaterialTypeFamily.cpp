#include "LowRendererMaterialTypeFamily.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Renderer {
    namespace MaterialTypeFamilyEnumHelper {
      void initialize()
      {
        Low::Util::RTTI::EnumInfo l_EnumInfo;
        l_EnumInfo.name = N(MaterialTypeFamily);
        l_EnumInfo.enumId = 5;
        l_EnumInfo.entry_name = &_entry_name;
        l_EnumInfo.entry_value = &_entry_value;

        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Solid);
          l_Entry.value = 0;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Ui);
          l_Entry.value = 1;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(DebugGeometry);
          l_Entry.value = 2;

          l_EnumInfo.entries.push_back(l_Entry);
        }

        Low::Util::register_enum_info(5, l_EnumInfo);
      }

      void cleanup()
      {
      }

      Low::Util::Name
      entry_name(Low::Renderer::MaterialTypeFamily p_Value)
      {
        if (p_Value == MaterialTypeFamily::SOLID) {
          return N(Solid);
        }
        if (p_Value == MaterialTypeFamily::UI) {
          return N(Ui);
        }
        if (p_Value == MaterialTypeFamily::DEBUGGEOMETRY) {
          return N(DebugGeometry);
        }

        LOW_ASSERT(
            false,
            "Could not find entry in enum MaterialTypeFamily.");
        return N(EMPTY);
      }

      Low::Util::Name _entry_name(uint8_t p_Value)
      {
        Low::Renderer::MaterialTypeFamily l_Enum =
            static_cast<Low::Renderer::MaterialTypeFamily>(p_Value);
        return entry_name(l_Enum);
      }

      Low::Renderer::MaterialTypeFamily
      entry_value(Low::Util::Name p_Name)
      {
        if (p_Name == N(Solid)) {
          return Low::Renderer::MaterialTypeFamily::SOLID;
        }
        if (p_Name == N(Ui)) {
          return Low::Renderer::MaterialTypeFamily::UI;
        }
        if (p_Name == N(DebugGeometry)) {
          return Low::Renderer::MaterialTypeFamily::DEBUGGEOMETRY;
        }

        LOW_ASSERT(
            false,
            "Could not find entry in enum MaterialTypeFamily.");
        return static_cast<Low::Renderer::MaterialTypeFamily>(0);
      }

      uint8_t _entry_value(Low::Util::Name p_Name)
      {
        return static_cast<uint8_t>(entry_value(p_Name));
      }

      u16 get_enum_id()
      {
        return 5;
      }

      u8 get_entry_count()
      {
        return 3;
      }
    } // namespace MaterialTypeFamilyEnumHelper
  }   // namespace Renderer
} // namespace Low
