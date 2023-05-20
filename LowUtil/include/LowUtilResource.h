#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"
#include "LowUtilName.h"

#include "LowMath.h"

namespace Low {
  namespace Util {
    namespace Resource {
      struct Image2D
      {
        List<List<uint8_t>> data;
        List<Math::UVector2> dimensions;
      };

      struct Vertex
      {
        Math::Vector3 position;
        Math::Vector2 texture_coordinates;
        Math::Vector3 normal;
        Math::Vector3 tangent;
        Math::Vector3 bitangent;
      };

      struct BoneVertexWeight
      {
        uint32_t vertexIndex;
        float weight;
      };

      struct Bone
      {
        Name name;
        List<BoneVertexWeight> weights;
      };

      struct AnimationVector3Key
      {
        float timestamp;
        Math::Vector3 value;
      };

      struct AnimationVector4Key
      {
        float timestamp;
        Math::Vector4 value;
      };

      struct AnimationChannel
      {
        Name boneName;
        Util::List<AnimationVector3Key> positions;
        Util::List<AnimationVector4Key> rotations;
        Util::List<AnimationVector3Key> scales;
      };

      struct Animation
      {
        Name name;
        float duration;
        List<AnimationChannel> channels;
      };

      struct MeshInfo
      {
        List<Vertex> vertices;
        List<uint32_t> indices;
        List<Bone> bones;
      };

      struct Submesh
      {
        Math::Matrix4x4 transform;
        List<MeshInfo> meshInfos;
      };

      struct Mesh
      {
        List<Submesh> submeshes;
        List<Animation> animations;
      };

      LOW_EXPORT void load_image2d(String p_FilePath, Image2D &p_Image);

      LOW_EXPORT void load_mesh(String p_FilePath, Mesh &p_Mesh);
    } // namespace Resource
  }   // namespace Util
} // namespace Low
