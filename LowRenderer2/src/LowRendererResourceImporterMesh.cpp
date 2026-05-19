#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

#include "LowRendererMeshResource.h"
#include "LowRendererMeshState.h"
#include "LowRendererResourceImporter.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include "../../LowDependencies/assimp/code/Common/BaseProcess.h"

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
        static MeshResource
        get_reimport(const u64 p_AssetHash,
                     const Util::String p_SourcePath)
        {
          for (u32 i = 0; i < MeshResource::living_count(); ++i) {
            MeshResource i_Resource =
                MeshResource::living_instances()[i];
            if (i_Resource.get_source_file() != p_SourcePath) {
              continue;
            }
            return i_Resource;
          }

          return Util::Handle::DEAD;
        }

        static void initialize_aabb(Math::AABB &p_AABB)
        {
          p_AABB.bounds.min =
              Math::Vector3(LOW_FLOAT_MAX, LOW_FLOAT_MAX,
                            LOW_FLOAT_MAX);
          p_AABB.bounds.max =
              Math::Vector3(-LOW_FLOAT_MAX, -LOW_FLOAT_MAX,
                            -LOW_FLOAT_MAX);
          p_AABB.center = Math::Vector3(0.0f);
        }

        static void include_vertex_in_aabb(Math::AABB &p_AABB,
                                           const aiVector3D &p_Vertex)
        {
          p_AABB.bounds.min.x =
              LOW_MATH_MIN(p_AABB.bounds.min.x, p_Vertex.x);
          p_AABB.bounds.min.y =
              LOW_MATH_MIN(p_AABB.bounds.min.y, p_Vertex.y);
          p_AABB.bounds.min.z =
              LOW_MATH_MIN(p_AABB.bounds.min.z, p_Vertex.z);

          p_AABB.bounds.max.x =
              LOW_MATH_MAX(p_AABB.bounds.max.x, p_Vertex.x);
          p_AABB.bounds.max.y =
              LOW_MATH_MAX(p_AABB.bounds.max.y, p_Vertex.y);
          p_AABB.bounds.max.z =
              LOW_MATH_MAX(p_AABB.bounds.max.z, p_Vertex.z);
        }

        static void finalize_aabb(Math::AABB &p_AABB)
        {
          p_AABB.center =
              (p_AABB.bounds.min + p_AABB.bounds.max) * 0.5f;
        }

        static bool
        calculate_bounding_sphere(const aiMesh *p_Mesh,
                                  const Math::Vector3 &p_Center,
                                  Math::Sphere &p_BoundingSphere)
        {
          LOWR_IMP_ASSERT_RETURN(
              p_Mesh && p_Mesh->mNumVertices > 0,
              "Mesh must not be null and must have vertices");

          // Compute maximum squared distance from center
          float l_MaxDistanceSquared = 0.0f;
          for (u32 i = 0; i < p_Mesh->mNumVertices; ++i) {
            const aiVector3D &i_V = p_Mesh->mVertices[i];
            const Math::Vector3 i_Position(i_V.x, i_V.y, i_V.z);
            const float i_DistanceSq =
                Math::VectorUtil::distance_squared(p_Center,
                                                   i_Position);

            if (i_DistanceSq > l_MaxDistanceSquared) {
              l_MaxDistanceSquared = i_DistanceSq;
            }
          }

          p_BoundingSphere.position = p_Center;
          p_BoundingSphere.radius = glm::sqrt(l_MaxDistanceSquared);

          return true;
        }

        static bool
        calculate_bounding_volumes(const aiMesh *p_Mesh,
                                   Math::Sphere &p_BoundingSphere,
                                   Math::AABB &p_AABB)
        {
          LOWR_IMP_ASSERT_RETURN(
              p_Mesh && p_Mesh->mNumVertices > 0,
              "Mesh must not be null and must have vertices");

          initialize_aabb(p_AABB);

          for (u32 i = 0; i < p_Mesh->mNumVertices; ++i) {
            include_vertex_in_aabb(p_AABB, p_Mesh->mVertices[i]);
          }

          finalize_aabb(p_AABB);

          LOWR_IMP_ASSERT_RETURN(
              calculate_bounding_sphere(p_Mesh, p_AABB.center,
                                        p_BoundingSphere),
              "Failed to calculate bounding sphere for mesh");

          return true;
        }

        static bool
        calculate_bounding_sphere(const aiScene *p_Scene,
                                  const Math::Vector3 &p_Center,
                                  Math::Sphere &p_BoundingSphere)
        {
          LOWR_IMP_ASSERT_RETURN(
              p_Scene && p_Scene->mNumMeshes > 0,
              "Scene must not be null and must have meshes");

          float l_MaxDistanceSquared = 0.0f;
          for (u32 i = 0; i < p_Scene->mNumMeshes; ++i) {
            aiMesh *i_Mesh = p_Scene->mMeshes[i];
            for (u32 j = 0; j < i_Mesh->mNumVertices; ++j) {
              const aiVector3D &j_Vertex = i_Mesh->mVertices[j];
              const Math::Vector3 j_Position(
                  j_Vertex.x, j_Vertex.y, j_Vertex.z);
              const float j_DistanceSq =
                  Math::VectorUtil::distance_squared(p_Center,
                                                     j_Position);

              if (j_DistanceSq > l_MaxDistanceSquared) {
                l_MaxDistanceSquared = j_DistanceSq;
              }
            }
          }

          p_BoundingSphere.position = p_Center;
          p_BoundingSphere.radius = glm::sqrt(l_MaxDistanceSquared);

          return true;
        }

        static bool
        calculate_bounding_volumes(const aiScene *p_Scene,
                                   Math::Sphere &p_BoundingSphere,
                                   Math::AABB &p_AABB)
        {
          LOWR_IMP_ASSERT_RETURN(
              p_Scene && p_Scene->mNumMeshes > 0,
              "Scene must not be null and must have meshes");

          initialize_aabb(p_AABB);

          for (u32 i = 0; i < p_Scene->mNumMeshes; ++i) {
            aiMesh *i_Mesh = p_Scene->mMeshes[i];
            for (u32 j = 0; j < i_Mesh->mNumVertices; ++j) {
              include_vertex_in_aabb(p_AABB, i_Mesh->mVertices[j]);
            }
          }

          finalize_aabb(p_AABB);

          LOWR_IMP_ASSERT_RETURN(
              calculate_bounding_sphere(p_Scene, p_AABB.center,
                                        p_BoundingSphere),
              "Failed to calculate bounding sphere for scene");

          return true;
        }

        bool populate_sidecar_info(const aiScene *p_Scene,
                                   Util::String p_Name,
                                   Util::Serial::Node &p_Node)
        {
          using namespace Low::Math;
          using namespace Low::Util;

          p_Node["version"] = 1;
          p_Node["mesh_name"] = p_Name;
          p_Node["scene_name"] = p_Scene->mName.C_Str();

          Math::AABB l_AABB;
          Math::Sphere l_BoundingSphere;

          LOWR_IMP_ASSERT_RETURN(
              calculate_bounding_volumes(p_Scene, l_BoundingSphere,
                                         l_AABB),
              "Failed to calculate scene bounding volumes.");

          for (u32 i = 0; i < p_Scene->mNumMeshes; ++i) {
            aiMesh *i_Submesh = p_Scene->mMeshes[i];

            Serial::Node i_Node;
            i_Node["name"] = i_Submesh->mName.C_Str();

            Math::AABB i_AABB;
            Math::Sphere i_BoundingSphere;
            LOWR_IMP_ASSERT_RETURN(
                calculate_bounding_volumes(i_Submesh,
                                           i_BoundingSphere, i_AABB),
                "Failed to calculate mesh bounding volumes.");

            i_Node["aabb"] = i_AABB;
            i_Node["bounding_sphere"] = i_BoundingSphere;

            p_Node["submeshes"][i_Submesh->mName.C_Str()] = i_Node;
          }

          p_Node["aabb"] = l_AABB;
          p_Node["bounding_sphere"] = l_BoundingSphere;
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
        l_RenderView.add_step_by_name(RENDERSTEP_SHADOW_PASS_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_SOLID_MATERIAL_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_SSAO_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_CAVITIES_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_LIGHTCULLING_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_LIGHTING_NAME);
        l_RenderView.add_step_by_name(RENDERSTEP_TONEMAPPING_NAME);

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
                          "\\thumbnails\\mesh_" +
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

        const u64 l_AssetHash = Util::fnv1a_64(l_Content.c_str());

        const MeshResource l_OriginalResource =
            get_reimport(l_AssetHash, p_ImportPath);

        const bool l_Reimport = l_OriginalResource.is_alive();

        if (l_Reimport) {
          if (l_OriginalResource.get_asset_hash() == l_AssetHash) {
            LOW_LOG_DEBUG << "Exiting reimport early because texture "
                             "didn't change."
                          << LOW_LOG_END;
            return "";
          }
        }

        Util::List<Util::String> l_Lines;
        Util::StringHelper::split(l_Content, '\n', l_Lines);

        const u64 l_MeshId = l_Reimport
                                 ? l_OriginalResource.get_mesh_id()
                                 : l_AssetHash;

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

        Util::Serial::Node l_SidecarInfo;
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
            p_OutputPath + ".meshresource.yaml";

        Util::String l_FileName =
            p_OutputPath.substr(p_OutputPath.find_last_of("/\\") + 1);

        Util::Serial::Node l_ResourceNode;
        {
          l_ResourceNode["version"] = 1;
          l_ResourceNode["mesh_id"] = Util::U64Id{l_MeshId};
          l_ResourceNode["asset_hash"] = Util::U64Id{l_AssetHash};
          l_ResourceNode["source_file"] = p_ImportPath;
          l_ResourceNode["name"] = l_FileName;
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

        Util::Serial::write_yaml_file(l_SidecarPath.c_str(),
                                      l_SidecarInfo);
        Util::Serial::write_yaml_file(l_ResourcePath.c_str(),
                                      l_ResourceNode);

        MeshResourceConfig l_Config;

        Mesh l_Mesh = ResourceManager::find_asset<Mesh>(l_MeshId);
        bool l_WasLoaded = false;
        if (!l_Reimport) {
          LOW_ASSERT_ERROR_RETURN_FALSE(
              ResourceManager::parse_mesh_resource_config(
                  l_ResourcePath, l_ResourceNode, l_Config),
              "Failed to parse resource config on mesh import");
          l_Mesh = Mesh::make_from_resource_config(l_Config);
        } else {
          // TODO: Check what happens if it's LOADING or something
          // similar
          l_WasLoaded = l_Mesh.get_state() == MeshState::LOADED;
        }

        l_Mesh.get_resource().set_asset_hash(l_AssetHash);

        if (l_WasLoaded) {
          ResourceManager::reload_mesh(l_Mesh);
        }

        create_thumbnail_picture(l_Mesh);

        return true;
      }
    } // namespace ResourceImporter
  } // namespace Renderer
} // namespace Low
