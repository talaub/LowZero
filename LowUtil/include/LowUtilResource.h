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

      struct Bone
      {
        uint32_t index;
        Math::Matrix4x4 offset;
      };

      struct AnimationChannel
      {
        Name boneName;
        Util::List<AnimationVector3Key> positions;
        Util::List<AnimationVector4Key> rotations;
        Util::List<AnimationVector3Key> scales;
      };

      struct BoneVertexWeight
      {
        uint32_t boneIndex;
        uint32_t vertexIndex;
        float weight;
      };

      struct MeshInfo
      {
        List<Vertex> vertices;
        List<uint32_t> indices;
        List<BoneVertexWeight> boneInfluences;
      };

      struct Submesh
      {
        Math::Matrix4x4 transform;
        Math::Matrix4x4 parentTransform;
        Math::Matrix4x4 localTransform;
        List<MeshInfo> meshInfos;
        Util::Name name;
      };

      struct Animation
      {
        Name name;
        float duration;
        float ticksPerSecond;
        List<AnimationChannel> channels;
      };

      struct Node
      {
        uint32_t index;
        List<Node> children;
      };

      struct Mesh
      {
        List<Submesh> submeshes;
        Map<Name, Bone> bones;
        uint32_t boneCount;
        List<Animation> animations;
        Node rootNode;
      };

      LOW_EXPORT void load_image2d(String p_FilePath, Image2D &p_Image);

      LOW_EXPORT void load_mesh(String p_FilePath, Mesh &p_Mesh);
    } // namespace Resource
  }   // namespace Util
} // namespace Low
