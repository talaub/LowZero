#include "LowRendererEditorImageType.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Renderer {
    namespace EditorImageTypeEnumHelper {
      void initialize()
      {
        Low::Util::RTTI::EnumInfo l_EnumInfo;
        l_EnumInfo.name = N(EditorImageType);
        l_EnumInfo.enumId = 6;
        l_EnumInfo.entry_name = &_entry_name;
        l_EnumInfo.entry_value = &_entry_value;

        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Mesh);
          l_Entry.value = 0;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Icon);
          l_Entry.value = 1;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Texture);
          l_Entry.value = 2;

          l_EnumInfo.entries.push_back(l_Entry);
        }

        Low::Util::register_enum_info(6, l_EnumInfo);
      }

      void cleanup()
      {
      }

      Low::Util::Name
      entry_name(Low::Renderer::EditorImageType p_Value)
      {
        if (p_Value == EditorImageType::MESH) {
          return N(Mesh);
        }
        if (p_Value == EditorImageType::ICON) {
          return N(Icon);
        }
        if (p_Value == EditorImageType::TEXTURE) {
          return N(Texture);
        }

        LOW_ASSERT(false,
                   "Could not find entry in enum EditorImageType.");
        return N(EMPTY);
      }

      Low::Util::Name _entry_name(uint8_t p_Value)
      {
        Low::Renderer::EditorImageType l_Enum =
            static_cast<Low::Renderer::EditorImageType>(p_Value);
        return entry_name(l_Enum);
      }

      Low::Renderer::EditorImageType
      entry_value(Low::Util::Name p_Name)
      {
        if (p_Name == N(Mesh)) {
          return Low::Renderer::EditorImageType::MESH;
        }
        if (p_Name == N(Icon)) {
          return Low::Renderer::EditorImageType::ICON;
        }
        if (p_Name == N(Texture)) {
          return Low::Renderer::EditorImageType::TEXTURE;
        }

        LOW_ASSERT(false,
                   "Could not find entry in enum EditorImageType.");
        return static_cast<Low::Renderer::EditorImageType>(0);
      }

      uint8_t _entry_value(Low::Util::Name p_Name)
      {
        return static_cast<uint8_t>(entry_value(p_Name));
      }

      u16 get_enum_id()
      {
        return 6;
      }

      u8 get_entry_count()
      {
        return 3;
      }
    } // namespace EditorImageTypeEnumHelper
  } // namespace Renderer
} // namespace Low
