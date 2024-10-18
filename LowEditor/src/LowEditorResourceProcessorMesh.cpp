#include "LowEditorResourceProcessorMesh.h"

#include "LowUtilLogger.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"
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
        struct RemoveAnimationPostProcessing
            : public Assimp::BaseProcess
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

        struct AdjustTextureCoordinatesPostProcessing
            : public Assimp::BaseProcess
        {
          bool IsActive(unsigned int p_Flags) const override
          {
            return true;
          }

          void Execute(aiScene *p_Scene) override
          {
            for (uint32_t i = 0u; i < p_Scene->mNumMeshes; ++i) {
              aiMesh *i_Mesh = p_Scene->mMeshes[i];
              for (uint32_t j = 0u; j < i_Mesh->mNumVertices; ++j) {
                i_Mesh->mTextureCoords[0][j].y =
                    1.0f - i_Mesh->mTextureCoords[0][j].y;
              }
            }
          }
        };

        bool process(Util::String p_FilePath,
                     Util::String p_OutputPath)
        {
          Assimp::Importer l_Importer;

          Util::FileIO::File l_File = Util::FileIO::open(
              p_FilePath.c_str(), Util::FileIO::FileMode::READ_BYTES);
          uint32_t l_FileSize = Util::FileIO::size_sync(l_File);
          Util::List<char> l_FileContent;
          l_FileContent.resize(l_FileSize + 1);
          Util::FileIO::read_sync(l_File, l_FileContent.data());

          Util::String l_Content = l_FileContent.data();
          Util::List<Util::String> l_Lines;
          Util::StringHelper::split(l_Content, '\n', l_Lines);

          l_Content = "";
          for (uint32_t i = 0; i < l_Lines.size(); ++i) {
            if (!Util::StringHelper::begins_with(l_Lines[i], "l")) {
              l_Content += l_Lines[i] + "\n";
            }
          }

          const aiScene *l_AiScene = l_Importer.ReadFileFromMemory(
              l_Content.c_str(), l_Content.size(), 0);

          Util::String l_ErrorMessage =
              "Could not load mesh scene from file '";
          l_ErrorMessage = p_FilePath;
          l_ErrorMessage += "'";

          LOW_ASSERT(l_AiScene,
                     l_ErrorMessage.c_str());

          bool l_OriginalSceneHasAnimation =
              l_AiScene->HasAnimations();

          RemoveAnimationPostProcessing *l_PP =
              new RemoveAnimationPostProcessing();
          AdjustTextureCoordinatesPostProcessing *l_TexPP =
              new AdjustTextureCoordinatesPostProcessing();
          l_AiScene =
              l_Importer.ApplyCustomizedPostProcessing(l_PP, false);
          l_AiScene = l_Importer.ApplyCustomizedPostProcessing(
              l_TexPP, false);
          delete l_PP;
          delete l_TexPP;

          Assimp::Exporter l_Exporter;
          l_Exporter.Export(l_AiScene, "glb2", p_OutputPath.c_str(),
                            aiProcess_Triangulate |
                                aiProcess_GenNormals |
                                aiProcess_FixInfacingNormals |
                                aiProcess_JoinIdenticalVertices);

          l_Importer.FreeScene();

          return l_OriginalSceneHasAnimation;
        }

        void process_animations(Util::String p_FilePath,
                                Util::String p_OutputPath)
        {
          Assimp::Importer l_Importer;

          const aiScene *l_AiScene =
              l_Importer.ReadFile(p_FilePath.c_str(), 0);

          Util::String l_ErrorMessage =
              "Could not load anim mesh scene from file '";
          l_ErrorMessage = p_FilePath;
          l_ErrorMessage += "'";

          LOW_ASSERT(l_AiScene,
                     l_ErrorMessage.c_str());
          LOW_ASSERT(
              l_AiScene->HasAnimations(),
              "Meshfile does not contain any animation information");

          Util::Name l_AnimationName = N(04_Idle);

          float l_AnimTime = 300.0f;

          aiAnimation *l_Animation = nullptr;

          for (uint32_t i = 0u; i < l_AiScene->mNumAnimations; ++i) {
            aiAnimation *i_Animation = l_AiScene->mAnimations[i];

            Util::Name i_AnimationName =
                LOW_NAME(i_Animation->mName.C_Str());

            if (i_AnimationName == l_AnimationName) {
              l_Animation = i_Animation;
              break;
            }
          }

          LOW_ASSERT(l_Animation, "Could not find animation");

          aiNode *l_Node =
              l_AiScene->mRootNode->FindNode("Oberschenkel_L_041");

          LOW_ASSERT(l_Node, "Could not find node");

          aiNodeAnim *l_Channel = nullptr;

          for (uint32_t i = 0u; i < l_Animation->mNumChannels; ++i) {
            Util::Name i_NodeName = LOW_NAME(
                l_Animation->mChannels[i]->mNodeName.C_Str());
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
          for (uint32_t i = 0u; i < l_Channel->mNumPositionKeys - 1;
               ++i) {
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
