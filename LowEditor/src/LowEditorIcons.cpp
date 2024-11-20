#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    const char *get_icon_by_name(Util::Name p_IconName)
    {
      static const Util::Name l_Cube = N(cube);
      static const Util::Name l_File = N(file);
      static const Util::Name l_Sword = N(sword);
      static const Util::Name l_Trash = N(trash);
      static const Util::Name l_Plus = N(plus);
      static const Util::Name l_Key = N(key);
      static const Util::Name l_Fight = N(fight);
      static const Util::Name l_Search = N(search);
      static const Util::Name l_Clear = N(clear);
      static const Util::Name l_Position = N(position);
      static const Util::Name l_Physics = N(physics);
      static const Util::Name l_Sun = N(sun);
      static const Util::Name l_Bulb = N(bulb);
      static const Util::Name l_Cylinder = N(cylinder);

      if (p_IconName == l_Cube) {
        return LOW_EDITOR_ICON_CUBE;
      } else if (p_IconName == l_File) {
        return LOW_EDITOR_ICON_FILE;
      } else if (p_IconName == l_Sword) {
        return LOW_EDITOR_ICON_SWORD;
      } else if (p_IconName == l_Trash) {
        return LOW_EDITOR_ICON_TRASH;
      } else if (p_IconName == l_Plus) {
        return LOW_EDITOR_ICON_PLUS;
      } else if (p_IconName == l_Key) {
        return LOW_EDITOR_ICON_KEY;
      } else if (p_IconName == l_Fight) {
        return LOW_EDITOR_ICON_FIGHT;
      } else if (p_IconName == l_Search) {
        return LOW_EDITOR_ICON_SEARCH;
      } else if (p_IconName == l_Clear) {
        return LOW_EDITOR_ICON_CLEAR;
      } else if (p_IconName == l_Position) {
        return LOW_EDITOR_ICON_POSITION;
      } else if (p_IconName == l_Physics) {
        return LOW_EDITOR_ICON_PHYSICS;
      } else if (p_IconName == l_Sun) {
        return LOW_EDITOR_ICON_SUN;
      } else if (p_IconName == l_Bulb) {
        return LOW_EDITOR_ICON_BULB;
      } else if (p_IconName == l_Cylinder) {
        return LOW_EDITOR_ICON_CYLINDER;
      }

      return LOW_EDITOR_ICON_MISSING;
    }
  } // namespace Editor
} // namespace Low
