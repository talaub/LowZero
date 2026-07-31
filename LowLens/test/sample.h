#pragma once

#include "LowUtilContainers.h"

#define LOW_FUNCTION(...)
#define LOW_ENUM(...)
#define LOW_STRUCT(...)
#define LOW_FIELD(...)
#define LOW_PARAM(...)

namespace Low {
  struct Hit;

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

    LOW_FUNCTION(bind_name="Raycast", bind_namespace="Physics", scripting)
    bool raycast(LOW_PARAM(in) const Vector3 &origin,
                 LOW_PARAM(out, bind_name="hit") Hit *hit);

  } // namespace Math

  LOW_STRUCT(scripting)
  struct Hit
  {
    LOW_FIELD()
    float distance;

    LOW_FIELD()
    int object_id;

    LOW_FIELD()
    Low::Util::String label;

    LOW_FIELD()
    Low::Util::List<float> weights;
  };

  LOW_FUNCTION(scripting)
  void consume_hit(LOW_PARAM(value) Hit *hit);

  LOW_FUNCTION(scripting)
  Low::Util::String format_hit(const Low::Util::String &prefix,
                               LOW_PARAM(out) Low::Util::String *label);

  LOW_FUNCTION(scripting)
  Low::Util::List<float>
  normalize_weights(const Low::Util::List<float> &weights,
                    LOW_PARAM(out) Low::Util::List<float> *normalized);

} // namespace Low

#include "sample.gen.h"
