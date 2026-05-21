#include "LowRendererMeshType.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Renderer {
    namespace MeshTypeEnumHelper {
      void initialize()
      {
        Low::Util::RTTI::EnumInfo l_EnumInfo;
        l_EnumInfo.name = N(MeshType);
        l_EnumInfo.enumId = 11;
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
          l_Entry.name = N(Skeletal);
          l_Entry.value = 1;

          l_EnumInfo.entries.push_back(l_Entry);
        }

        Low::Util::register_enum_info(11, l_EnumInfo);
      }

      void cleanup()
      {
      }

      Low::Util::Name entry_name(Low::Renderer::MeshType p_Value)
      {
        if (p_Value == MeshType::STATIC) {
          return N(Static);
        }
        if (p_Value == MeshType::SKELETAL) {
          return N(Skeletal);
        }

        LOW_ASSERT(false, "Could not find entry in enum MeshType.");
        return N(EMPTY);
      }

      Low::Util::Name _entry_name(uint8_t p_Value)
      {
        Low::Renderer::MeshType l_Enum =
            static_cast<Low::Renderer::MeshType>(p_Value);
        return entry_name(l_Enum);
      }

      Low::Renderer::MeshType entry_value(Low::Util::Name p_Name)
      {
        if (p_Name == N(Static)) {
          return Low::Renderer::MeshType::STATIC;
        }
        if (p_Name == N(Skeletal)) {
          return Low::Renderer::MeshType::SKELETAL;
        }

        LOW_ASSERT(false, "Could not find entry in enum MeshType.");
        return static_cast<Low::Renderer::MeshType>(0);
      }

      uint8_t _entry_value(Low::Util::Name p_Name)
      {
        return static_cast<uint8_t>(entry_value(p_Name));
      }

      u16 get_enum_id()
      {
        return 11;
      }

      u8 get_entry_count()
      {
        return 2;
      }
    } // namespace MeshTypeEnumHelper
  } // namespace Renderer
} // namespace Low
