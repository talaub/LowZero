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

      void load_image2d(String p_FilePath, Image2D &p_Image)
      {
        const uint32_t l_Channels = 4u;

        gli::texture2d l_Texture(gli::load(p_FilePath.c_str()));
        LOW_ASSERT(!l_Texture.empty(), "Could not load file");

        LOW_ASSERT(l_Texture.target() == gli::TARGET_2D,
                   "Expected Image2D data file");

        p_Image.dimensions.resize(l_Texture.levels());
        p_Image.data.resize(l_Texture.levels());

        for (uint32_t i = 0u; i < p_Image.dimensions.size(); ++i) {
          p_Image.dimensions[i].x = l_Texture.extent(i).x;
          p_Image.dimensions[i].y = l_Texture.extent(i).y;

          p_Image.data[i].resize(p_Image.dimensions[i].x *
                                 p_Image.dimensions[i].y * l_Channels);
          memcpy(p_Image.data[i].data(), l_Texture.data(0, 0, i),
                 p_Image.data[i].size());
        }
      }

      static void parse_mesh(const aiMesh *p_AiMesh, MeshInfo &p_MeshInfo)
      {

        LOW_ASSERT(p_AiMesh->HasPositions(),
                   "Mesh has no position information");
        LOW_ASSERT(p_AiMesh->HasNormals(), "Mesh has no normal information");
        LOW_ASSERT(p_AiMesh->HasTangentsAndBitangents(),
                   "Mesh has no tanged/bitangent information");

        p_MeshInfo.vertices.resize(p_AiMesh->mNumVertices);
        for (uint32_t i = 0u; i < p_AiMesh->mNumVertices; ++i) {
          p_MeshInfo.vertices[i].position = {p_AiMesh->mVertices[i].x,
                                             p_AiMesh->mVertices[i].y,
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
          p_MeshInfo.vertices[i].bitangent = {p_AiMesh->mBitangents[i].x,
                                              p_AiMesh->mBitangents[i].y,
                                              p_AiMesh->mBitangents[i].z};
        }

        LOW_ASSERT(p_AiMesh->HasFaces(), "Mesh has no index information");
        for (uint32_t i = 0u; i < p_AiMesh->mNumFaces; ++i) {
          for (uint32_t j = 0u; j < p_AiMesh->mFaces[i].mNumIndices; ++j) {
            p_MeshInfo.indices.push_back(p_AiMesh->mFaces[i].mIndices[j]);
          }
        }

        if (p_AiMesh->HasBones()) {
          p_MeshInfo.bones.resize(p_AiMesh->mNumBones);

          for (uint32_t i = 0u; i < p_AiMesh->mNumBones; ++i) {
            p_MeshInfo.bones[i].weights.resize(
                p_AiMesh->mBones[i]->mNumWeights);
            p_MeshInfo.bones[i].name =
                LOW_NAME(p_AiMesh->mBones[i]->mName.C_Str());

            for (uint32_t j = 0u; j < p_AiMesh->mBones[i]->mNumWeights; ++j) {
              p_MeshInfo.bones[i].weights[j].vertexIndex =
                  p_AiMesh->mBones[i]->mWeights[j].mVertexId;
              p_MeshInfo.bones[i].weights[j].weight =
                  p_AiMesh->mBones[i]->mWeights[j].mWeight;
            }
          }
        }
      }

      static void parse_submesh(const aiScene *p_AiScene,
                                const aiNode *p_AiNode, Mesh &p_Mesh,
                                Math::Matrix4x4 &p_Transformation)
      {
        aiMatrix4x4 l_TransformationMatrix = p_AiNode->mTransformation;

        Submesh l_Submesh;

        l_Submesh.transform = *(Math::Matrix4x4 *)&l_TransformationMatrix;

        l_Submesh.transform = p_Transformation * l_Submesh.transform;

        l_Submesh.meshInfos.resize(p_AiNode->mNumMeshes);

        for (uint32_t i = 0; i < l_Submesh.meshInfos.size(); ++i) {
          parse_mesh(p_AiScene->mMeshes[p_AiNode->mMeshes[i]],
                     l_Submesh.meshInfos[i]);
        }

        p_Mesh.submeshes.push_back(l_Submesh);

        for (uint32_t i = 0; i < p_AiNode->mNumChildren; ++i) {
          parse_submesh(p_AiScene, p_AiNode->mChildren[i], p_Mesh,
                        l_Submesh.transform);
        }
      }

      static void parse_animation_channel(const aiNodeAnim *p_NodeAnim,
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
            p_Channel.positions[i].value =
                *(Math::Vector3 *)&(p_NodeAnim->mPositionKeys[i].mValue);
            p_Channel.positions[i].timestamp =
                p_NodeAnim->mPositionKeys[i].mTime;
          }
          if (i < p_NodeAnim->mNumRotationKeys) {
            p_Channel.rotations[i].value =
                *(Math::Vector4 *)&(p_NodeAnim->mRotationKeys[i].mValue);
            p_Channel.rotations[i].timestamp =
                p_NodeAnim->mRotationKeys[i].mTime;
          }
          if (i < p_NodeAnim->mNumScalingKeys) {
            p_Channel.scales[i].value =
                *(Math::Vector3 *)&(p_NodeAnim->mScalingKeys[i].mValue);
            p_Channel.scales[i].timestamp = p_NodeAnim->mScalingKeys[i].mTime;
          }
        }
      }

      static void parse_animation(const aiAnimation *p_AiAnimation,
                                  Animation &p_Animation)
      {
        p_Animation.name = LOW_NAME(p_AiAnimation->mName.C_Str());
        p_Animation.duration = p_AiAnimation->mDuration;
        p_Animation.channels.resize(p_AiAnimation->mNumChannels);

        LOW_LOG_INFO << p_Animation.duration << LOW_LOG_END;

        for (uint32_t i = 0u; i < p_AiAnimation->mNumChannels; ++i) {
          parse_animation_channel(p_AiAnimation->mChannels[i],
                                  p_Animation.channels[i]);
        }
      }

      static void parse_animations(const aiScene *p_AiScene, Mesh &p_Mesh)
      {
        p_Mesh.animations.resize(p_AiScene->mNumAnimations);

        for (uint32_t i = 0u; i < p_AiScene->mNumAnimations; ++i) {
          aiAnimation *i_Animation = p_AiScene->mAnimations[i];

          parse_animation(i_Animation, p_Mesh.animations[i]);
        }
      }

      void load_mesh(String p_FilePath, Mesh &p_Mesh)
      {
        Assimp::Importer l_Importer;

        const aiScene *l_AiScene =
            l_Importer.ReadFile(p_FilePath.c_str(), aiProcess_CalcTangentSpace);

        LOW_ASSERT(l_AiScene, "Could not load mesh scene from file");

        const aiNode *l_RootNode = l_AiScene->mRootNode;

        Math::Matrix4x4 l_Transformation = Math::Matrix4x4(1.0);

        parse_submesh(l_AiScene, l_RootNode, p_Mesh, l_Transformation);

        if (l_AiScene->HasAnimations()) {
          parse_animations(l_AiScene, p_Mesh);
        }

        l_Importer.FreeScene();
      }
    } // namespace Resource
  }   // namespace Util
} // namespace Low
