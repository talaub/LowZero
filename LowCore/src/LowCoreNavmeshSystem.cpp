#include "LowCoreNavmeshSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHandle.h"

#include "LowMathQuaternionUtil.h"

#include "LowRenderer.h"

#include "LowCoreSystem.h"
#include "LowCorePhysicsSystem.h"
#include "LowCorePhysicsObjects.h"
#include "LowCoreRigidbody.h"
#include "LowCoreTransform.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreNavmeshAgent.h"

#include <Recast.h>
#include <ChunkyTriMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMesh.h>
#include <DetourAlloc.h>

// NAVMESH AREA FLAGS
#define LOW_NAVMESH_AREA_GROUND 1
// NAVMESH POLY FLAGS
#define LOW_NAVMESH_POLYFLAG_WALK 1

// CONSTANTS
#define LOW_NAVMESH_MAX_TRIMESH_AREAS 500
#define LOW_NAVMESH_MAX_PATH_SIZE 500

namespace Low {
  namespace Core {
    namespace System {
      namespace Navmesh {
        rcConfig g_RecastConfig;
        rcContext g_RecastContext;
        rcHeightfield *g_HeightField;

        dtNavMesh *g_NavMesh;
        dtNavMeshQuery *g_NavQuery;
        dtCrowd *g_Crowd;

        Util::List<Math::Vector3> g_Triangles;

        Util::List<float> g_VerticesList;

        Math::Vector3 g_Origin(0.0f, 1.0f, -7.0f);
        Math::Vector3 g_HalfExtents(4.0f, 1.0f, 4.0f);

        namespace Debug {
          static void draw_triangle(Math::Vector3 p_V0, Math::Vector3 p_V1,
                                    Math::Vector3 p_V2)
          {
            DebugGeometry::render_triangle(p_V0, p_V1, p_V2, Math::Vector4(),
                                           true, false);
          }

          static void draw_box(float p_MinX, float p_MinY, float p_MinZ,
                               float p_MaxX, float p_MaxY, float p_MaxZ,
                               Math::Color p_Color)
          {
            // Define vertices of the box
            std::vector<Math::Vector3> l_Vertices = {
                {p_MinX, p_MinY, p_MinZ}, // 0
                {p_MaxX, p_MinY, p_MinZ}, // 1
                {p_MaxX, p_MaxY, p_MinZ}, // 2
                {p_MinX, p_MaxY, p_MinZ}, // 3
                {p_MinX, p_MinY, p_MaxZ}, // 4
                {p_MaxX, p_MinY, p_MaxZ}, // 5
                {p_MaxX, p_MaxY, p_MaxZ}, // 6
                {p_MinX, p_MaxY, p_MaxZ}  // 7
            };

            // Define triangles using vertices
            std::vector<std::vector<int>> l_Triangles = {// Front face
                                                         {0, 1, 2},
                                                         {0, 2, 3},
                                                         // Back face
                                                         {4, 5, 6},
                                                         {4, 6, 7},
                                                         // Left face
                                                         {0, 4, 7},
                                                         {0, 7, 3},
                                                         // Right face
                                                         {1, 5, 6},
                                                         {1, 6, 2},
                                                         // Top face
                                                         {3, 2, 6},
                                                         {3, 6, 7},
                                                         // Bottom face
                                                         {0, 1, 5},
                                                         {0, 5, 4}};

            // Draw each triangle
            for (const auto &i_Triangle : l_Triangles) {
              const Math::Vector3 &i_V1 = l_Vertices[i_Triangle[0]];
              const Math::Vector3 &i_V2 = l_Vertices[i_Triangle[1]];
              const Math::Vector3 &i_V3 = l_Vertices[i_Triangle[2]];
              draw_triangle(i_V1, i_V2, i_V3);
            }
          }

          void draw_point(Math::Vector3 p_Point, Math::Color p_Color)
          {
            Math::Sphere l_Sphere;
            l_Sphere.position = p_Point;
            l_Sphere.radius = 0.2f;
            DebugGeometry::render_sphere(l_Sphere, p_Color, true, false);
          }

          void draw_heightfield_walkable(const rcHeightfield &p_Heightfield)
          {

            const float *l_Origin = p_Heightfield.bmin;
            const float l_CellSize = p_Heightfield.cs;
            const float l_CellHeight = p_Heightfield.ch;

            const int l_Width = p_Heightfield.width;
            const int l_Height = p_Heightfield.height;

            Math::Color l_Color(217.0f / 255.0f, 217.0f / 255.0f,
                                217.0f / 255.0f, 255.0f / 255.0f);

            for (int i_Y = 0; i_Y < l_Height; ++i_Y) {
              for (int i_X = 0; i_X < l_Width; ++i_X) {
                float i_FX = l_Origin[0] + i_X * l_CellSize;
                float i_FZ = l_Origin[2] + i_Y * l_CellSize;
                const rcSpan *i_Span = p_Heightfield.spans[i_X + i_Y * l_Width];
                while (i_Span) {
                  if (i_Span->area == RC_WALKABLE_AREA)
                    l_Color = Math::Color(64.0f / 255.0f, 128.0f / 255.0f,
                                          160.0f / 255.0f, 255.0f / 255.0f);
                  else if (i_Span->area == RC_NULL_AREA)
                    l_Color = Math::Color(64.0f / 255.0f, 64.0f / 255.0f,
                                          64.0f / 255.0f, 255.0f / 255.0f);

                  draw_box(i_FX, l_Origin[1] + i_Span->smin * l_CellHeight,
                           i_FZ, i_FX + l_CellSize,
                           l_Origin[1] + i_Span->smax * l_CellHeight,
                           i_FZ + l_CellSize, l_Color);
                  i_Span = i_Span->next;
                }
              }
            }
          }

          static void draw_navmesh_tile(const dtNavMesh &p_Navmesh,
                                        const dtNavMeshQuery *p_NavQuery,
                                        const dtMeshTile *p_Tile,
                                        unsigned char p_Flags)
          {
            dtPolyRef l_Base = p_Navmesh.getPolyRefBase(p_Tile);

            Util::List<Math::Vector3> l_Vertices;

            int l_TileNum = p_Navmesh.decodePolyIdTile(l_Base);

            for (int i = 0; i < p_Tile->header->polyCount; ++i) {
              const dtPoly *i_Poly = &p_Tile->polys[i];
              if (i_Poly->getType() ==
                  DT_POLYTYPE_OFFMESH_CONNECTION) // Skip off-mesh links.
                continue;

              const dtPolyDetail *i_PolyDetail = &p_Tile->detailMeshes[i];

              for (int j = 0; j < i_PolyDetail->triCount; ++j) {
                const unsigned char *i_Tri =
                    &p_Tile->detailTris[(i_PolyDetail->triBase + j) * 4];
                for (int k = 0; k < 3; ++k) {
                  if (i_Tri[k] < i_Poly->vertCount)
                    l_Vertices.push_back(
                        *(Math::Vector3 *)&p_Tile
                             ->verts[i_Poly->verts[i_Tri[k]] * 3]);
                  else
                    l_Vertices.push_back(
                        *(Math::Vector3 *)&p_Tile
                             ->detailVerts[(i_PolyDetail->vertBase + i_Tri[k] -
                                            i_Poly->vertCount) *
                                           3]);
                }
              }
            }

            for (int i = 0; i < l_Vertices.size(); i += 3) {
              draw_triangle(l_Vertices[i], l_Vertices[i + 1],
                            l_Vertices[i + 2]);
            }

            for (int i = 0; i < p_Tile->header->vertCount; ++i) {
              const float *i_Vertex = &p_Tile->verts[i * 3];
              draw_point({i_Vertex[0], i_Vertex[1], i_Vertex[2]},
                         Math::Color(0.0f, 1.0f, 0.0f, 0.5f));
            }
          }

          void draw_navmesh(const dtNavMesh &p_Navmesh)
          {

            for (int i = 0; i < p_Navmesh.getMaxTiles(); ++i) {
              const dtMeshTile *i_Tile = p_Navmesh.getTile(i);
              if (!i_Tile->header)
                continue;
              draw_navmesh_tile(p_Navmesh, 0, i_Tile, 0);
            }
          }

          void
          draw_poly_mesh_detail(const struct rcPolyMeshDetail &p_PolyMeshDetail)
          {
            for (int i = 0; i < p_PolyMeshDetail.nmeshes; ++i) {
              const unsigned int *i_Meshes = &p_PolyMeshDetail.meshes[i * 4];
              const unsigned int i_BVerts = i_Meshes[0];
              const unsigned int i_BTris = i_Meshes[2];
              const int i_NTris = (int)i_Meshes[3];
              const float *i_Verts = &p_PolyMeshDetail.verts[i_BVerts * 3];
              const unsigned char *i_Tris = &p_PolyMeshDetail.tris[i_BTris * 4];

              for (int j = 0; j < i_NTris; ++j) {
                draw_triangle(
                    *(Math::Vector3 *)&i_Verts[i_Tris[j * 4 + 0] * 3],
                    *(Math::Vector3 *)&i_Verts[i_Tris[j * 4 + 1] * 3],
                    *(Math::Vector3 *)&i_Verts[i_Tris[j * 4 + 2] * 3]);
              }
            }
          }

          void draw_poly_mesh(const struct rcPolyMesh &p_PolyMesh)
          {
            const int l_Nvp = p_PolyMesh.nvp;
            const float l_CellSize = p_PolyMesh.cs;
            const float l_CellHeight = p_PolyMesh.ch;
            const float *l_Origin = p_PolyMesh.bmin;

            Util::List<Math::Vector3> l_Vertices;

            for (int i = 0; i < p_PolyMesh.npolys; ++i) {
              const unsigned short *i_Poly = &p_PolyMesh.polys[i * l_Nvp * 2];
              const unsigned char i_Area = p_PolyMesh.areas[i];

              unsigned short i_Vi[3];
              for (int j = 2; j < l_Nvp; ++j) {
                if (i_Poly[j] == RC_MESH_NULL_IDX)
                  break;
                i_Vi[0] = i_Poly[0];
                i_Vi[1] = i_Poly[j - 1];
                i_Vi[2] = i_Poly[j];
                for (int k = 0; k < 3; ++k) {
                  const unsigned short *i_V = &p_PolyMesh.verts[i_Vi[k] * 3];
                  const float i_X = l_Origin[0] + i_V[0] * l_CellSize;
                  const float i_Y = l_Origin[1] + (i_V[1] + 1) * l_CellHeight;
                  const float i_Z = l_Origin[2] + i_V[2] * l_CellSize;
                  l_Vertices.push_back({i_X, i_Y, i_Z});
                }
              }
            }

            for (int i = 0; i < l_Vertices.size(); i += 3) {
              draw_triangle(l_Vertices[i], l_Vertices[i + 1],
                            l_Vertices[i + 2]);
            }

            for (int i = 0; i < p_PolyMesh.nverts; ++i) {
              const unsigned short *i_V = &p_PolyMesh.verts[i * 3];
              const float i_X = l_Origin[0] + i_V[0] * l_CellSize;
              const float i_Y =
                  l_Origin[1] + (i_V[1] + 1) * l_CellHeight + 0.1f;
              const float i_Z = l_Origin[2] + i_V[2] * l_CellSize;
              draw_point({i_X, i_Y, i_Z}, Math::Color(0.0f, 1.0f, 0.0f, 1.0f));
            }
          }
        } // namespace Debug

        static void gather_geometry(const Math::Vector3 &p_Origin,
                                    const Math::Vector3 &p_HalfExtent,
                                    Util::List<float> &p_Geometry)
        {
          physx::PxScene *l_PhysicsScene = Core::Physics::get_scene();

          Math::Vector3 l_Origin = p_Origin;
          Math::Vector3 l_HalfExtent = p_HalfExtent;

          Util::List<Math::Vector3> l_Vertices;
          Math::Quaternion l_Quat = Math::QuaternionUtil::get_identity();
          // TODO: Cleanup
          uint32_t l_CollisionMask = 0; //(1u << 0u) | (1u << 8u);
          Core::Physics::get_overlapping_mesh_with_box(
              l_PhysicsScene, l_HalfExtent, l_Origin, l_Quat, l_CollisionMask,
              l_Vertices, 0);

          Util::List<Core::Physics::OverlapResult> l_OverlapResults;

          Core::Physics::overlap_box(l_PhysicsScene, l_HalfExtent, l_Origin,
                                     l_Quat, l_OverlapResults, l_CollisionMask,
                                     0);

          Util::List<Math::Vector3> l_ShapeVertices;

          for (size_t i = 0; i < l_OverlapResults.size(); ++i) {
            void *i_UserData = l_OverlapResults[i].actor->userData;

            Util::Handle i_Handle =
                Util::find_handle_by_unique_id(*(Util::UniqueId *)i_UserData);

            if (i_Handle.get_type() != Component::Rigidbody::TYPE_ID) {
              continue;
            }

            Component::Rigidbody i_Rigidbody = i_Handle.get_id();

            if (!i_Rigidbody.is_alive()) {
              continue;
            }

            // Currently only box shapes are supported
            if (i_Rigidbody.get_shape().type != Math::ShapeType::BOX) {
              continue;
            }

            Entity i_Entity = i_Rigidbody.get_entity();
            Component::Transform i_Transform = i_Entity.get_transform();

            Math::Vector3 i_Position = i_Transform.get_world_position();
            Math::Quaternion i_Rotation = i_Transform.get_world_rotation();
            Math::Vector3 i_Scale = i_Transform.get_world_scale();
            Math::Vector3 i_HalfExtents =
                i_Rigidbody.get_shape().box.halfExtents;

            float i_OffsetY = i_HalfExtents.y * i_Scale.y;
            float i_OffsetX = i_HalfExtents.x * i_Scale.x;
            float i_OffsetZ = i_HalfExtents.z * i_Scale.z;

            Math::Vector3 i_BottomXZ =
                i_Position +
                (i_Rotation * Math::Vector3(i_OffsetX, -i_OffsetY, i_OffsetZ));
            Math::Vector3 i_BottomMinXZ =
                i_Position +
                (i_Rotation * Math::Vector3(-i_OffsetX, -i_OffsetY, i_OffsetZ));
            Math::Vector3 i_BottomXMinZ =
                i_Position +
                (i_Rotation * Math::Vector3(i_OffsetX, -i_OffsetY, -i_OffsetZ));
            Math::Vector3 i_BottomMinXMinZ =
                i_Position + (i_Rotation * Math::Vector3(-i_OffsetX, -i_OffsetY,
                                                         -i_OffsetZ));
            Math::Vector3 i_TopXZ =
                i_Position +
                (i_Rotation * Math::Vector3(i_OffsetX, i_OffsetY, i_OffsetZ));
            Math::Vector3 i_TopMinXZ =
                i_Position +
                (i_Rotation * Math::Vector3(-i_OffsetX, i_OffsetY, i_OffsetZ));
            Math::Vector3 i_TopXMinZ =
                i_Position +
                (i_Rotation * Math::Vector3(i_OffsetX, i_OffsetY, -i_OffsetZ));
            Math::Vector3 i_TopMinXMinZ =
                i_Position +
                (i_Rotation * Math::Vector3(-i_OffsetX, i_OffsetY, -i_OffsetZ));

            {
              using namespace physx;

              PxBoxGeometry i_BoxGeometry;
              // Get the dimensions of the box
              i_Rigidbody.get_rigid_dynamic().get_physx_shape()->getBoxGeometry(
                  i_BoxGeometry);

              PxVec3 i_Dimensions = i_BoxGeometry.halfExtents;

              // Calculate the corner vertices in local space
              PxVec3 i_CornersLocal[8] = {
                  PxVec3(-i_Dimensions.x, -i_Dimensions.y, -i_Dimensions.z),
                  PxVec3(i_Dimensions.x, -i_Dimensions.y, -i_Dimensions.z),
                  PxVec3(i_Dimensions.x, i_Dimensions.y, -i_Dimensions.z),
                  PxVec3(-i_Dimensions.x, i_Dimensions.y, -i_Dimensions.z),
                  PxVec3(-i_Dimensions.x, -i_Dimensions.y, i_Dimensions.z),
                  PxVec3(i_Dimensions.x, -i_Dimensions.y, i_Dimensions.z),
                  PxVec3(i_Dimensions.x, i_Dimensions.y, i_Dimensions.z),
                  PxVec3(-i_Dimensions.x, i_Dimensions.y, i_Dimensions.z)};

              // Transform the local space vertices to global space
              PxTransform i_Transform =
                  i_Rigidbody.get_rigid_dynamic().get_physx_transform();
              PxVec3 i_CornersGlobal[8];
              for (int i = 0; i < 8; ++i) {
                i_CornersGlobal[i] = i_Transform.transform(i_CornersLocal[i]);
              }

#if 1
              // Clockwise
              const int i_FaceIndices[12][3] = {
                  {0, 1, 2}, {0, 2, 3}, // Bottom face
                  {4, 7, 6}, {4, 6, 5}, // Top face
                  {0, 4, 5}, {0, 5, 1}, // Side face 1
                  {2, 6, 7}, {2, 7, 3}, // Side face 2
                  {0, 3, 7}, {0, 7, 4}, // Side face 3
                  {1, 5, 6}, {1, 6, 2}  // Side face 4
              };
#else
              // Counter Clockwise
              const int i_FaceIndices[12][3] = {
                  {0, 2, 1}, {0, 3, 2}, // Bottom face
                  {4, 6, 7}, {4, 5, 6}, // Top face
                  {0, 5, 4}, {0, 1, 5}, // Side face 1
                  {2, 7, 6}, {2, 3, 7}, // Side face 2
                  {0, 7, 3}, {0, 4, 7}, // Side face 3
                  {1, 6, 5}, {1, 2, 6}  // Side face 4
              };
#endif

              // Create a list of triangles
              for (int i = 0; i < 12; ++i) {
                PxVec3 i_Vertex0 = i_CornersGlobal[i_FaceIndices[i][0]];
                PxVec3 i_Vertex1 = i_CornersGlobal[i_FaceIndices[i][1]];
                PxVec3 i_Vertex2 = i_CornersGlobal[i_FaceIndices[i][2]];

                l_ShapeVertices.push_back(
                    Math::Vector3(i_Vertex0.x, i_Vertex0.y, i_Vertex0.z));
                l_ShapeVertices.push_back(
                    Math::Vector3(i_Vertex1.x, i_Vertex1.y, i_Vertex1.z));
                l_ShapeVertices.push_back(
                    Math::Vector3(i_Vertex2.x, i_Vertex2.y, i_Vertex2.z));
              }
            }
          }

          const size_t i_VerticesCountBefore = p_Geometry.size();

          for (auto &i_Vertex : l_Vertices) {
            p_Geometry.push_back(i_Vertex.x);
            p_Geometry.push_back(i_Vertex.y);
            p_Geometry.push_back(i_Vertex.z);
          }

          for (auto &i_Vertex : l_ShapeVertices) {
            p_Geometry.push_back(i_Vertex.x);
            p_Geometry.push_back(i_Vertex.y);
            p_Geometry.push_back(i_Vertex.z);
          }

          l_Vertices.clear();
          l_ShapeVertices.clear();
        }

        void process_navmesh(rcContext &p_Ctx)
        {
          Util::List<float> l_Geometry;
          gather_geometry(g_Origin, g_HalfExtents, l_Geometry);

          uint32_t l_TriangleIndex = 0;
          Util::List<int> l_Indices;
          for (uint32_t i = 0; i < l_Geometry.size(); i = i + 3) {
            l_Indices.push_back(l_TriangleIndex++);
          }

          // Assuming you have a list of triangles in the form of vertices
          // Create a bounding box for the geometry
          float l_BoundingMin[3] = {g_Origin.x - (g_HalfExtents.x * 2.0f),
                                    g_Origin.y - (g_HalfExtents.y * 2.0f),
                                    g_Origin.z - (g_HalfExtents.z * 2.0f)};
          float l_BoundingMax[3] = {g_Origin.x + (g_HalfExtents.x * 2.0f),
                                    g_Origin.y + (g_HalfExtents.y * 2.0f),
                                    g_Origin.z + (g_HalfExtents.z * 2.0f)};

          g_RecastConfig.bmin[0] = l_BoundingMin[0];
          g_RecastConfig.bmin[1] = l_BoundingMin[1];
          g_RecastConfig.bmin[2] = l_BoundingMin[2];

          g_RecastConfig.bmax[0] = l_BoundingMax[0];
          g_RecastConfig.bmax[1] = l_BoundingMax[1];
          g_RecastConfig.bmax[2] = l_BoundingMax[2];

          g_RecastConfig.maxEdgeLen =
              static_cast<int>(2.0f / g_RecastConfig.cs);
          g_RecastConfig.maxEdgeLen = 20;
          g_RecastConfig.maxSimplificationError = 1;

          g_RecastConfig.minRegionArea = 1;
          g_RecastConfig.mergeRegionArea = 0;

          g_RecastConfig.maxVertsPerPoly = 3;
          g_RecastConfig.borderSize = 2;

          rcCalcGridSize(g_RecastConfig.bmin, g_RecastConfig.bmax,
                         g_RecastConfig.cs, &g_RecastConfig.width,
                         &g_RecastConfig.height);

          rcChunkyTriMesh *l_ChunkyMesh = new rcChunkyTriMesh;
          rcCreateChunkyTriMesh(l_Geometry.data(), l_Indices.data(),
                                l_Indices.size() / 3, 512, l_ChunkyMesh);

          // Initialize a heightfield
          rcHeightfield *l_Heightfield = rcAllocHeightfield();
          LOW_ASSERT(rcCreateHeightfield(
                         &g_RecastContext, *l_Heightfield, g_RecastConfig.width,
                         g_RecastConfig.height, l_BoundingMin, l_BoundingMax,
                         g_RecastConfig.cs, g_RecastConfig.ch),
                     "Failed to create heightfield");

          unsigned char *l_Triareas =
              new unsigned char[LOW_NAVMESH_MAX_TRIMESH_AREAS];

          // Find triangles which are walkable based on their slope and
          // rasterize them. If your input data is multiple meshes, you can
          // transform them here, calculate the are type for each of the
          // meshes and rasterize them.
          memset(l_Triareas, 0,
                 LOW_NAVMESH_MAX_TRIMESH_AREAS * sizeof(unsigned char));
          float tbmin[2], tbmax[2];
          tbmin[0] = g_RecastConfig.bmin[0];
          tbmin[1] = g_RecastConfig.bmin[2];
          tbmax[0] = g_RecastConfig.bmax[0];
          tbmax[1] = g_RecastConfig.bmax[2];
          int cid[1024]; // TODO: Make grow when returning too many items.
          const int ncid =
              rcGetChunksOverlappingRect(l_ChunkyMesh, tbmin, tbmax, cid, 1024);

          _LOW_ASSERT(ncid);

          for (int i = 0; i < ncid; ++i) {
            const rcChunkyTriMeshNode &node = l_ChunkyMesh->nodes[cid[i]];
            const int *ctris = &l_ChunkyMesh->tris[node.i * 3];
            const int nctris = node.n;

            rcMarkWalkableTriangles(&p_Ctx, g_RecastConfig.walkableSlopeAngle,
                                    l_Geometry.data(), l_Geometry.size(), ctris,
                                    nctris, l_Triareas);

            _LOW_ASSERT(rcRasterizeTriangles(
                &g_RecastContext, l_Geometry.data(), l_Geometry.size(), ctris,
                l_Triareas, nctris, *l_Heightfield,
                g_RecastConfig.walkableClimb));
          }

          int l_SpanCount = rcGetHeightFieldSpanCount(&p_Ctx, *l_Heightfield);

          rcFilterLowHangingWalkableObstacles(
              &p_Ctx, g_RecastConfig.walkableClimb, *l_Heightfield);

          rcCompactHeightfield *l_CompactHeightField =
              rcAllocCompactHeightfield();
          LOW_ASSERT(l_CompactHeightField,
                     "Could not allocator compact heightfield");

          LOW_ASSERT(rcBuildCompactHeightfield(
                         &g_RecastContext, g_RecastConfig.walkableHeight,
                         g_RecastConfig.walkableClimb, *l_Heightfield,
                         *l_CompactHeightField),
                     "Could not build compact heightfield");

          LOW_ASSERT(rcErodeWalkableArea(&g_RecastContext,
                                         g_RecastConfig.walkableRadius,
                                         *l_CompactHeightField),
                     "Failed to erode walkable areas");

          LOW_ASSERT(
              rcBuildDistanceField(&g_RecastContext, *l_CompactHeightField),
              "Failed to build distance field");

          LOW_ASSERT(rcBuildRegions(&g_RecastContext, *l_CompactHeightField,
                                    g_RecastConfig.borderSize,
                                    g_RecastConfig.minRegionArea,
                                    g_RecastConfig.mergeRegionArea),
                     "Failed to build recast regions");

          rcContourSet *l_ContourSet = rcAllocContourSet();
          _LOW_ASSERT(l_ContourSet);

          LOW_ASSERT(rcBuildContours(&g_RecastContext, *l_CompactHeightField,
                                     g_RecastConfig.maxSimplificationError,
                                     g_RecastConfig.maxEdgeLen, *l_ContourSet),
                     "Failed to build recast contour set");

          LOW_ASSERT(l_ContourSet->nconts, "Recast contourset is empty");

          rcPolyMesh *l_PolyMesh = rcAllocPolyMesh();
          _LOW_ASSERT(l_PolyMesh);

          LOW_ASSERT(rcBuildPolyMesh(&g_RecastContext, *l_ContourSet,
                                     g_RecastConfig.maxVertsPerPoly,
                                     *l_PolyMesh),
                     "Failer to build recast poly mesh");

          rcPolyMeshDetail *l_DetailMesh = rcAllocPolyMeshDetail();
          _LOW_ASSERT(l_DetailMesh);

          LOW_ASSERT(rcBuildPolyMeshDetail(
                         &g_RecastContext, *l_PolyMesh, *l_CompactHeightField,
                         g_RecastConfig.detailSampleDist,
                         g_RecastConfig.detailSampleMaxError, *l_DetailMesh),
                     "Failed to build recast detail mesh");

          for (int i = 0; i < l_PolyMesh->npolys; ++i) {
            if (l_PolyMesh->areas[i] == RC_WALKABLE_AREA)
              l_PolyMesh->areas[i] = LOW_NAVMESH_AREA_GROUND;
            if (l_PolyMesh->areas[i] == LOW_NAVMESH_AREA_GROUND) {
              l_PolyMesh->flags[i] = LOW_NAVMESH_POLYFLAG_WALK;
            }
          }

          dtNavMeshCreateParams params;
          memset(&params, 0, sizeof(params));
          params.verts = l_PolyMesh->verts;
          params.vertCount = l_PolyMesh->nverts;
          params.polys = l_PolyMesh->polys;
          params.polyAreas = l_PolyMesh->areas;
          params.polyFlags = l_PolyMesh->flags;
          params.polyCount = l_PolyMesh->npolys;
          params.nvp = l_PolyMesh->nvp;
          params.detailMeshes = l_DetailMesh->meshes;
          params.detailVerts = l_DetailMesh->verts;
          params.detailVertsCount = l_DetailMesh->nverts;
          params.detailTris = l_DetailMesh->tris;
          params.detailTriCount = l_DetailMesh->ntris;
          params.walkableHeight = g_RecastConfig.walkableHeight;
          params.walkableRadius = g_RecastConfig.walkableRadius;
          params.walkableClimb = g_RecastConfig.walkableClimb;

          params.tileLayer = 0;
          params.tileX = 0;
          params.tileY = 0;

          rcVcopy(params.bmin, l_PolyMesh->bmin);
          rcVcopy(params.bmax, l_PolyMesh->bmax);
          params.cs = g_RecastConfig.cs;
          params.ch = g_RecastConfig.ch;
          params.buildBvTree = true;

          unsigned char *l_NavData;
          int l_NavDataSize = 0;

          LOW_ASSERT(dtCreateNavMeshData(&params, &l_NavData, &l_NavDataSize),
                     "Could not create navmesh data");

          g_NavMesh = dtAllocNavMesh();

          g_NavQuery = dtAllocNavMeshQuery();
          LOW_ASSERT(dtStatusSucceed(g_NavQuery->init(g_NavMesh, 2048)),
                     "Failed to initialize navmesh query");

          g_NavMesh->init(l_NavData, l_NavDataSize, DT_TILE_FREE_DATA);
          // MARK
        }

        int add_agent(Math::Vector3 &p_Position, dtCrowdAgentParams *p_Params)
        {
          return g_Crowd->addAgent((float *)&p_Position, p_Params);
        }

        void set_agent_target_position(Component::NavmeshAgent p_Agent,
                                       Math::Vector3 &p_Position)
        {

          const dtQueryFilter *l_Filter = g_Crowd->getFilter(0);
          const float *l_HalfExtents = g_Crowd->getQueryHalfExtents();

          Math::Vector3 l_NearestStartPoint;
          Math::Vector3 l_NearestTargetPoint;

          dtPolyRef startPoly = 0;
          dtPolyRef endPoly = 0;

          Math::Vector3 l_AgentWorldPos =
              p_Agent.get_entity().get_transform().get_world_position();

          {
            dtStatus status = g_NavQuery->findNearestPoly(
                (float *)&l_AgentWorldPos, l_HalfExtents, l_Filter, &startPoly,
                (float *)&l_NearestStartPoint);

            LOW_ASSERT(dtStatusSucceed(status),
                       "Failed finding nearest polygon to start position");
          }
          {
            dtStatus status = g_NavQuery->findNearestPoly(
                (float *)&p_Position, l_HalfExtents, l_Filter, &endPoly,
                (float *)&l_NearestTargetPoint);

            LOW_ASSERT(dtStatusSucceed(status),
                       "Failed finding nearest polygon to target position");
          }

          dtPolyRef path[LOW_NAVMESH_MAX_PATH_SIZE];
          int pathCount;
          g_NavQuery->findPath(startPoly, endPoly, (float *)&l_AgentWorldPos,
                               (float *)&l_NearestTargetPoint, l_Filter, path,
                               &pathCount, LOW_NAVMESH_MAX_PATH_SIZE);

          const dtCrowdAgent *l_Agent =
              g_Crowd->getAgent(p_Agent.get_agent_index());

          g_Crowd->requestMoveTarget(p_Agent.get_agent_index(), endPoly,
                                     (float *)&l_NearestTargetPoint);
        }

        static void start()
        {
          LOW_LOG_INFO << "Setting up recast" << LOW_LOG_END;

          memset(&g_RecastConfig, 0, sizeof(g_RecastConfig));

#if 1
          g_RecastConfig.cs = 0.3f;
          g_RecastConfig.ch = 0.5f;
#else
          g_RecastConfig.cs = 1.0f;
          g_RecastConfig.ch = 1.0f;
#endif
          g_RecastConfig.walkableRadius = 1.0f;
          g_RecastConfig.walkableSlopeAngle = 45.0f;

          g_RecastConfig.tileSize = 32;

          // Other parameters you may want to customize based on your specific
          // needs
          g_RecastConfig.detailSampleDist = 1.0f;
          g_RecastConfig.detailSampleMaxError = 0.1f;

          // Set the height of the un-walkable areas (e.g., cliffs, walls)
          g_RecastConfig.walkableHeight = static_cast<int>(
              ceilf(3.0f / g_RecastConfig.ch)); // 3 meters in world units
          g_RecastConfig.walkableHeight = 3;
          g_RecastConfig.walkableClimb = static_cast<int>(
              floorf(0.05f / g_RecastConfig.ch)); // 0.9 meters in world units
          g_RecastConfig.walkableClimb = 2;

          g_RecastContext.enableLog(true);

          process_navmesh(g_RecastContext);

          g_Crowd = dtAllocCrowd();
          _LOW_ASSERT(g_Crowd);

          g_Crowd->init(100, 1.0f, g_NavMesh);

          LOW_LOG_INFO << "Finished setting up recast" << LOW_LOG_END;
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          LOW_PROFILE_CPU("Core", "Navmesh Tick");
          SYSTEM_ON_START(start);

          int a = g_NavMesh->getMaxTiles();

          // Debug::draw_navmesh(*g_NavMesh);

          if (p_State != Util::EngineState::PLAYING) {
            return;
          }

          Component::NavmeshAgent *l_Agents =
              Component::NavmeshAgent::living_instances();

          g_Crowd->update(p_Delta, nullptr);

          const dtQueryFilter *l_Filter = g_Crowd->getFilter(0);
          const float *l_HalfExtents = g_Crowd->getQueryHalfExtents();

          for (uint32_t i = 0u; i < Component::NavmeshAgent::living_count();
               ++i) {
            Component::NavmeshAgent i_Agent = l_Agents[i];
            Entity i_Entity = i_Agent.get_entity();
            Component::Transform i_Transform = i_Entity.get_transform();

            // If agent has not been initialized
            if (i_Agent.get_agent_index() < 0) {
              // TODO: read values from transform or component

              dtPolyRef i_ClosestPoly = 0;
              Math::Vector3 i_NearestNavmeshPoint;

              Math::Vector3 i_AgentWorldPos =
                  i_Transform.get_world_position() + i_Agent.get_offset();

              {
                dtStatus i_Status = g_NavQuery->findNearestPoly(
                    (float *)&i_AgentWorldPos, l_HalfExtents, l_Filter,
                    &i_ClosestPoly, (float *)&i_NearestNavmeshPoint);

                LOW_ASSERT(dtStatusSucceed(i_Status),
                           "Failed finding nearest polygon to start position");
              }

              dtCrowdAgentParams i_Params;
              i_Params.radius = i_Agent.get_radius();
              i_Params.height = i_Agent.get_height();
              i_Params.maxAcceleration = 1.0f;
              i_Params.maxSpeed = 2.0f;
              i_Params.collisionQueryRange = 0.1f;
              i_Params.pathOptimizationRange = 0.2f;
              i_Params.queryFilterType = 0;

              int i_AgentIndex =
                  System::Navmesh::add_agent(i_NearestNavmeshPoint, &i_Params);

              i_Agent.set_agent_index(i_AgentIndex);
            }

            const dtCrowdAgent *i_NavAgent =
                g_Crowd->getAgent(i_Agent.get_agent_index());

            Math::Vector3 i_TargetPos = *(Math::Vector3 *)i_NavAgent->targetPos;
            Math::Vector3 i_WorldPosition = *(Math::Vector3 *)i_NavAgent->npos;
            i_WorldPosition -= i_Agent.get_offset();
            Math::Vector3 i_LocalPosition = i_WorldPosition;

            Component::Transform i_Parent = i_Transform.get_parent();

            if (i_Parent.is_alive()) {
              i_LocalPosition =
                  glm::inverse(i_Parent.get_world_matrix()) *
                  Math::Vector4(i_LocalPosition.x, i_LocalPosition.y,
                                i_LocalPosition.z, 1.0f);
            }

            i_Transform.position(i_LocalPosition);
          }
        }
      } // namespace Navmesh
    }   // namespace System
  }     // namespace Core
} // namespace Low
