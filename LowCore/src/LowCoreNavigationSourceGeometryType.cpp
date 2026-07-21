#include "LowCoreNavigationSourceGeometryType.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Core {
    namespace Navigation {
      namespace SourceGeometryTypeEnumHelper {
        void initialize()
        {
          Low::Util::RTTI::EnumInfo l_EnumInfo;
          l_EnumInfo.name = N(SourceGeometryType);
          l_EnumInfo.enumId = 16;
          l_EnumInfo.entry_name = &_entry_name;
          l_EnumInfo.entry_value = &_entry_value;

          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Collider);
            l_Entry.value = 0;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(RenderMesh);
            l_Entry.value = 1;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Terrain);
            l_Entry.value = 2;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(CustomMesh);
            l_Entry.value = 3;

            l_EnumInfo.entries.push_back(l_Entry);
          }

          Low::Util::register_enum_info(16, l_EnumInfo);
        }

        void cleanup()
        {
        }

        Low::Util::Name
        entry_name(Low::Core::Navigation::SourceGeometryType p_Value)
        {
          if (p_Value == SourceGeometryType::COLLIDER) {
            return N(Collider);
          }
          if (p_Value == SourceGeometryType::RENDERMESH) {
            return N(RenderMesh);
          }
          if (p_Value == SourceGeometryType::TERRAIN) {
            return N(Terrain);
          }
          if (p_Value == SourceGeometryType::CUSTOMMESH) {
            return N(CustomMesh);
          }

          LOW_ASSERT(
              false,
              "Could not find entry in enum SourceGeometryType.");
          return N(EMPTY);
        }

        Low::Util::Name _entry_name(uint8_t p_Value)
        {
          Low::Core::Navigation::SourceGeometryType l_Enum =
              static_cast<Low::Core::Navigation::SourceGeometryType>(
                  p_Value);
          return entry_name(l_Enum);
        }

        Low::Core::Navigation::SourceGeometryType
        entry_value(Low::Util::Name p_Name)
        {
          if (p_Name == N(Collider)) {
            return Low::Core::Navigation::SourceGeometryType::
                COLLIDER;
          }
          if (p_Name == N(RenderMesh)) {
            return Low::Core::Navigation::SourceGeometryType::
                RENDERMESH;
          }
          if (p_Name == N(Terrain)) {
            return Low::Core::Navigation::SourceGeometryType::TERRAIN;
          }
          if (p_Name == N(CustomMesh)) {
            return Low::Core::Navigation::SourceGeometryType::
                CUSTOMMESH;
          }

          LOW_ASSERT(
              false,
              "Could not find entry in enum SourceGeometryType.");
          return static_cast<
              Low::Core::Navigation::SourceGeometryType>(0);
        }

        uint8_t _entry_value(Low::Util::Name p_Name)
        {
          return static_cast<uint8_t>(entry_value(p_Name));
        }

        u16 get_enum_id()
        {
          return 16;
        }

        u8 get_entry_count()
        {
          return 4;
        }
      } // namespace SourceGeometryTypeEnumHelper
    } // namespace Navigation
  } // namespace Core
} // namespace Low
