#include "LowUtilSerialization.h"

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
    } // namespace Serialization
  }   // namespace Util
} // namespace Low
