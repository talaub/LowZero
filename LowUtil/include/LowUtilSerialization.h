#pragma once

#include "LowUtilApi.h"

#include "LowUtilYaml.h"

#include "LowMath.h"

namespace Low {
  namespace Util {
    namespace Serialization {
      void LOW_EXPORT serialize(Yaml::Node &p_Node, Math::Vector4 &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node, Math::Vector3 &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node, Math::Vector2 &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node, Math::Quaternion &p_Value);

      Math::Quaternion LOW_EXPORT deserialize_quaternion(Yaml::Node &p_Node);
      Math::Vector4 LOW_EXPORT deserialize_vector4(Yaml::Node &p_Node);
      Math::Vector3 LOW_EXPORT deserialize_vector3(Yaml::Node &p_Node);
      Math::Vector2 LOW_EXPORT deserialize_vector2(Yaml::Node &p_Node);
    } // namespace Serialization
  }   // namespace Util
} // namespace Low
