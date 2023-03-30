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

      void load_mesh(String p_FilePath, Mesh &p_Mesh)
      {
        // TODO: This is a test implementation that only reads the toplevel mesh
        // from a file

        Assimp::Importer l_Importer;
        const aiScene *l_AiScene = l_Importer.ReadFile(
            p_FilePath.c_str(), aiProcess_CalcTangentSpace |
                                    aiProcess_FixInfacingNormals |
                                    aiProcess_Triangulate | aiProcess_FlipUVs);

        LOW_ASSERT(l_AiScene, "Could not load mesh scene from file");

        const aiMesh *l_AiMesh = l_AiScene->mMeshes[0];

        LOW_ASSERT(l_AiMesh->HasPositions(),
                   "Mesh has no position information");

        p_Mesh.vertices.resize(l_AiMesh->mNumVertices);
        for (uint32_t i = 0u; i < l_AiMesh->mNumVertices; ++i) {
          p_Mesh.vertices[i].position = {l_AiMesh->mVertices[i].x,
                                         l_AiMesh->mVertices[i].y,
                                         l_AiMesh->mVertices[i].z};
        }

        for (uint32_t i = 0u; i < l_AiMesh->mNumVertices; ++i) {
          p_Mesh.vertices[i].texture_coordinates = {
              l_AiMesh->mTextureCoords[0][i].x,
              l_AiMesh->mTextureCoords[0][i].y};
        }

        LOW_ASSERT(l_AiMesh->HasFaces(), "Mesh has no index information");
        for (uint32_t i = 0u; i < l_AiMesh->mNumFaces; ++i) {
          for (uint32_t j = 0u; j < l_AiMesh->mFaces[i].mNumIndices; ++j) {
            p_Mesh.indices.push_back(l_AiMesh->mFaces[i].mIndices[j]);
          }
        }
      }

    } // namespace Resource
  }   // namespace Util
} // namespace Low
