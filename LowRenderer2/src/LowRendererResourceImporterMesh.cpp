#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

#include "LowRendererMeshResource.h"
#include "LowRendererMeshState.h"
#include "LowRendererMeshType.h"
#include "LowRendererResourceImporter.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include "../../LowDependencies/assimp/code/Common/BaseProcess.h"

#include <cmath>
#include <cstdio>
#include <cctype>

#include "LowUtil.h"
#include "LowUtilHandle.h"
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
            if (!i_Mesh->HasTextureCoords(0)) {
              continue;
            }

            for (uint32_t j = 0u; j < i_Mesh->mNumVertices; ++j) {
              i_Mesh->mTextureCoords[0][j].y =
                  1.0f - i_Mesh->mTextureCoords[0][j].y;
            }
          }
        }
      };

      namespace MeshImport {
        struct MeshSourceData
        {
          Util::List<char> bytes;
          Util::String extension;
          Util::String importHint;
          u64 assetHash;
        };

        struct SkeletonImportBone
        {
          Util::Name name;
          u32 sourceBoneIndex;
          i32 parentIndex;
          Math::Matrix4x4 localBindTransform;
          Math::Matrix4x4 globalBindTransform;
          Math::Matrix4x4 inverseBindMatrix;
          bool deforming;
        };

        struct SkeletonImportData
        {
          Util::List<SkeletonImportBone> bones;
          Util::Map<Util::Name, u32> boneNameToIndex;
          u64 exactHash;
        };

        static Math::Matrix4x4
        assimp_matrix_to_glm(const aiMatrix4x4 &p_Matrix)
        {
          return Math::Matrix4x4(
              (double)p_Matrix.a1, (double)p_Matrix.b1,
              (double)p_Matrix.c1, (double)p_Matrix.d1,
              (double)p_Matrix.a2, (double)p_Matrix.b2,
              (double)p_Matrix.c2, (double)p_Matrix.d2,
              (double)p_Matrix.a3, (double)p_Matrix.b3,
              (double)p_Matrix.c3, (double)p_Matrix.d3,
              (double)p_Matrix.a4, (double)p_Matrix.b4,
              (double)p_Matrix.c4, (double)p_Matrix.d4);
        }

        static Util::String
        get_lowercase_extension(const Util::String &p_Path)
        {
          Util::String l_Extension =
              Util::PathHelper::get_file_extension(p_Path);

          for (u32 i = 0; i < l_Extension.size(); ++i) {
            l_Extension[i] = static_cast<char>(std::tolower(
                static_cast<unsigned char>(l_Extension[i])));
          }

          return l_Extension;
        }

        static bool is_obj_source(const MeshSourceData &p_SourceData)
        {
          return p_SourceData.extension == "obj";
        }

        static bool
        load_mesh_source_data(const Util::String &p_ImportPath,
                              MeshSourceData &p_SourceData)
        {
          p_SourceData.extension =
              get_lowercase_extension(p_ImportPath);
          p_SourceData.importHint = p_SourceData.extension;

          LOWR_IMP_ASSERT_RETURN(
              Util::FileIO::file_exists_sync(p_ImportPath.c_str()),
              "Could not find mesh source file.");

          Util::FileIO::File l_File =
              Util::FileIO::open(p_ImportPath.c_str(),
                                 Util::FileIO::FileMode::READ_BYTES);

          const u32 l_FileSize = Util::FileIO::size_sync(l_File);
          p_SourceData.bytes.resize(l_FileSize + 1);

          LOWR_IMP_ASSERT_RETURN(
              Util::FileIO::read_sync(l_File,
                                      p_SourceData.bytes.data()) == 0,
              "Could not read mesh source file.");

          p_SourceData.assetHash =
              Util::fnv1a_64(p_SourceData.bytes.data(), l_FileSize);

          Util::FileIO::close(l_File);

          return true;
        }

        static Util::String
        preprocess_obj_source(const MeshSourceData &p_SourceData)
        {
          Util::String l_Content(p_SourceData.bytes.data());
          Util::List<Util::String> l_Lines;
          Util::StringHelper::split(l_Content, '\n', l_Lines);

          l_Content = "";
          for (u32 i = 0; i < l_Lines.size(); ++i) {
            if (!Util::StringHelper::begins_with(l_Lines[i], "l")) {
              l_Content += l_Lines[i] + "\n";
            }
          }

          return l_Content;
        }

        static const aiScene *
        load_assimp_scene(Assimp::Importer &p_Importer,
                          const Util::String &p_ImportPath,
                          const MeshSourceData &p_SourceData)
        {
          if (is_obj_source(p_SourceData)) {
            const Util::String l_Content =
                preprocess_obj_source(p_SourceData);

            return p_Importer.ReadFileFromMemory(
                l_Content.c_str(), l_Content.size(), 0,
                p_SourceData.importHint.c_str());
          }

          return p_Importer.ReadFile(p_ImportPath.c_str(), 0);
        }

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
          p_AABB.bounds.min = Math::Vector3(
              LOW_FLOAT_MAX, LOW_FLOAT_MAX, LOW_FLOAT_MAX);
          p_AABB.bounds.max = Math::Vector3(
              -LOW_FLOAT_MAX, -LOW_FLOAT_MAX, -LOW_FLOAT_MAX);
          p_AABB.center = Math::Vector3(0.0f);
        }

        static void
        include_vertex_in_aabb(Math::AABB &p_AABB,
                               const Math::Vector3 &p_Vertex)
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

        static void include_vertex_in_aabb(Math::AABB &p_AABB,
                                           const aiVector3D &p_Vertex)
        {
          include_vertex_in_aabb(
              p_AABB,
              Math::Vector3(p_Vertex.x, p_Vertex.y, p_Vertex.z));
        }

        static Math::Vector3
        transform_vertex(const aiVector3D &p_Vertex,
                         const Math::Matrix4x4 &p_Transform)
        {
          const Math::Vector4 l_Position =
              p_Transform *
              Math::Vector4(p_Vertex.x, p_Vertex.y, p_Vertex.z, 1.0f);
          return Math::Vector3(l_Position.x, l_Position.y,
                               l_Position.z);
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

        static void include_node_vertices_in_aabb(
            const aiScene *p_Scene, const aiNode *p_Node,
            const Math::Matrix4x4 &p_ParentTransform,
            Math::AABB &p_AABB, bool &p_HasVertices)
        {
          const Math::Matrix4x4 l_Transform =
              p_ParentTransform *
              assimp_matrix_to_glm(p_Node->mTransformation);

          for (u32 i = 0u; i < p_Node->mNumMeshes; ++i) {
            aiMesh *i_Mesh = p_Scene->mMeshes[p_Node->mMeshes[i]];
            for (u32 j = 0u; j < i_Mesh->mNumVertices; ++j) {
              include_vertex_in_aabb(
                  p_AABB, transform_vertex(i_Mesh->mVertices[j],
                                           l_Transform));
              p_HasVertices = true;
            }
          }

          for (u32 i = 0u; i < p_Node->mNumChildren; ++i) {
            include_node_vertices_in_aabb(
                p_Scene, p_Node->mChildren[i], l_Transform, p_AABB,
                p_HasVertices);
          }
        }

        static void include_mesh_instances_in_aabb(
            const aiScene *p_Scene, const aiNode *p_Node,
            const u32 p_MeshIndex,
            const Math::Matrix4x4 &p_ParentTransform,
            Math::AABB &p_AABB, bool &p_HasVertices)
        {
          const Math::Matrix4x4 l_Transform =
              p_ParentTransform *
              assimp_matrix_to_glm(p_Node->mTransformation);

          for (u32 i = 0u; i < p_Node->mNumMeshes; ++i) {
            if (p_Node->mMeshes[i] != p_MeshIndex) {
              continue;
            }

            aiMesh *i_Mesh = p_Scene->mMeshes[p_MeshIndex];
            for (u32 j = 0u; j < i_Mesh->mNumVertices; ++j) {
              include_vertex_in_aabb(
                  p_AABB, transform_vertex(i_Mesh->mVertices[j],
                                           l_Transform));
              p_HasVertices = true;
            }
          }

          for (u32 i = 0u; i < p_Node->mNumChildren; ++i) {
            include_mesh_instances_in_aabb(
                p_Scene, p_Node->mChildren[i], p_MeshIndex,
                l_Transform, p_AABB, p_HasVertices);
          }
        }

        static void update_node_bounding_sphere_radius(
            const aiScene *p_Scene, const aiNode *p_Node,
            const Math::Vector3 &p_Center,
            const Math::Matrix4x4 &p_ParentTransform,
            float &p_MaxDistanceSquared)
        {
          const Math::Matrix4x4 l_Transform =
              p_ParentTransform *
              assimp_matrix_to_glm(p_Node->mTransformation);

          for (u32 i = 0u; i < p_Node->mNumMeshes; ++i) {
            aiMesh *i_Mesh = p_Scene->mMeshes[p_Node->mMeshes[i]];
            for (u32 j = 0u; j < i_Mesh->mNumVertices; ++j) {
              const Math::Vector3 j_Position =
                  transform_vertex(i_Mesh->mVertices[j], l_Transform);
              const float j_DistanceSq =
                  Math::VectorUtil::distance_squared(p_Center,
                                                     j_Position);
              p_MaxDistanceSquared =
                  LOW_MATH_MAX(p_MaxDistanceSquared, j_DistanceSq);
            }
          }

          for (u32 i = 0u; i < p_Node->mNumChildren; ++i) {
            update_node_bounding_sphere_radius(
                p_Scene, p_Node->mChildren[i], p_Center, l_Transform,
                p_MaxDistanceSquared);
          }
        }

        static void update_mesh_instance_bounding_sphere_radius(
            const aiScene *p_Scene, const aiNode *p_Node,
            const u32 p_MeshIndex, const Math::Vector3 &p_Center,
            const Math::Matrix4x4 &p_ParentTransform,
            float &p_MaxDistanceSquared)
        {
          const Math::Matrix4x4 l_Transform =
              p_ParentTransform *
              assimp_matrix_to_glm(p_Node->mTransformation);

          for (u32 i = 0u; i < p_Node->mNumMeshes; ++i) {
            if (p_Node->mMeshes[i] != p_MeshIndex) {
              continue;
            }

            aiMesh *i_Mesh = p_Scene->mMeshes[p_MeshIndex];
            for (u32 j = 0u; j < i_Mesh->mNumVertices; ++j) {
              const Math::Vector3 j_Position =
                  transform_vertex(i_Mesh->mVertices[j], l_Transform);
              const float j_DistanceSq =
                  Math::VectorUtil::distance_squared(p_Center,
                                                     j_Position);
              p_MaxDistanceSquared =
                  LOW_MATH_MAX(p_MaxDistanceSquared, j_DistanceSq);
            }
          }

          for (u32 i = 0u; i < p_Node->mNumChildren; ++i) {
            update_mesh_instance_bounding_sphere_radius(
                p_Scene, p_Node->mChildren[i], p_MeshIndex, p_Center,
                l_Transform, p_MaxDistanceSquared);
          }
        }

        static bool
        calculate_bounding_sphere(const aiScene *p_Scene,
                                  const Math::Vector3 &p_Center,
                                  Math::Sphere &p_BoundingSphere)
        {
          LOWR_IMP_ASSERT_RETURN(
              p_Scene && p_Scene->mRootNode &&
                  p_Scene->mNumMeshes > 0,
              "Scene must not be null and must have meshes");

          float l_MaxDistanceSquared = 0.0f;
          update_node_bounding_sphere_radius(
              p_Scene, p_Scene->mRootNode, p_Center,
              Math::Matrix4x4(1.0f), l_MaxDistanceSquared);

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
              p_Scene && p_Scene->mRootNode &&
                  p_Scene->mNumMeshes > 0,
              "Scene must not be null and must have meshes");

          initialize_aabb(p_AABB);

          bool l_HasVertices = false;
          include_node_vertices_in_aabb(p_Scene, p_Scene->mRootNode,
                                        Math::Matrix4x4(1.0f), p_AABB,
                                        l_HasVertices);

          LOWR_IMP_ASSERT_RETURN(
              l_HasVertices,
              "Cannot calculate scene bounding volumes without "
              "vertices.");

          finalize_aabb(p_AABB);

          LOWR_IMP_ASSERT_RETURN(
              calculate_bounding_sphere(p_Scene, p_AABB.center,
                                        p_BoundingSphere),
              "Failed to calculate bounding sphere for scene");

          return true;
        }

        static bool calculate_bounding_volumes(
            const aiScene *p_Scene, const u32 p_MeshIndex,
            Math::Sphere &p_BoundingSphere, Math::AABB &p_AABB)
        {
          LOWR_IMP_ASSERT_RETURN(
              p_Scene && p_Scene->mRootNode &&
                  p_MeshIndex < p_Scene->mNumMeshes,
              "Scene must not be null and mesh index must be valid");

          initialize_aabb(p_AABB);

          bool l_HasVertices = false;
          bool l_UsedLocalMeshFallback = false;
          include_mesh_instances_in_aabb(
              p_Scene, p_Scene->mRootNode, p_MeshIndex,
              Math::Matrix4x4(1.0f), p_AABB, l_HasVertices);

          if (!l_HasVertices) {
            l_UsedLocalMeshFallback = true;
            aiMesh *i_Mesh = p_Scene->mMeshes[p_MeshIndex];
            for (u32 i = 0u; i < i_Mesh->mNumVertices; ++i) {
              include_vertex_in_aabb(p_AABB, i_Mesh->mVertices[i]);
            }
            l_HasVertices = i_Mesh->mNumVertices > 0u;
          }

          LOWR_IMP_ASSERT_RETURN(
              l_HasVertices,
              "Cannot calculate mesh bounding volumes without "
              "vertices.");

          finalize_aabb(p_AABB);

          float l_MaxDistanceSquared = 0.0f;
          update_mesh_instance_bounding_sphere_radius(
              p_Scene, p_Scene->mRootNode, p_MeshIndex, p_AABB.center,
              Math::Matrix4x4(1.0f), l_MaxDistanceSquared);

          if (l_UsedLocalMeshFallback) {
            aiMesh *i_Mesh = p_Scene->mMeshes[p_MeshIndex];
            for (u32 i = 0u; i < i_Mesh->mNumVertices; ++i) {
              const Math::Vector3 i_Position(i_Mesh->mVertices[i].x,
                                             i_Mesh->mVertices[i].y,
                                             i_Mesh->mVertices[i].z);
              l_MaxDistanceSquared =
                  LOW_MATH_MAX(l_MaxDistanceSquared,
                               Math::VectorUtil::distance_squared(
                                   p_AABB.center, i_Position));
            }
          }

          p_BoundingSphere.position = p_AABB.center;
          p_BoundingSphere.radius = glm::sqrt(l_MaxDistanceSquared);

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
                calculate_bounding_volumes(p_Scene, i,
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

        static void append_u32(Util::String &p_Out, const u32 p_Value)
        {
          char l_Buffer[32];
          snprintf(l_Buffer, sizeof(l_Buffer), "%u", p_Value);
          p_Out += l_Buffer;
        }

        static void append_i32(Util::String &p_Out, const i32 p_Value)
        {
          char l_Buffer[32];
          snprintf(l_Buffer, sizeof(l_Buffer), "%d", p_Value);
          p_Out += l_Buffer;
        }

        static void append_i64(Util::String &p_Out, const i64 p_Value)
        {
          char l_Buffer[32];
          snprintf(l_Buffer, sizeof(l_Buffer), "%lld",
                   static_cast<long long>(p_Value));
          p_Out += l_Buffer;
        }

        static void append_quantized_float(Util::String &p_Out,
                                           const float p_Value)
        {
          constexpr float l_Quantization = 100000.0f;
          append_i64(p_Out, static_cast<i64>(std::llround(
                                p_Value * l_Quantization)));
        }

        static void append_matrix(Util::String &p_Out,
                                  const Math::Matrix4x4 &p_Matrix)
        {
          for (u32 i_Column = 0u; i_Column < 4u; ++i_Column) {
            for (u32 i_Row = 0u; i_Row < 4u; ++i_Row) {
              append_quantized_float(p_Out,
                                     p_Matrix[i_Column][i_Row]);
              p_Out += ",";
            }
          }
        }

        static bool collect_relevant_skeleton_nodes(
            const aiNode *p_Node,
            const Util::Map<Util::Name, Math::Matrix4x4>
                &p_DeformingBones,
            Util::Set<Util::Name> &p_RelevantNodes)
        {
          const Util::Name l_NodeName =
              LOW_NAME(p_Node->mName.C_Str());

          bool l_NodeRelevant = p_DeformingBones.find(l_NodeName) !=
                                p_DeformingBones.end();

          for (u32 i = 0u; i < p_Node->mNumChildren; ++i) {
            if (collect_relevant_skeleton_nodes(p_Node->mChildren[i],
                                                p_DeformingBones,
                                                p_RelevantNodes)) {
              l_NodeRelevant = true;
            }
          }

          if (l_NodeRelevant) {
            p_RelevantNodes.insert(l_NodeName);
          }

          return l_NodeRelevant;
        }

        static void extract_skeleton_bone(
            const aiNode *p_Node,
            const Util::Map<Util::Name, Math::Matrix4x4>
                &p_DeformingBones,
            const Util::Set<Util::Name> &p_RelevantNodes,
            const i32 p_ParentIndex,
            const Math::Matrix4x4 &p_ParentTransform,
            SkeletonImportData &p_OutData)
        {
          const Util::Name l_NodeName =
              LOW_NAME(p_Node->mName.C_Str());
          const Math::Matrix4x4 l_LocalTransform =
              assimp_matrix_to_glm(p_Node->mTransformation);
          const Math::Matrix4x4 l_GlobalTransform =
              p_ParentTransform * l_LocalTransform;

          i32 l_CurrentParentIndex = p_ParentIndex;

          if (p_RelevantNodes.find(l_NodeName) !=
              p_RelevantNodes.end()) {
            SkeletonImportBone l_Bone;
            l_Bone.name = l_NodeName;
            l_Bone.parentIndex = p_ParentIndex;
            l_Bone.localBindTransform = l_LocalTransform;
            l_Bone.globalBindTransform = l_GlobalTransform;
            l_Bone.inverseBindMatrix = Math::Matrix4x4(1.0f);
            l_Bone.deforming = false;
            l_Bone.sourceBoneIndex = LOW_UINT32_MAX;

            auto l_DeformingBoneIt =
                p_DeformingBones.find(l_NodeName);
            if (l_DeformingBoneIt != p_DeformingBones.end()) {
              l_Bone.inverseBindMatrix = l_DeformingBoneIt->second;
              l_Bone.deforming = true;
            }

            l_CurrentParentIndex =
                static_cast<i32>(p_OutData.bones.size());
            l_Bone.sourceBoneIndex =
                static_cast<u32>(p_OutData.bones.size());
            p_OutData.boneNameToIndex[l_NodeName] =
                static_cast<u32>(p_OutData.bones.size());
            p_OutData.bones.push_back(l_Bone);
          }

          for (u32 i = 0u; i < p_Node->mNumChildren; ++i) {
            extract_skeleton_bone(p_Node->mChildren[i],
                                  p_DeformingBones, p_RelevantNodes,
                                  l_CurrentParentIndex,
                                  l_GlobalTransform, p_OutData);
          }
        }

        static u64 calculate_skeleton_hash(
            const SkeletonImportData &p_SkeletonData)
        {
          if (p_SkeletonData.bones.empty()) {
            return 0u;
          }

          Util::String l_CanonicalData;
          l_CanonicalData += "LOW_SKELETON_HASH_V1|";
          append_u32(l_CanonicalData, p_SkeletonData.bones.size());
          l_CanonicalData += "|";

          for (u32 i = 0u; i < p_SkeletonData.bones.size(); ++i) {
            const SkeletonImportBone &i_Bone =
                p_SkeletonData.bones[i];

            l_CanonicalData += "bone|";
            append_u32(l_CanonicalData, i_Bone.name.m_Index);
            l_CanonicalData += "|";
            append_i32(l_CanonicalData, i_Bone.parentIndex);
            l_CanonicalData += "|";
            append_matrix(l_CanonicalData, i_Bone.localBindTransform);
            l_CanonicalData += "|";
            append_matrix(l_CanonicalData, i_Bone.inverseBindMatrix);
            l_CanonicalData += "|";
          }

          return Util::fnv1a_64(l_CanonicalData.c_str(),
                                l_CanonicalData.size());
        }

        static bool
        extract_skeleton_import_data(const aiScene *p_Scene,
                                     SkeletonImportData &p_OutData)
        {
          p_OutData.bones.clear();
          p_OutData.boneNameToIndex.clear();
          p_OutData.exactHash = 0u;

          LOWR_IMP_ASSERT_RETURN(
              p_Scene, "Cannot extract skeleton from null scene.");
          LOWR_IMP_ASSERT_RETURN(p_Scene->mRootNode,
                                 "Cannot extract skeleton from scene "
                                 "without root node.");

          Util::Map<Util::Name, Math::Matrix4x4> l_DeformingBones;

          for (u32 i = 0u; i < p_Scene->mNumMeshes; ++i) {
            aiMesh *i_Mesh = p_Scene->mMeshes[i];
            if (!i_Mesh->HasBones()) {
              continue;
            }

            for (u32 j = 0u; j < i_Mesh->mNumBones; ++j) {
              aiBone *j_Bone = i_Mesh->mBones[j];
              const Util::Name j_BoneName =
                  LOW_NAME(j_Bone->mName.C_Str());

              if (l_DeformingBones.find(j_BoneName) ==
                  l_DeformingBones.end()) {
                l_DeformingBones[j_BoneName] =
                    assimp_matrix_to_glm(j_Bone->mOffsetMatrix);
              }
            }
          }

          if (l_DeformingBones.empty()) {
            return true;
          }

          Util::Set<Util::Name> l_RelevantNodes;
          collect_relevant_skeleton_nodes(
              p_Scene->mRootNode, l_DeformingBones, l_RelevantNodes);

          extract_skeleton_bone(p_Scene->mRootNode, l_DeformingBones,
                                l_RelevantNodes, -1,
                                Math::Matrix4x4(1.0f), p_OutData);

          p_OutData.exactHash = calculate_skeleton_hash(p_OutData);

          return true;
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

      static bool import_skeleton(
          const MeshImport::SkeletonImportData &p_SkeletonData,
          const Util::String &p_ImportPath,
          const Util::String &p_OutputPath, u64 *p_SkeletonUniqueId)
      {
        *p_SkeletonUniqueId = 0;

        if (p_SkeletonData.exactHash == 0) {
          return true;
        }

        const u64 l_SkeletonId = Util::generate_unique_id();

        Util::Serial::Node l_ResourceNode;
        l_ResourceNode["skeleton_id"] = Util::U64Id{l_SkeletonId};
        l_ResourceNode["skeleton_hash"] =
            Util::U64Id{p_SkeletonData.exactHash};
        l_ResourceNode["source_file"] = p_ImportPath;
        l_ResourceNode["version"] = 1;
        l_ResourceNode["bone_count"] = p_SkeletonData.bones.size();

        Util::Serial::Node l_DataNode;
        l_DataNode["unique_id"] = Util::U64Id{l_SkeletonId};
        l_DataNode["bone_count"] = p_SkeletonData.bones.size();

        for (u32 i = 0; i < p_SkeletonData.bones.size(); ++i) {
          const MeshImport::SkeletonImportBone &i_Bone =
              p_SkeletonData.bones[i];

          Util::Serial::Node i_BoneNode;
          i_BoneNode["name"] = i_Bone.name;
          i_BoneNode["parent_index"] = i_Bone.parentIndex;
          i_BoneNode["local_bind_transform"] =
              i_Bone.localBindTransform;
          i_BoneNode["global_bind_transform"] =
              i_Bone.globalBindTransform;
          i_BoneNode["inverse_bind_matrix"] =
              i_Bone.inverseBindMatrix;
          i_BoneNode["deforming"] = i_Bone.deforming;

          l_DataNode["bones"].push_back(i_BoneNode);
        }

        const Util::String l_BaseAssetPath =
            Util::get_project().assetCachePath + "\\" +
            Util::hash_to_string(l_SkeletonId);

        const Util::String l_DataPath =
            l_BaseAssetPath + ".skeleton.yaml";

        const Util::String l_ResourcePath =
            p_OutputPath + ".skeletonresource.yaml";

        Util::Serial::write_yaml_file(l_DataPath.c_str(), l_DataNode);
        Util::Serial::write_yaml_file(l_ResourcePath.c_str(),
                                      l_ResourceNode);

        *p_SkeletonUniqueId = l_SkeletonId;

        return true;
      }

      bool import_mesh(Util::String p_ImportPath,
                       Util::String p_OutputPath)
      {
        Assimp::Importer l_Importer;

        using namespace MeshImport;

        MeshSourceData l_SourceData;
        LOWR_IMP_ASSERT_RETURN(
            load_mesh_source_data(p_ImportPath, l_SourceData),
            "Failed to load mesh source data.");

        const u64 l_AssetHash = l_SourceData.assetHash;

        const MeshResource l_OriginalResource =
            get_reimport(l_AssetHash, p_ImportPath);

        const bool l_Reimport = l_OriginalResource.is_alive();

        if (l_Reimport) {
          if (l_OriginalResource.get_asset_hash() == l_AssetHash) {
            LOW_LOG_DEBUG << "Exiting reimport early because mesh "
                             "didn't change."
                          << LOW_LOG_END;
            return "";
          }
        }

        const u64 l_MeshId = l_Reimport
                                 ? l_OriginalResource.get_mesh_id()
                                 : l_AssetHash;

        const aiScene *l_AiScene =
            load_assimp_scene(l_Importer, p_ImportPath, l_SourceData);

        Util::String l_ErrorMessage =
            "Could not load mesh scene from file '";
        l_ErrorMessage = p_ImportPath;
        l_ErrorMessage += "'";

        LOWR_IMP_ASSERT_RETURN(l_AiScene, l_ErrorMessage.c_str());

        SkeletonImportData l_SkeletonData;
        LOWR_IMP_ASSERT_RETURN(
            extract_skeleton_import_data(l_AiScene, l_SkeletonData),
            "Failed to extract skeleton data from mesh file.");

        const bool l_HasBones = !l_SkeletonData.bones.empty();

        u64 l_SkeletonUniqueId = 0;

        if (l_HasBones) {
          LOWR_IMP_ASSERT_RETURN(
              import_skeleton(l_SkeletonData, p_ImportPath,
                              p_OutputPath, &l_SkeletonUniqueId),
              "Failed to import skeleton from mesh file.");
        }

        const Util::String l_FileName =
            p_OutputPath.substr(p_OutputPath.find_last_of("/\\") + 1);

        Util::Serial::Node l_SidecarInfo;
        populate_sidecar_info(l_AiScene, l_FileName, l_SidecarInfo);

        l_SidecarInfo["skeleton"] = 0;
        if (l_HasBones) {
          l_SidecarInfo["skeleton"] = Util::U64Id{l_SkeletonUniqueId};
        }

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

        Util::Serial::Node l_ResourceNode;
        {
          l_ResourceNode["version"] = 1;
          l_ResourceNode["mesh_id"] = Util::U64Id{l_MeshId};
          l_ResourceNode["asset_hash"] = Util::U64Id{l_AssetHash};
          l_ResourceNode["source_file"] = p_ImportPath;
          l_ResourceNode["name"] = l_FileName;
          l_ResourceNode["submesh_count"] =
              l_SidecarInfo["submeshes"].size();

          if (l_HasBones) {
            l_ResourceNode["type"] =
                MeshTypeEnumHelper::entry_name(MeshType::SKELETAL);
          } else {
            l_ResourceNode["type"] =
                MeshTypeEnumHelper::entry_name(MeshType::STATIC);
          }
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
