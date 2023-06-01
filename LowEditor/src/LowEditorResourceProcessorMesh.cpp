#include "LowEditorResourceProcessorMesh.h"

#include "LowUtilLogger.h"
#include <iostream>

#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include "../../LowDependencies/assimp/code/Common/BaseProcess.h"

#include "LowUtilAssert.h"

namespace Low {
  namespace Editor {
    namespace ResourceProcessor {

      namespace Mesh {
        struct RemoveAnimationPostProcessing : public Assimp::BaseProcess
        {
          bool IsActive(unsigned int p_Flags) const override
          {
            return true;
          }

          void Execute(aiScene *p_Scene) override
          {
            for (uint32_t i = 0u; i < p_Scene->mNumAnimations; ++i) {
              delete p_Scene->mAnimations[i];
            }
            p_Scene->mAnimations = nullptr;
            p_Scene->mNumAnimations = 0;
          }
        };

        bool process(Util::String p_FilePath, Util::String p_OutputPath)
        {
          Assimp::Importer l_Importer;

          const aiScene *l_AiScene = l_Importer.ReadFile(p_FilePath.c_str(), 0);
          LOW_ASSERT(l_AiScene, "Could not load mesh scene from file");

          bool l_OriginalSceneHasAnimation = l_AiScene->HasAnimations();

          RemoveAnimationPostProcessing *l_PP =
              new RemoveAnimationPostProcessing();
          l_AiScene = l_Importer.ApplyCustomizedPostProcessing(l_PP, false);
          delete l_PP;

          Assimp::Exporter l_Exporter;
          l_Exporter.Export(l_AiScene, "glb2", p_OutputPath.c_str(),
                            aiProcess_Triangulate | aiProcess_GenNormals |
                                aiProcess_FixInfacingNormals |
                                aiProcess_JoinIdenticalVertices |
                                aiProcess_ConvertToLeftHanded);

          l_Importer.FreeScene();

          return l_OriginalSceneHasAnimation;
        }

        void process_animations(Util::String p_FilePath,
                                Util::String p_OutputPath)
        {
          Assimp::Importer l_Importer;
          const aiScene *l_AiScene = l_Importer.ReadFile(p_FilePath.c_str(), 0);
          LOW_ASSERT(l_AiScene, "Could not load mesh scene from file");
          LOW_ASSERT(l_AiScene->HasAnimations(),
                     "Meshfile does not contain any animation information");

          Util::Name l_AnimationName = N(04_Idle);

          float l_AnimTime = 300.0f;

          aiAnimation *l_Animation = nullptr;

          for (uint32_t i = 0u; i < l_AiScene->mNumAnimations; ++i) {
            aiAnimation *i_Animation = l_AiScene->mAnimations[i];

            Util::Name i_AnimationName = LOW_NAME(i_Animation->mName.C_Str());

            LOW_LOG_INFO << i_AnimationName << ": "
                         << (float)i_Animation->mDuration << LOW_LOG_END;

            if (i_AnimationName == l_AnimationName) {
              l_Animation = i_Animation;
              break;
            }
          }

          LOW_ASSERT(l_Animation, "Could not find animation");

          aiNode *l_Node = l_AiScene->mRootNode->FindNode("Oberschenkel_L_041");

          LOW_ASSERT(l_Node, "Could not find node");

          aiNodeAnim *l_Channel = nullptr;

          for (uint32_t i = 0u; i < l_Animation->mNumChannels; ++i) {
            Util::Name i_NodeName =
                LOW_NAME(l_Animation->mChannels[i]->mNodeName.C_Str());
            if (i_NodeName == LOW_NAME(l_Node->mName.C_Str())) {
              l_Channel = l_Animation->mChannels[i];
              break;
            }
          }

          LOW_ASSERT(l_Channel, "Could not find channel");

          Math::Vector3 l_Position(0.0f);
          Math::Quaternion l_Rotation(1.0f, 0.0f, 0.0f, 0.0f);
          Math::Vector3 l_Scale(1.0f);

          int l_PositionIndex = -1;
          for (uint32_t i = 0u; i < l_Channel->mNumPositionKeys - 1; ++i) {
            if (l_AnimTime < l_Channel->mPositionKeys[i + 1].mTime) {
              l_PositionIndex = i;
            }
          }
          LOW_ASSERT(l_PositionIndex >= 0, "Position issue");

          l_Importer.FreeScene();
        }
      } // namespace Mesh
    }   // namespace ResourceProcessor
  }     // namespace Editor
} // namespace Low
