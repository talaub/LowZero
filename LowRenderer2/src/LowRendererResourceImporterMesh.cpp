#include "LowRendererResourceImporter.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include "../../LowDependencies/assimp/code/Common/BaseProcess.h"

#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

#include "LowUtil.h"
#include "LowUtilString.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilYaml.h"
#include "LowUtilSerialization.h"
#include "LowUtilHashing.h"

#include "LowRendererRenderView.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderScene.h"
#include "LowRendererResourceManager.h"
#include "LowRendererRenderStep.h"
#include "LowRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../LowDependencies/stb/stb_image.h"

#define LOWR_IMP_ASSERT(cond, text)                                  \
  {                                                                  \
    if (!cond) {                                                     \
      LOW_LOG_ERROR << "[IMPORTER] " << text << LOW_LOG_END;         \
      return false;                                                  \
    }                                                                \
  }

#define LOWR_IMP_ASSERT_RETURN(cond, text)                           \
  {                                                                  \
    if (!cond) {                                                     \
      LOW_LOG_ERROR << "[IMPORTER] " << text << LOW_LOG_END;         \
      return false;                                                  \
    }                                                                \
  }

namespace Low {
  namespace Renderer {
    namespace ResourceImporter {
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

      namespace MeshImport {

        static bool
        calculate_bounding_sphere(const aiMesh *p_Mesh,
                                  Math::Sphere &p_BoundingSphere)
        {
          using namespace Low::Math;

          LOWR_IMP_ASSERT_RETURN(
              p_Mesh && p_Mesh->mNumVertices > 0,
              "Mesh must not be null and must have vertices");

          Vector3 l_Center = Vector3(0.0f);
          u32 l_NumVertices = p_Mesh->mNumVertices;

          // Compute centroid
          for (u32 i = 0; i < l_NumVertices; ++i) {
            const aiVector3D &l_V = p_Mesh->mVertices[i];
            l_Center += Vector3(l_V.x, l_V.y, l_V.z);
          }
          l_Center /= static_cast<float>(l_NumVertices);

          // Compute maximum squared distance from center
          float l_MaxDistanceSquared = 0.0f;
          for (u32 i = 0; i < l_NumVertices; ++i) {
            const aiVector3D &i_V = p_Mesh->mVertices[i];
            const Vector3 i_Position(i_V.x, i_V.y, i_V.z);
            const float i_DistanceSq =
                VectorUtil::distance_squared(l_Center, i_Position);

            if (i_DistanceSq > l_MaxDistanceSquared) {
              l_MaxDistanceSquared = i_DistanceSq;
            }
          }

          p_BoundingSphere.position = l_Center;
          p_BoundingSphere.radius = glm::sqrt(l_MaxDistanceSquared);

          return true;
        }

        static bool
        calculate_bounding_volumes(const aiMesh *p_Mesh,
                                   Math::Sphere &p_BoundingSphere,
                                   Math::AABB &p_AABB)
        {
          Math::Vector3 l_Min(LOW_FLOAT_MAX, LOW_FLOAT_MAX,
                              LOW_FLOAT_MAX);
          Math::Vector3 l_Max(LOW_FLOAT_MIN, LOW_FLOAT_MIN,
                              LOW_FLOAT_MIN);

          for (u32 i = 0; i < p_Mesh->mNumVertices; ++i) {
            aiVector3D i_Vertex = p_Mesh->mVertices[i];
            l_Min.x = LOW_MATH_MIN(l_Min.x, i_Vertex.x);
            l_Min.y = LOW_MATH_MIN(l_Min.y, i_Vertex.y);
            l_Min.z = LOW_MATH_MIN(l_Min.z, i_Vertex.z);

            l_Max.x = LOW_MATH_MAX(l_Max.x, i_Vertex.x);
            l_Max.y = LOW_MATH_MAX(l_Max.y, i_Vertex.y);
            l_Max.z = LOW_MATH_MAX(l_Max.z, i_Vertex.z);
          }

          p_AABB.bounds.min = l_Min;
          p_AABB.bounds.max = l_Max;

          p_AABB.center = (l_Min + l_Max) * 0.5f;

          LOWR_IMP_ASSERT_RETURN(
              calculate_bounding_sphere(p_Mesh, p_BoundingSphere),
              "Failed to calculate bounding sphere for mesh");

          return true;
        }

        bool populate_sidecar_info(const aiScene *p_Scene,
                                   Util::String p_Name,
                                   Util::Yaml::Node &p_Node)
        {
          using namespace Low::Math;
          using namespace Low::Util;

          p_Node["version"] = 1;
          p_Node["mesh_name"] = p_Name.c_str();
          p_Node["scene_name"] = p_Scene->mName.C_Str();

          Math::AABB l_AABB;
          l_AABB.bounds.min = Vector3(LOW_FLOAT_MAX);
          l_AABB.bounds.max = Vector3(LOW_FLOAT_MIN);

          Math::Sphere l_BoundingSphere;

          for (u32 i = 0; i < p_Scene->mNumMeshes; ++i) {
            aiMesh *i_Submesh = p_Scene->mMeshes[i];

            Yaml::Node i_Node;
            i_Node["name"] = i_Submesh->mName.C_Str();

            Math::AABB i_AABB;
            Math::Sphere i_BoundingSphere;
            LOWR_IMP_ASSERT_RETURN(
                calculate_bounding_volumes(i_Submesh,
                                           i_BoundingSphere, i_AABB),
                "Failed to calculate mesh bounding volumes.");

            // TODO: REMOVE!!! This is a hack for testing
            l_BoundingSphere = i_BoundingSphere;

            l_AABB.bounds.min =
                glm::min(l_AABB.bounds.min, i_AABB.bounds.min);
            l_AABB.bounds.max =
                glm::max(l_AABB.bounds.max, i_AABB.bounds.max);

            Serialization::serialize(i_Node["aabb"], i_AABB);
            Serialization::serialize(i_Node["bounding_sphere"],
                                     i_BoundingSphere);

            p_Node["submeshes"][i_Submesh->mName.C_Str()] = i_Node;
          }

          l_AABB.center =
              (l_AABB.bounds.min + l_AABB.bounds.max) * 0.5f;

          Serialization::serialize(p_Node["aabb"], l_AABB);
          // TODO: Calculate bounding sphere for whole mesh
          Serialization::serialize(p_Node["bounding_sphere"],
                                   l_BoundingSphere);
          return true;
        }

        static bool is_reimport()
        {
          // TODO: IMPLEMENT
          return false;
        }
      } // namespace MeshImport

      static void create_thumbnail_picture(Mesh p_Mesh)
      {
        Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

        l_LocalMatrix =
            glm::translate(l_LocalMatrix, Math::Vector3(0.0f));
        l_LocalMatrix *=
            glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        l_LocalMatrix =
            glm::scale(l_LocalMatrix, Math::Vector3(1.0f));

        RenderScene l_RenderScene = RenderScene::make(N(Thumbnail));
        RenderView l_RenderView = RenderView::make(N(Thumbnail));
        l_RenderView.set_render_scene(l_RenderScene);
        l_RenderView.set_dimensions(Math::UVector2(500, 500));
        l_RenderView.add_step_by_name(RENDERSTEP_SOLID_MATERIAL_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_SSAO_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_LIGHTCULLING_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_LIGHTING_NAME);

        RenderObject l_RenderObject =
            RenderObject::make(l_RenderScene, p_Mesh);
        l_RenderObject.set_material(get_default_material());
        l_RenderObject.set_world_transform(l_LocalMatrix);

        l_RenderScene.set_directional_light_color(1.0f, 1.0f, 1.0f);
        l_RenderScene.set_directional_light_intensity(0.75f);
        l_RenderScene.set_directional_light_direction(-0.15f, -1.0f,
                                                      -1.5f);

        ThumbnailCreationSchedule l_Schedule;
        l_Schedule.mesh = p_Mesh;
        l_Schedule.material = Util::Handle::DEAD;
        l_Schedule.scene = l_RenderScene;
        l_Schedule.view = l_RenderView;
        l_Schedule.object = l_RenderObject;

        l_Schedule.path = Util::get_project().editorImagesPath +
                          "\\mesh_" +
                          Util::hash_to_string(
                              p_Mesh.get_resource().get_mesh_id()) +
                          ".png";

        l_Schedule.viewDirection = Math::Vector3(0.2f, -0.1f, -1.0f);

        submit_thumbnail_creation(l_Schedule);
      }

      bool import_mesh(Util::String p_ImportPath,
                       Util::String p_OutputPath)
      {
        Assimp::Importer l_Importer;

        using namespace MeshImport;

        Util::FileIO::File l_File = Util::FileIO::open(
            p_ImportPath.c_str(), Util::FileIO::FileMode::READ_BYTES);
        uint32_t l_FileSize = Util::FileIO::size_sync(l_File);
        Util::List<char> l_FileContent;
        l_FileContent.resize(l_FileSize + 1);
        Util::FileIO::read_sync(l_File, l_FileContent.data());

        Util::String l_Content = l_FileContent.data();
        Util::List<Util::String> l_Lines;
        Util::StringHelper::split(l_Content, '\n', l_Lines);

        const bool l_Reimport = is_reimport();

        const u64 l_AssetHash = Util::fnv1a_64(l_Content.c_str());
        // TODO: Fix reimport
        const u64 l_MeshId = l_Reimport ? 0 : l_AssetHash;

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
        l_ErrorMessage = p_ImportPath;
        l_ErrorMessage += "'";

        LOWR_IMP_ASSERT_RETURN(l_AiScene, l_ErrorMessage.c_str());

        bool l_OriginalSceneHasAnimation = l_AiScene->HasAnimations();

        Util::Yaml::Node l_SidecarInfo;
        populate_sidecar_info(l_AiScene, "Test", l_SidecarInfo);

        AdjustTextureCoordinatesPostProcessing *l_TexPP =
            new AdjustTextureCoordinatesPostProcessing();
        l_AiScene =
            l_Importer.ApplyCustomizedPostProcessing(l_TexPP, false);
        delete l_TexPP;

        const Util::String l_BaseAssetPath =
            Util::get_project().assetCachePath + "\\" +
            Util::hash_to_string(l_MeshId);

        const Util::String l_GlbPath = l_BaseAssetPath + ".glb";
        const Util::String l_SidecarPath =
            l_BaseAssetPath + ".mesh.yaml";

        const Util::String l_ResourcePath =
            Util::get_project().dataPath + "\\" + p_OutputPath +
            ".meshresource.yaml";

        Util::String l_FileName =
            p_OutputPath.substr(p_OutputPath.find_last_of("/\\") + 1);

        Util::Yaml::Node l_ResourceNode;
        {
          l_ResourceNode["version"] = 1;
          l_ResourceNode["mesh_id"] =
              Util::hash_to_string(l_MeshId).c_str();
          l_ResourceNode["asset_hash"] =
              Util::hash_to_string(l_AssetHash).c_str();
          l_ResourceNode["source_file"] = p_ImportPath.c_str();
          l_ResourceNode["name"] = l_FileName.c_str();
          l_ResourceNode["submesh_count"] =
              l_SidecarInfo["submeshes"].size();
        }

        Assimp::Exporter l_Exporter;
        l_Exporter.Export(
            l_AiScene, "glb2", l_GlbPath.c_str(),
            aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                aiProcess_GenNormals | aiProcess_GenUVCoords |
                aiProcess_CalcTangentSpace | // Tangents and
                                             // bitangents
                aiProcess_ImproveCacheLocality |
                aiProcess_RemoveRedundantMaterials |
                aiProcess_FindInvalidData | aiProcess_OptimizeMeshes |
                aiProcess_SortByPType);

        l_Importer.FreeScene();

        Util::Yaml::write_file(l_SidecarPath.c_str(), l_SidecarInfo);
        Util::Yaml::write_file(l_ResourcePath.c_str(),
                               l_ResourceNode);

        // TODO: FIX REIMPORT
        if (!l_Reimport) {
          MeshResourceConfig l_Config;

          LOW_ASSERT_ERROR_RETURN_FALSE(
              ResourceManager::parse_mesh_resource_config(
                  l_ResourcePath, l_ResourceNode, l_Config),
              "Failed to parse resource config on mesh import");

          Mesh l_Mesh = Mesh::make_from_resource_config(l_Config);

          create_thumbnail_picture(l_Mesh);
        }

        return true;
      }
    } // namespace ResourceImporter
  } // namespace Renderer
} // namespace Low
