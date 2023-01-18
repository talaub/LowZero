#pragma once

#include "LowMath.h"

namespace Low {
  namespace Math {
    struct Cylinder
    {
      Vector3 m_Position;
      Quaternion m_Rotation;
      float m_Radius;
      float m_Height;
    };

    struct Box
    {
      Vector3 m_Position;
      Quaternion m_Rotation;
      Vector3 m_HalfExtents;
    };

    struct Sphere
    {
      Vector3 m_Position;
      float m_Radius;
    };

    namespace ShapeType {
      enum Enum
      {
        Sphere,
        Box,
        Cylinder
      };
    }

    struct Shape
    {
      uint8_t m_Type;

      union
      {
        Cylinder m_Cylinder;
        Sphere m_Sphere;
        Box m_Box;
      } m_Data;
    };
  } // namespace Math
} // namespace Low
