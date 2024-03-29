#pragma once

#include "LowUtilApi.h"

#include "LowUtilYaml.h"
#include "LowUtilVariant.h"

#include "LowMath.h"

namespace Low {
  namespace Util {
    namespace Serialization {
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Vector4 &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Vector3 &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Vector2 &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Quaternion &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::UVector2 &p_Value);

      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Shape &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Box &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Sphere &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Cone &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Cylinder &p_Value);

      void LOW_EXPORT serialize_variant(Yaml::Node &p_Node,
                                        Variant p_Variant);

      void LOW_EXPORT serialize_enum(Yaml::Node &p_Node, u16 p_EnumId,
                                     u8 p_EnumValue);

      Math::Quaternion LOW_EXPORT
      deserialize_quaternion(Yaml::Node &p_Node);
      Math::Vector4 LOW_EXPORT
      deserialize_vector4(Yaml::Node &p_Node);
      Math::Vector3 LOW_EXPORT
      deserialize_vector3(Yaml::Node &p_Node);
      Math::Vector2 LOW_EXPORT
      deserialize_vector2(Yaml::Node &p_Node);
      Math::UVector2 LOW_EXPORT
      deserialize_uvector2(Yaml::Node &p_Node);

      Math::Box LOW_EXPORT deserialize_box(Yaml::Node &p_Node);
      Math::Shape LOW_EXPORT deserialize_shape(Yaml::Node &p_Node);

      Variant LOW_EXPORT deserialize_variant(Yaml::Node &p_Node);

      u8 LOW_EXPORT deserialize_enum(Yaml::Node &p_Node);
    } // namespace Serialization
  }   // namespace Util
} // namespace Low
