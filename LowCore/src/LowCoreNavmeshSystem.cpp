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

        Util::List<float> g_VerticesList;

        Math::Vector3 g_Origin(0.0f, 1.0f, -7.0f);
        Math::Vector3 g_HalfExtents(4.0f, 1.0f, 4.0f);

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

            l_ShapeVertices.push_back(i_BottomXZ);
            l_ShapeVertices.push_back(i_BottomXMinZ);
            l_ShapeVertices.push_back(i_BottomMinXMinZ);
            l_ShapeVertices.push_back(i_BottomXZ);
            l_ShapeVertices.push_back(i_BottomMinXMinZ);
            l_ShapeVertices.push_back(i_BottomMinXZ);
            l_ShapeVertices.push_back(i_TopXZ);
            l_ShapeVertices.push_back(i_TopXMinZ);
            l_ShapeVertices.push_back(i_TopMinXMinZ);
            l_ShapeVertices.push_back(i_TopXZ);
            l_ShapeVertices.push_back(i_TopMinXMinZ);
            l_ShapeVertices.push_back(i_TopMinXZ);

            l_ShapeVertices.push_back(i_BottomXZ);
            l_ShapeVertices.push_back(i_TopXZ);
            l_ShapeVertices.push_back(i_TopMinXZ);
            l_ShapeVertices.push_back(i_BottomXZ);
            l_ShapeVertices.push_back(i_TopMinXZ);
            l_ShapeVertices.push_back(i_BottomMinXZ);
            l_ShapeVertices.push_back(i_BottomXMinZ);
            l_ShapeVertices.push_back(i_TopMinXMinZ);
            l_ShapeVertices.push_back(i_TopXMinZ);
            l_ShapeVertices.push_back(i_BottomXMinZ);
            l_ShapeVertices.push_back(i_BottomMinXMinZ);
            l_ShapeVertices.push_back(i_TopMinXMinZ);

            l_ShapeVertices.push_back(i_BottomXZ);
            l_ShapeVertices.push_back(i_BottomXMinZ);
            l_ShapeVertices.push_back(i_TopXMinZ);
            l_ShapeVertices.push_back(i_BottomXZ);
            l_ShapeVertices.push_back(i_TopXMinZ);
            l_ShapeVertices.push_back(i_TopXZ);
            l_ShapeVertices.push_back(i_BottomMinXZ);
            l_ShapeVertices.push_back(i_TopMinXZ);
            l_ShapeVertices.push_back(i_TopMinXMinZ);
            l_ShapeVertices.push_back(i_BottomMinXZ);
            l_ShapeVertices.push_back(i_TopMinXMinZ);
            l_ShapeVertices.push_back(i_BottomMinXMinZ);
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
          float l_BoundingMin[3] = {g_Origin.x - g_HalfExtents.x,
                                    g_Origin.y - g_HalfExtents.y,
                                    g_Origin.z - g_HalfExtents.z};
          float l_BoundingMax[3] = {g_Origin.x + g_HalfExtents.x,
                                    g_Origin.y + g_HalfExtents.y,
                                    g_Origin.z + g_HalfExtents.z};

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

          g_RecastConfig.maxVertsPerPoly = 20;
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

          unsigned char *l_Triareas = new unsigned char[500];

          // Find triangles which are walkable based on their slope and
          // rasterize them. If your input data is multiple meshes, you can
          // transform them here, calculate the are type for each of the meshes
          // and rasterize them.
          memset(l_Triareas, 0, 500 * sizeof(unsigned char));
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
          LOW_LOG_DEBUG << "SPAN_COUNT: " << l_SpanCount << LOW_LOG_END;

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

          params.tileX = 0;
          params.tileY = 0;
          params.tileLayer = 0;

          Math::Box l_Box;
          l_Box.halfExtents = g_HalfExtents;
          l_Box.position = g_Origin;
          l_Box.rotation = Math::QuaternionUtil::get_identity();

          DebugGeometry::render_box(l_Box, Math::Color(1.0f, 0.0f, 0.0f, 1.0f),
                                    true, true);

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
          g_NavMesh->init(l_NavData, l_NavDataSize, DT_TILE_FREE_DATA);
        }

        int add_agent(Math::Vector3 &p_Position, dtCrowdAgentParams *p_Params)
        {
          return g_Crowd->addAgent((float *)&p_Position, p_Params);
        }

        void set_agent_target_position(int p_AgentIndex,
                                       Math::Vector3 &p_Position)
        {

          const dtQueryFilter *l_Filter = g_Crowd->getFilter(0);
          const float *l_HalfExtents = g_Crowd->getQueryHalfExtents();

          dtPolyRef l_Poly = 500;
          float l_Out = 0;

          dtStatus l_PolyResult = g_NavQuery->findNearestPoly(
              (float *)&p_Position, l_HalfExtents, l_Filter, &l_Poly, &l_Out);

          LOW_ASSERT(dtStatusSucceed(l_PolyResult),
                     "Failed finding nearest poly in detour");

          LOW_LOG_DEBUG << "MOVING AGENT " << p_AgentIndex << " TO "
                        << p_Position << LOW_LOG_END;
          dtStatus l_Status = g_Crowd->requestMoveTarget(p_AgentIndex, l_Poly,
                                                         (float *)&p_Position);

          if (dtStatusSucceed(l_Status)) {
            // Request successful
          } else {
            // Handle error or print status information
            printf("Move target request failed with status: %u\n", l_Status);
          }

          const dtCrowdAgent *i_NavAgent = g_Crowd->getAgent(p_AgentIndex);
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

          g_NavQuery = dtAllocNavMeshQuery();
          LOW_ASSERT(dtStatusSucceed(g_NavQuery->init(g_NavMesh, 2048)),
                     "Failed to initialize navmesh query");

          LOW_LOG_INFO << "Finished setting up recast" << LOW_LOG_END;
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          return;
          Math::Box l_Box;
          l_Box.halfExtents = g_HalfExtents;
          l_Box.position = g_Origin;
          l_Box.rotation = Math::QuaternionUtil::get_identity();

          DebugGeometry::render_box(l_Box, Math::Color(1.0f, 0.0f, 0.0f, 1.0f),
                                    true, true);

          LOW_PROFILE_CPU("Core", "Navmesh Tick");
          SYSTEM_ON_START(start);

          int a = g_NavMesh->getMaxTiles();

          for (int i = 0; i < g_NavMesh->getMaxTiles(); ++i) {
            const dtMeshTile *tile = g_NavMesh->getTile(i);

            // Skip empty tiles
            if (!tile || !tile->header) {
              continue;
            }

            // Iterate through all polygons in the tile
            for (int j = 0; j < tile->header->polyCount; ++j) {
              const dtPoly *poly = &tile->polys[j];

              // Iterate through the vertices of the polygon
              for (int k = 0; k < poly->vertCount; ++k) {
                const float *pos = &tile->verts[poly->verts[k] * 3];
                // printf("(%f, %f, %f) ", pos[0], pos[1], pos[2]);

                Math::Sphere i_Sphere;
                i_Sphere.radius = 0.06f;
                i_Sphere.position = *(Math::Vector3 *)pos;

                DebugGeometry::render_sphere(
                    i_Sphere, Math::Vector4(0.1f, 1.0f, 1.0f, 1.0f), false,
                    false);
              }
            }
          }

          if (p_State != Util::EngineState::PLAYING) {
            return;
          }

          Component::NavmeshAgent *l_Agents =
              Component::NavmeshAgent::living_instances();

          g_Crowd->update(p_Delta, nullptr);

          for (uint32_t i = 0u; i < Component::NavmeshAgent::living_count();
               ++i) {
            Component::NavmeshAgent i_Agent = l_Agents[i];
            Entity i_Entity = i_Agent.get_entity();
            Component::Transform i_Transform = i_Entity.get_transform();

            // If agent has not been initialized
            if (i_Agent.get_agent_index() < 0) {
              // TODO: read values from transform or component
              dtCrowdAgentParams i_Params;
              i_Params.radius = 0.2f;
              i_Params.height = 1.0f;
              i_Params.maxAcceleration = 1.0f;
              i_Params.maxSpeed = 2.0f;
              i_Params.collisionQueryRange = 0.1f;
              i_Params.pathOptimizationRange = 0.2f;

              int i_AgentIndex = System::Navmesh::add_agent(
                  i_Transform.get_world_position(), &i_Params);

              i_Agent.set_agent_index(i_AgentIndex);
            }

            const dtCrowdAgent *i_NavAgent =
                g_Crowd->getAgent(i_Agent.get_agent_index());

            Math::Vector3 i_TargetPos = *(Math::Vector3 *)i_NavAgent->targetPos;

            Math::Vector3 i_WorldPosition = *(Math::Vector3 *)i_NavAgent->npos;
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
