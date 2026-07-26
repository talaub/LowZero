#pragma once

#define LOW_FUNCTION(...)
#define LOW_ENUM(...)
#define LOW_STRUCT(...)
#define LOW_FIELD(...)

namespace Low {
  namespace Math {

    LOW_ENUM(scripting, bind_name="Axis", bind_namespace="Math")
    enum class Axis
    {
      X,
      Y,
      Z
    };

    LOW_FUNCTION(bind_name="Lerp", bind_namespace="Math", property, scripting)
    float lerp(float a, float b, float t);

  } // namespace Math

  LOW_STRUCT(scripting)
  struct Hit
  {
    LOW_FIELD()
    float distance;

    LOW_FIELD()
    int object_id;
  };

} // namespace Low

#include "sample.gen.h"
