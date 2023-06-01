#include "LowUtilSerialization.h"

#include "LowUtilAssert.h"

namespace Low {
  namespace Util {
    namespace Serialization {
      void serialize(Yaml::Node &p_Node, Math::Quaternion &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
        p_Node["z"] = p_Value.z;
        p_Node["w"] = p_Value.w;
      }

      void serialize(Yaml::Node &p_Node, Math::Vector4 &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
        p_Node["z"] = p_Value.z;
        p_Node["a"] = p_Value.a;
      }

      void serialize(Yaml::Node &p_Node, Math::Vector3 &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
        p_Node["z"] = p_Value.z;
      }

      void serialize(Yaml::Node &p_Node, Math::Vector2 &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
      }

      void serialize(Yaml::Node &p_Node, Math::Box &p_Box)
      {
        serialize(p_Node["position"], p_Box.position);
        serialize(p_Node["rotation"], p_Box.rotation);
        serialize(p_Node["half_extents"], p_Box.halfExtents);
      }

      void serialize(Yaml::Node &p_Node, Math::Shape &p_Shape)
      {
        if (p_Shape.type == Math::ShapeType::BOX) {
          serialize(p_Node["box"], p_Shape.box);
        } else {
          LOW_ASSERT(false, "Serialization of shape type not supported");
        }
      }

      Math::Quaternion deserialize_quaternion(Yaml::Node &p_Node)
      {
        Math::Quaternion l_Result;
        l_Result.x = p_Node["x"].as<float>();
        l_Result.y = p_Node["y"].as<float>();
        l_Result.z = p_Node["z"].as<float>();
        l_Result.w = p_Node["w"].as<float>();

        return l_Result;
      }

      Math::Vector4 deserialize_vector4(Yaml::Node &p_Node)
      {
        Math::Vector4 l_Result;
        l_Result.x = p_Node["x"].as<float>();
        l_Result.y = p_Node["y"].as<float>();
        l_Result.z = p_Node["z"].as<float>();
        l_Result.a = p_Node["a"].as<float>();

        return l_Result;
      }

      Math::Vector3 deserialize_vector3(Yaml::Node &p_Node)
      {
        Math::Vector3 l_Result;
        l_Result.x = p_Node["x"].as<float>();
        l_Result.y = p_Node["y"].as<float>();
        l_Result.z = p_Node["z"].as<float>();

        return l_Result;
      }

      Math::Vector2 deserialize_vector2(Yaml::Node &p_Node)
      {
        Math::Vector2 l_Result;
        l_Result.x = p_Node["x"].as<float>();
        l_Result.y = p_Node["y"].as<float>();

        return l_Result;
      }

      Math::Box deserialize_box(Yaml::Node &p_Node)
      {
        Math::Box l_Result;

        l_Result.position = deserialize_vector3(p_Node["position"]);
        l_Result.rotation = deserialize_quaternion(p_Node["rotation"]);
        l_Result.halfExtents = deserialize_vector3(p_Node["half_extents"]);

        return l_Result;
      }

      Math::Shape deserialize_shape(Yaml::Node &p_Node)
      {
        Math::Shape l_Shape;
        if (p_Node["box"]) {
          l_Shape.type = Math::ShapeType::BOX;
          l_Shape.box = deserialize_box(p_Node["box"]);
        } else {
          LOW_ASSERT(false, "Could not deserialize shape");
        }

        return l_Shape;
      }
    } // namespace Serialization
  }   // namespace Util
} // namespace Low
