#include "LowCoreTweenEase.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Core {
    namespace TweenEaseEnumHelper {
      void initialize()
      {
        Low::Util::RTTI::EnumInfo l_EnumInfo;
        l_EnumInfo.name = N(TweenEase);
        l_EnumInfo.enumId = 13;
        l_EnumInfo.entry_name = &_entry_name;
        l_EnumInfo.entry_value = &_entry_value;

        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Linear);
          l_Entry.value = 0;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(InQuad);
          l_Entry.value = 1;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(OutQuad);
          l_Entry.value = 2;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(InOutQuad);
          l_Entry.value = 3;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(InCubic);
          l_Entry.value = 4;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(OutCubic);
          l_Entry.value = 5;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(InOutCubic);
          l_Entry.value = 6;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(InQuart);
          l_Entry.value = 7;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(OutQuart);
          l_Entry.value = 8;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(InOutQuart);
          l_Entry.value = 9;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(InBack);
          l_Entry.value = 10;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(OutBack);
          l_Entry.value = 11;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(InOutBack);
          l_Entry.value = 12;

          l_EnumInfo.entries.push_back(l_Entry);
        }

        Low::Util::register_enum_info(13, l_EnumInfo);
      }

      void cleanup()
      {
      }

      Low::Util::Name entry_name(Low::Core::TweenEase p_Value)
      {
        if (p_Value == TweenEase::LINEAR) {
          return N(Linear);
        }
        if (p_Value == TweenEase::INQUAD) {
          return N(InQuad);
        }
        if (p_Value == TweenEase::OUTQUAD) {
          return N(OutQuad);
        }
        if (p_Value == TweenEase::INOUTQUAD) {
          return N(InOutQuad);
        }
        if (p_Value == TweenEase::INCUBIC) {
          return N(InCubic);
        }
        if (p_Value == TweenEase::OUTCUBIC) {
          return N(OutCubic);
        }
        if (p_Value == TweenEase::INOUTCUBIC) {
          return N(InOutCubic);
        }
        if (p_Value == TweenEase::INQUART) {
          return N(InQuart);
        }
        if (p_Value == TweenEase::OUTQUART) {
          return N(OutQuart);
        }
        if (p_Value == TweenEase::INOUTQUART) {
          return N(InOutQuart);
        }
        if (p_Value == TweenEase::INBACK) {
          return N(InBack);
        }
        if (p_Value == TweenEase::OUTBACK) {
          return N(OutBack);
        }
        if (p_Value == TweenEase::INOUTBACK) {
          return N(InOutBack);
        }

        LOW_ASSERT(false, "Could not find entry in enum TweenEase.");
        return N(EMPTY);
      }

      Low::Util::Name _entry_name(uint8_t p_Value)
      {
        Low::Core::TweenEase l_Enum =
            static_cast<Low::Core::TweenEase>(p_Value);
        return entry_name(l_Enum);
      }

      Low::Core::TweenEase entry_value(Low::Util::Name p_Name)
      {
        if (p_Name == N(Linear)) {
          return Low::Core::TweenEase::LINEAR;
        }
        if (p_Name == N(InQuad)) {
          return Low::Core::TweenEase::INQUAD;
        }
        if (p_Name == N(OutQuad)) {
          return Low::Core::TweenEase::OUTQUAD;
        }
        if (p_Name == N(InOutQuad)) {
          return Low::Core::TweenEase::INOUTQUAD;
        }
        if (p_Name == N(InCubic)) {
          return Low::Core::TweenEase::INCUBIC;
        }
        if (p_Name == N(OutCubic)) {
          return Low::Core::TweenEase::OUTCUBIC;
        }
        if (p_Name == N(InOutCubic)) {
          return Low::Core::TweenEase::INOUTCUBIC;
        }
        if (p_Name == N(InQuart)) {
          return Low::Core::TweenEase::INQUART;
        }
        if (p_Name == N(OutQuart)) {
          return Low::Core::TweenEase::OUTQUART;
        }
        if (p_Name == N(InOutQuart)) {
          return Low::Core::TweenEase::INOUTQUART;
        }
        if (p_Name == N(InBack)) {
          return Low::Core::TweenEase::INBACK;
        }
        if (p_Name == N(OutBack)) {
          return Low::Core::TweenEase::OUTBACK;
        }
        if (p_Name == N(InOutBack)) {
          return Low::Core::TweenEase::INOUTBACK;
        }

        LOW_ASSERT(false, "Could not find entry in enum TweenEase.");
        return static_cast<Low::Core::TweenEase>(0);
      }

      uint8_t _entry_value(Low::Util::Name p_Name)
      {
        return static_cast<uint8_t>(entry_value(p_Name));
      }

      u16 get_enum_id()
      {
        return 13;
      }

      u8 get_entry_count()
      {
        return 13;
      }
    } // namespace TweenEaseEnumHelper
  } // namespace Core
} // namespace Low
