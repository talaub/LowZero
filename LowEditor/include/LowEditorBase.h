#pragma once

#include "LowUtilVariant.h"

namespace Low {
  namespace Editor {
    namespace Base {
      bool Vector3Edit(const char *p_Label, Math::Vector3 *p_Vector3);
      bool FloatEdit(const char *p_Label, float *p_Val,
                     float p_Min = -1000.0f, float p_Max = 1000.0f,
                     float p_Step = 0.1f);
      bool IntEdit(const char *p_Label, int *p_Val,
                   int p_Min = LOW_INT_MIN, int p_Max = LOW_INT_MAX,
                   int p_Step = 1);
      bool NameEdit(const char *p_Label, Util::Name *p_Name);
      bool StringEdit(const char *p_Label, Util::String *p_String);
      bool BoolEdit(const char *p_Label, bool *p_Bool);

      bool VariantEdit(const char *p_Label, Util::Variant &p_Variant);
    } // namespace Base
  }   // namespace Editor
} // namespace Low
