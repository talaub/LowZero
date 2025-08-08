#include "LowUtilResource.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/convert.hpp>
#include <stdint.h>
#include <string>
#include <vcruntime_string.h>

#include <assimp/Importer.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/vector3.h>

namespace Low {
  namespace Util {
    namespace Resource {
      inline static Math::Matrix4x4
      Assimp2Glm(const aiMatrix4x4 &from)
      {
        return Math::Matrix4x4(
            (double)from.a1, (double)from.b1, (double)from.c1,
            (double)from.d1, (double)from.a2, (double)from.b2,
            (double)from.c2, (double)from.d2, (double)from.a3,
            (double)from.b3, (double)from.c3, (double)from.d3,
            (double)from.a4, (double)from.b4, (double)from.c4,
            (double)from.d4);
      }

      static void load_mipmap(Image2D &p_Image,
                              gli::texture2d p_Texture, u8 p_MipLevel)
      {
        LOW_ASSERT(p_MipLevel < 4, "Requested miplevel out of range");

        const uint32_t l_Channels = 4u;
        p_Image.data.resize(p_Texture.levels());

        p_Image.dimensions.x = p_Texture.extent(p_MipLevel).x;
        p_Image.dimensions.y = p_Texture.extent(p_MipLevel).y;

        p_Image.data.resize(p_Image.dimensions.x *
                            p_Image.dimensions.y * l_Channels);
        memcpy(p_Image.data.data(), p_Texture.data(0, 0, p_MipLevel),
               p_Image.data.size());

        p_Image.size = p_Image.data.size();

        p_Image.format = Image2DFormat::RGBA8;
        p_Image.miplevel = p_MipLevel;
      }

      void load_image_mipmaps(String p_FilePath,
                              ImageMipMaps &p_Image)
      {
        gli::texture2d l_Texture(gli::load(p_FilePath.c_str()));
        LOW_ASSERT(!l_Texture.empty(), "Could not load file");

        LOW_ASSERT(l_Texture.target() == gli::TARGET_2D,
                   "Expected Image2D data file");

        load_mipmap(p_Image.mip0, l_Texture, 0);
        load_mipmap(p_Image.mip1, l_Texture, 1);
        load_mipmap(p_Image.mip2, l_Texture, 2);
        load_mipmap(p_Image.mip3, l_Texture, 3);
      }

      void load_image2d(String p_FilePath, Image2D &p_Image,
                        uint8_t p_MipLevel)
      {
        gli::texture2d l_Texture(gli::load(p_FilePath.c_str()));
        LOW_ASSERT(!l_Texture.empty(), "Could not load file");

        LOW_ASSERT(l_Texture.target() == gli::TARGET_2D,
                   "Expected Image2D data file");

        load_mipmap(p_Image, l_Texture, p_MipLevel);
      }

      static void parse_mesh(const aiMesh *p_AiMesh,
                             MeshInfo &p_MeshInfo, Mesh &p_Mesh)
      {
        LOW_ASSERT(p_AiMesh->HasPositions(),
                   "Mesh has no position information");
        LOW_ASSERT(p_AiMesh->HasNormals(),
                   "Mesh has no normal information");
        LOW_ASSERT(p_AiMesh->HasTextureCoords(0),
                   "Mesh has no texture coordinates");
        LOW_ASSERT(p_AiMesh->HasTangentsAndBitangents(),
                   "Mesh has no tangent/bitangent information");

        p_MeshInfo.name = LOW_NAME(p_AiMesh->mName.C_Str());

        p_MeshInfo.vertices.resize(p_AiMesh->mNumVertices);
        for (uint32_t i = 0u; i < p_AiMesh->mNumVertices; ++i) {
          p_MeshInfo.vertices[i].position = {
              p_AiMesh->mVertices[i].x, p_AiMesh->mVertices[i].y,
              p_AiMesh->mVertices[i].z};

          p_MeshInfo.vertices[i].texture_coordinates = {
              p_AiMesh->mTextureCoords[0][i].x,
              p_AiMesh->mTextureCoords[0][i].y};

          p_MeshInfo.vertices[i].normal = {p_AiMesh->mNormals[i].x,
                                           p_AiMesh->mNormals[i].y,
                                           p_AiMesh->mNormals[i].z};

          p_MeshInfo.vertices[i].tangent = {p_AiMesh->mTangents[i].x,
                                            p_AiMesh->mTangents[i].y,
                                            p_AiMesh->mTangents[i].z};
          p_MeshInfo.vertices[i].bitangent = {
              p_AiMesh->mBitangents[i].x, p_AiMesh->mBitangents[i].y,
              p_AiMesh->mBitangents[i].z};
        }

        LOW_ASSERT(p_AiMesh->HasFaces(),
                   "Mesh has no index information");
        for (uint32_t i = 0u; i < p_AiMesh->mNumFaces; ++i) {
          for (uint32_t j = 0u; j < p_AiMesh->mFaces[i].mNumIndices;
               ++j) {
            p_MeshInfo.indices.push_back(
                p_AiMesh->mFaces[i].mIndices[j]);
          }
        }

        if (p_AiMesh->HasBones()) {
          for (uint32_t i = 0u; i < p_AiMesh->mNumBones; ++i) {
            uint32_t i_BoneIndex = 0;

            Name i_BoneName =
                LOW_NAME(p_AiMesh->mBones[i]->mName.C_Str());

            if (p_Mesh.bones.find(i_BoneName) == p_Mesh.bones.end()) {
              Bone i_Bone;
              i_Bone.index = p_Mesh.boneCount;
              i_Bone.offset =
                  Assimp2Glm(p_AiMesh->mBones[i]->mOffsetMatrix);
              p_Mesh.bones[i_BoneName] = i_Bone;

              p_Mesh.boneCount++;
            }

            i_BoneIndex = p_Mesh.bones[i_BoneName].index;

            auto i_Weights = p_AiMesh->mBones[i]->mWeights;
            uint32_t i_WeightCount = p_AiMesh->mBones[i]->mNumWeights;

            for (uint32_t j = 0u; j < i_WeightCount; ++j) {
              BoneVertexWeight i_Weight;
              i_Weight.boneIndex = i_BoneIndex;
              i_Weight.weight = i_Weights[j].mWeight;
              i_Weight.vertexIndex = i_Weights[j].mVertexId;

              p_MeshInfo.boneInfluences.push_back(i_Weight);
            }
          }
        }
      }

      static void parse_submesh(const aiScene *p_AiScene,
                                const aiNode *p_AiNode, Mesh &p_Mesh,
                                Math::Matrix4x4 &p_Transformation,
                                Node &p_Node)
      {
        aiMatrix4x4 l_TransformationMatrix =
            p_AiNode->mTransformation;

        Submesh l_Submesh;
        l_Submesh.name = LOW_NAME(p_AiNode->mName.C_Str());

        l_Submesh.parentTransform = p_Transformation;
        l_Submesh.localTransform = Assimp2Glm(l_TransformationMatrix);

        l_Submesh.transform =
            p_Transformation * l_Submesh.localTransform;

        l_Submesh.meshInfos.resize(p_AiNode->mNumMeshes);

        for (uint32_t i = 0; i < l_Submesh.meshInfos.size(); ++i) {
          parse_mesh(p_AiScene->mMeshes[p_AiNode->mMeshes[i]],
                     l_Submesh.meshInfos[i], p_Mesh);
        }
        // printf("SIZE: %s = %d\n", l_Submesh.name.c_str(),
        // p_Mesh.bones.size());
        for (auto it = p_Mesh.bones.begin(); it != p_Mesh.bones.end();
             ++it) {
          // printf("B: %s\n", it->first.c_str());
        }

        if (p_Mesh.bones.find(l_Submesh.name) == p_Mesh.bones.end()) {
          Bone i_Bone;
          i_Bone.index = p_Mesh.boneCount;
          i_Bone.offset = l_Submesh.localTransform;
          p_Mesh.bones[l_Submesh.name] = i_Bone;

          p_Mesh.boneCount++;
        }

        p_Node.index = p_Mesh.submeshes.size();
        p_Mesh.submeshes.push_back(l_Submesh);

        p_Node.children.resize(p_AiNode->mNumChildren);

        for (uint32_t i = 0; i < p_AiNode->mNumChildren; ++i) {
          parse_submesh(p_AiScene, p_AiNode->mChildren[i], p_Mesh,
                        l_Submesh.transform, p_Node.children[i]);
        }
      }

      static void
      parse_animation_channel(const aiNodeAnim *p_NodeAnim,
                              AnimationChannel &p_Channel)
      {
        p_Channel.boneName = LOW_NAME(p_NodeAnim->mNodeName.C_Str());

        p_Channel.positions.resize(p_NodeAnim->mNumPositionKeys);
        p_Channel.rotations.resize(p_NodeAnim->mNumRotationKeys);
        p_Channel.scales.resize(p_NodeAnim->mNumScalingKeys);

        uint32_t l_LargestCount =
            LOW_MATH_MAX(p_NodeAnim->mNumPositionKeys,
                         LOW_MATH_MAX(p_NodeAnim->mNumRotationKeys,
                                      p_NodeAnim->mNumScalingKeys));

        for (uint32_t i = 0u; i < l_LargestCount; ++i) {
          if (i < p_NodeAnim->mNumPositionKeys) {
            p_Channel.positions[i].value.x =
                p_NodeAnim->mPositionKeys[i].mValue.x;
            p_Channel.positions[i].value.y =
                p_NodeAnim->mPositionKeys[i].mValue.y;
            p_Channel.positions[i].value.z =
                p_NodeAnim->mPositionKeys[i].mValue.z;

            p_Channel.positions[i].timestamp =
                p_NodeAnim->mPositionKeys[i].mTime;
          }
          if (i < p_NodeAnim->mNumRotationKeys) {
            p_Channel.rotations[i].value.x =
                p_NodeAnim->mRotationKeys[i].mValue.x;
            p_Channel.rotations[i].value.y =
                p_NodeAnim->mRotationKeys[i].mValue.y;
            p_Channel.rotations[i].value.z =
                p_NodeAnim->mRotationKeys[i].mValue.z;
            p_Channel.rotations[i].value.w =
                p_NodeAnim->mRotationKeys[i].mValue.w;

            p_Channel.rotations[i].timestamp =
                p_NodeAnim->mRotationKeys[i].mTime;
          }
          if (i < p_NodeAnim->mNumScalingKeys) {

            p_Channel.scales[i].value.x =
                p_NodeAnim->mScalingKeys[i].mValue.x;
            p_Channel.scales[i].value.y =
                p_NodeAnim->mScalingKeys[i].mValue.y;
            p_Channel.scales[i].value.z =
                p_NodeAnim->mScalingKeys[i].mValue.z;

            p_Channel.scales[i].timestamp =
                p_NodeAnim->mScalingKeys[i].mTime;
          }
        }
      }

      static void parse_animation(const aiAnimation *p_AiAnimation,
                                  Animation &p_Animation)
      {
        p_Animation.name = LOW_NAME(p_AiAnimation->mName.C_Str());
        p_Animation.duration = p_AiAnimation->mDuration;
        p_Animation.ticksPerSecond = p_AiAnimation->mTicksPerSecond;
        p_Animation.channels.resize(p_AiAnimation->mNumChannels);

        for (uint32_t i = 0u; i < p_AiAnimation->mNumChannels; ++i) {
          parse_animation_channel(p_AiAnimation->mChannels[i],
                                  p_Animation.channels[i]);
        }
      }

      static void parse_animations(const aiScene *p_AiScene,
                                   Mesh &p_Mesh)
      {
        p_Mesh.animations.resize(p_AiScene->mNumAnimations);

        for (uint32_t i = 0u; i < p_AiScene->mNumAnimations; ++i) {
          aiAnimation *i_Animation = p_AiScene->mAnimations[i];

          parse_animation(i_Animation, p_Mesh.animations[i]);
        }
      }

      void load_mesh(String p_FilePath, Mesh &p_Mesh)
      {

        p_Mesh.submeshes.clear();
        p_Mesh.bones.clear();
        p_Mesh.animations.clear();
        p_Mesh.boneCount = 0;

        LOW_LOG_DEBUG << "Loading: " << p_FilePath << LOW_LOG_END;

        Assimp::Importer l_Importer;

        const aiScene *l_AiScene = l_Importer.ReadFile(
            p_FilePath.c_str(), aiProcess_CalcTangentSpace);

        String l_ErrorMsg = "Could not load mesh scene from file '";
        l_ErrorMsg += p_FilePath;
        l_ErrorMsg += "'";

        LOW_ASSERT(l_AiScene, p_FilePath.c_str());

        const aiNode *l_RootNode = l_AiScene->mRootNode;

        p_Mesh.boneCount = 0;

        Math::Matrix4x4 l_Transformation = Math::Matrix4x4(1.0);

        parse_submesh(l_AiScene, l_RootNode, p_Mesh, l_Transformation,
                      p_Mesh.rootNode);

        if (l_AiScene->HasAnimations()) {
          parse_animations(l_AiScene, p_Mesh);
        }

        l_Importer.FreeScene();
      }
    } // namespace Resource
  }   // namespace Util
} // namespace Low
