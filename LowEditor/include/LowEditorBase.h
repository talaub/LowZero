#pragma once

#include "LowEditorApi.h"

#include "LowUtilVariant.h"

namespace Low {
  namespace Editor {
    namespace Base {
      bool LOW_EDITOR_API Vector3Edit(const char *p_Label,
                                      Math::Vector3 *p_Vector3,
                                      float p_MaxWidth = -1.0f);
      bool LOW_EDITOR_API FloatEdit(const char *p_Label, float *p_Val,
                                    float p_Min = -1000.0f,
                                    float p_Max = 1000.0f,
                                    float p_Step = 0.1f);
      bool LOW_EDITOR_API ConciseFloatEdit(const char *p_Label,
                                           float *p_Val,
                                           float p_Min = -1000.0f,
                                           float p_Max = 1000.0f,
                                           float p_Step = 0.1f);
      bool LOW_EDITOR_API IntEdit(const char *p_Label, int *p_Val,
                                  int p_Min = LOW_INT_MIN,
                                  int p_Max = LOW_INT_MAX,
                                  int p_Step = 1);
      bool LOW_EDITOR_API UInt32Edit(const char *p_Label, u32 *p_Val,
                                     u32 p_Max = LOW_INT_MAX,
                                     u32 p_Step = 1);
      bool LOW_EDITOR_API NameEdit(const char *p_Label,
                                   Util::Name *p_Name);
      bool LOW_EDITOR_API StringEdit(const char *p_Label,
                                     Util::String *p_String);
      bool LOW_EDITOR_API BoolEdit(const char *p_Label, bool *p_Bool);

      bool LOW_EDITOR_API VariantEdit(const char *p_Label,
                                      Util::Variant &p_Variant,
                                      bool p_Concise = false);
    } // namespace Base
  }   // namespace Editor
} // namespace Low
