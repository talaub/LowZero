#pragma once

#include "LowUtilApi.h"

#include "LowUtilYaml.h"
#include "LowUtilVariant.h"
#include "LowUtilHashing.h"

#include "LowMath.h"

#define LOW_SERIALIZATION_HANDLE_FROM_UNIQUE_ID(x)                   \
  Low::Util::find_handle_by_unique_id(x.as<Low::Util::UniqueId>())   \
      .get_id()

#define LOW_SERIALIZATION_GET_HASH(x)                                \
  Low::Util::string_to_hash(LOW_YAML_AS_STRING(x))
#define LOW_SERIALIZATION_SET_HASH(x, y)                             \
  x = Low::Util::hash_to_string(y).c_str()

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
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::Bounds &p_Value);
      void LOW_EXPORT serialize(Yaml::Node &p_Node,
                                Math::AABB &p_Value);

      void LOW_EXPORT serialize_handle(Yaml::Node &p_Node,
                                       Handle p_Handle);

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
      Math::IVector2 LOW_EXPORT
      deserialize_ivector2(Yaml::Node &p_Node);

      Math::Box LOW_EXPORT deserialize_box(Yaml::Node &p_Node);
      Math::Sphere LOW_EXPORT deserialize_sphere(Yaml::Node &p_Node);
      Math::Shape LOW_EXPORT deserialize_shape(Yaml::Node &p_Node);
      Math::Bounds LOW_EXPORT deserialize_bounds(Yaml::Node &p_Node);
      Math::AABB LOW_EXPORT deserialize_aabb(Yaml::Node &p_Node);

      Variant LOW_EXPORT deserialize_variant(Yaml::Node &p_Node);

      u8 LOW_EXPORT deserialize_enum(Yaml::Node &p_Node);

      Handle LOW_EXPORT deserialize_handle(Yaml::Node &p_Node);
    } // namespace Serialization
  } // namespace Util
} // namespace Low
