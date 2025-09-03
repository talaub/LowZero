#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"
#include "LowUtilName.h"

#include "LowMath.h"

namespace Low {
  namespace Util {
    namespace Resource {
      enum class Image2DFormat
      {
        RGBA8,
        RGB8,
        R8
      };

      struct Image2D
      {
        List<uint8_t> data;
        Math::UVector2 dimensions;
        uint8_t miplevel;
        Image2DFormat format;
        u64 size;
      };

      struct ImageMipMaps
      {
        Image2D mip0;
        Image2D mip1;
        Image2D mip2;
        Image2D mip3;
      };

#if 1
      struct Vertex
      {
        alignas(16) Math::Vector3 position;
        alignas(16) Math::Vector2 texture_coordinates;
        alignas(16) Math::Vector3 normal;
        alignas(16) Math::Vector3 tangent;
        alignas(16) Math::Vector3 bitangent;
      };
#else
      struct Vertex
      {
        Math::Vector3 position;
        Math::Vector2 texture_coordinates;
        Math::Vector3 normal;
        Math::Vector3 tangent;
        Math::Vector3 bitangent;
      };
#endif

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
        Util::Name name;
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

      LOW_EXPORT void load_image2d(String p_FilePath,
                                   Image2D &p_Image,
                                   uint8_t p_MipLevel = 0);

      LOW_EXPORT void load_image_mipmaps(String p_FilePath,
                                         ImageMipMaps &p_Image);

      LOW_EXPORT void load_mesh(String p_FilePath, Mesh &p_Mesh);
    } // namespace Resource
  } // namespace Util
} // namespace Low
