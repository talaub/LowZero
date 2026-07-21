#include "LowCoreNavigationBackend.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <DetourAlloc.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
  constexpr unsigned char LOW_NAV_AREA_PREFERRED = 1u;
  constexpr unsigned char LOW_NAV_AREA_NORMAL = 2u;
  constexpr unsigned char LOW_NAV_AREA_ROUGH = 3u;
  constexpr unsigned char LOW_NAV_AREA_DIFFICULT = 4u;
  constexpr unsigned short LOW_NAV_POLYFLAG_WALK = 1u;
  constexpr int LOW_NAV_MAX_PATH_POLYS = 256;
  constexpr int LOW_NAV_MAX_STRAIGHT_PATH_POINTS = 256;

  static void copy_vector(const Low::Math::Vector3 &p_Vector,
                          float *p_Out)
  {
    p_Out[0] = p_Vector.x;
    p_Out[1] = p_Vector.y;
    p_Out[2] = p_Vector.z;
  }

  static Low::Math::Vector3 read_vector(const float *p_Vector)
  {
    return Low::Math::Vector3(p_Vector[0], p_Vector[1], p_Vector[2]);
  }

  static unsigned char
  get_recast_area(Low::Core::Navigation::AreaType p_AreaType)
  {
    switch (p_AreaType) {
    case Low::Core::Navigation::AreaType::PREFERRED:
      return LOW_NAV_AREA_PREFERRED;
    case Low::Core::Navigation::AreaType::NORMAL:
      return LOW_NAV_AREA_NORMAL;
    case Low::Core::Navigation::AreaType::ROUGH:
      return LOW_NAV_AREA_ROUGH;
    case Low::Core::Navigation::AreaType::DIFFICULT:
      return LOW_NAV_AREA_DIFFICULT;
    case Low::Core::Navigation::AreaType::BLOCKED:
      return RC_NULL_AREA;
    }

    return LOW_NAV_AREA_NORMAL;
  }

  static bool is_walkable_area(unsigned char p_Area)
  {
    return p_Area == LOW_NAV_AREA_PREFERRED ||
           p_Area == LOW_NAV_AREA_NORMAL ||
           p_Area == LOW_NAV_AREA_ROUGH ||
           p_Area == LOW_NAV_AREA_DIFFICULT;
  }

  static Low::Core::Navigation::AreaType
  get_navigation_area_type(unsigned char p_Area)
  {
    switch (p_Area) {
    case LOW_NAV_AREA_PREFERRED:
      return Low::Core::Navigation::AreaType::PREFERRED;
    case LOW_NAV_AREA_ROUGH:
      return Low::Core::Navigation::AreaType::ROUGH;
    case LOW_NAV_AREA_DIFFICULT:
      return Low::Core::Navigation::AreaType::DIFFICULT;
    case LOW_NAV_AREA_NORMAL:
    default:
      return Low::Core::Navigation::AreaType::NORMAL;
    }
  }

  static void configure_filter_costs(dtQueryFilter &p_Filter)
  {
    p_Filter.setAreaCost(LOW_NAV_AREA_PREFERRED, 0.6f);
    p_Filter.setAreaCost(LOW_NAV_AREA_NORMAL, 1.0f);
    p_Filter.setAreaCost(LOW_NAV_AREA_ROUGH, 2.5f);
    p_Filter.setAreaCost(LOW_NAV_AREA_DIFFICULT, 5.0f);
  }
} // namespace

namespace Low {
  namespace Core {
    namespace Navigation {
      struct WorldBackend
      {
        rcContext recast_context;
        dtNavMesh *navmesh = nullptr;
        dtNavMeshQuery *navmesh_query = nullptr;
        BuildSettings build_settings;
      };

      static void clear_navmesh(WorldBackend *p_World)
      {
        if (!p_World) {
          return;
        }

        if (p_World->navmesh_query) {
          dtFreeNavMeshQuery(p_World->navmesh_query);
          p_World->navmesh_query = nullptr;
        }

        if (p_World->navmesh) {
          dtFreeNavMesh(p_World->navmesh);
          p_World->navmesh = nullptr;
        }
      }

      WorldBackend *
      create_world_backend(const BuildSettings &p_BuildSettings)
      {
        WorldBackend *l_World = new WorldBackend();
        l_World->build_settings = p_BuildSettings;
        l_World->recast_context.enableLog(true);

        return l_World;
      }

      void destroy_world_backend(WorldBackend *p_World)
      {
        if (!p_World) {
          return;
        }

        clear_navmesh(p_World);
        delete p_World;
      }

      void
      set_world_build_settings(WorldBackend *p_World,
                               const BuildSettings &p_BuildSettings)
      {
        LOW_ASSERT(p_World,
                   "Cannot set build settings on null navmesh world");
        p_World->build_settings = p_BuildSettings;
      }

      const BuildSettings &
      get_world_build_settings(const WorldBackend *p_World)
      {
        LOW_ASSERT(
            p_World,
            "Cannot get build settings from null navmesh world");
        return p_World->build_settings;
      }

      bool
      build_navmesh_from_geometry(WorldBackend *p_World,
                                  const BuildGeometry &p_Geometry)
      {
        LOW_ASSERT(p_World,
                   "Cannot build navmesh in null navigation world");

        const int l_VertexCount =
            static_cast<int>(p_Geometry.vertices.size());
        const int l_IndexCount =
            static_cast<int>(p_Geometry.indices.size());
        if (l_VertexCount <= 0 || l_IndexCount <= 0 ||
            (l_IndexCount % 3) != 0) {
          LOW_LOG_WARN << "Invalid navigation build geometry"
                       << LOW_LOG_END;
          return false;
        }

        for (uint32_t i_Index : p_Geometry.indices) {
          if (i_Index >= p_Geometry.vertices.size()) {
            LOW_LOG_WARN << "Navigation build index out of bounds"
                         << LOW_LOG_END;
            return false;
          }
        }

        Low::Util::List<float> l_Vertices;
        l_Vertices.reserve(p_Geometry.vertices.size() * 3u);
        for (const Math::Vector3 &i_Vertex : p_Geometry.vertices) {
          l_Vertices.push_back(i_Vertex.x);
          l_Vertices.push_back(i_Vertex.y);
          l_Vertices.push_back(i_Vertex.z);
        }

        Low::Util::List<int> l_Indices;
        l_Indices.reserve(p_Geometry.indices.size());
        for (uint32_t i_Index : p_Geometry.indices) {
          l_Indices.push_back(static_cast<int>(i_Index));
        }

        const BuildSettings &l_Settings = p_World->build_settings;
        rcConfig l_Config;
        std::memset(&l_Config, 0, sizeof(l_Config));
        l_Config.cs = l_Settings.cell_size;
        l_Config.ch = l_Settings.cell_height;
        l_Config.walkableSlopeAngle = l_Settings.agent_max_slope;
        l_Config.walkableHeight = static_cast<int>(
            std::ceil(l_Settings.agent_height / l_Config.ch));
        l_Config.walkableClimb = static_cast<int>(
            std::floor(l_Settings.agent_max_climb / l_Config.ch));
        l_Config.walkableRadius = static_cast<int>(
            std::ceil(l_Settings.agent_radius / l_Config.cs));
        l_Config.maxEdgeLen = static_cast<int>(12.0f / l_Config.cs);
        l_Config.maxSimplificationError = 1.3f;
        l_Config.minRegionArea = rcSqr(8);
        l_Config.mergeRegionArea = rcSqr(20);
        l_Config.maxVertsPerPoly = 6;
        l_Config.detailSampleDist = l_Config.cs * 6.0f;
        l_Config.detailSampleMaxError = l_Config.ch;

        copy_vector(p_Geometry.bounds.minimum, l_Config.bmin);
        copy_vector(p_Geometry.bounds.maximum, l_Config.bmax);
        rcCalcGridSize(l_Config.bmin, l_Config.bmax, l_Config.cs,
                       &l_Config.width, &l_Config.height);

        if (l_Config.width <= 0 || l_Config.height <= 0) {
          LOW_LOG_WARN << "Invalid navigation build bounds"
                       << LOW_LOG_END;
          return false;
        }

        rcHeightfield *l_Heightfield = rcAllocHeightfield();
        rcCompactHeightfield *l_CompactHeightfield = nullptr;
        rcContourSet *l_ContourSet = nullptr;
        rcPolyMesh *l_PolyMesh = nullptr;
        rcPolyMeshDetail *l_DetailMesh = nullptr;
        unsigned char *l_TriangleAreas = nullptr;
        unsigned char *l_NavData = nullptr;
        int l_NavDataSize = 0;

        auto cleanup = [&]() {
          if (l_TriangleAreas) {
            delete[] l_TriangleAreas;
          }
          if (l_Heightfield) {
            rcFreeHeightField(l_Heightfield);
          }
          if (l_CompactHeightfield) {
            rcFreeCompactHeightfield(l_CompactHeightfield);
          }
          if (l_ContourSet) {
            rcFreeContourSet(l_ContourSet);
          }
          if (l_PolyMesh) {
            rcFreePolyMesh(l_PolyMesh);
          }
          if (l_DetailMesh) {
            rcFreePolyMeshDetail(l_DetailMesh);
          }
        };

        rcContext &l_Context = p_World->recast_context;
        const int l_TriangleCount = l_IndexCount / 3;
        if (!p_Geometry.triangle_area_types.empty() &&
            p_Geometry.triangle_area_types.size() !=
                static_cast<uint32_t>(l_TriangleCount)) {
          LOW_LOG_WARN << "Navigation build geometry area count does "
                          "not match triangle count"
                       << LOW_LOG_END;
          cleanup();
          return false;
        }

        if (!l_Heightfield ||
            !rcCreateHeightfield(&l_Context, *l_Heightfield,
                                 l_Config.width, l_Config.height,
                                 l_Config.bmin, l_Config.bmax,
                                 l_Config.cs, l_Config.ch)) {
          cleanup();
          return false;
        }

        l_TriangleAreas = new unsigned char[l_TriangleCount];
        std::memset(l_TriangleAreas, 0,
                    sizeof(unsigned char) * l_TriangleCount);
        rcMarkWalkableTriangles(
            &l_Context, l_Config.walkableSlopeAngle,
            l_Vertices.data(), l_VertexCount, l_Indices.data(),
            l_TriangleCount, l_TriangleAreas);
        for (int i = 0; i < l_TriangleCount; ++i) {
          const AreaType i_AreaType =
              p_Geometry.triangle_area_types.empty()
                  ? AreaType::NORMAL
                  : p_Geometry.triangle_area_types[i];
          if (i_AreaType == AreaType::BLOCKED ||
              l_TriangleAreas[i] == RC_WALKABLE_AREA) {
            l_TriangleAreas[i] = get_recast_area(i_AreaType);
          }
        }

        if (!rcRasterizeTriangles(
                &l_Context, l_Vertices.data(), l_VertexCount,
                l_Indices.data(), l_TriangleAreas, l_TriangleCount,
                *l_Heightfield, l_Config.walkableClimb)) {
          cleanup();
          return false;
        }

        rcFilterLowHangingWalkableObstacles(
            &l_Context, l_Config.walkableClimb, *l_Heightfield);
        rcFilterLedgeSpans(&l_Context, l_Config.walkableHeight,
                           l_Config.walkableClimb, *l_Heightfield);
        rcFilterWalkableLowHeightSpans(
            &l_Context, l_Config.walkableHeight, *l_Heightfield);

        l_CompactHeightfield = rcAllocCompactHeightfield();
        if (!l_CompactHeightfield ||
            !rcBuildCompactHeightfield(
                &l_Context, l_Config.walkableHeight,
                l_Config.walkableClimb, *l_Heightfield,
                *l_CompactHeightfield)) {
          cleanup();
          return false;
        }

        if (!rcErodeWalkableArea(&l_Context, l_Config.walkableRadius,
                                 *l_CompactHeightfield) ||
            !rcBuildDistanceField(&l_Context,
                                  *l_CompactHeightfield) ||
            !rcBuildRegions(&l_Context, *l_CompactHeightfield,
                            l_Config.borderSize,
                            l_Config.minRegionArea,
                            l_Config.mergeRegionArea)) {
          cleanup();
          return false;
        }

        l_ContourSet = rcAllocContourSet();
        if (!l_ContourSet ||
            !rcBuildContours(&l_Context, *l_CompactHeightfield,
                             l_Config.maxSimplificationError,
                             l_Config.maxEdgeLen, *l_ContourSet) ||
            l_ContourSet->nconts == 0) {
          cleanup();
          return false;
        }

        l_PolyMesh = rcAllocPolyMesh();
        if (!l_PolyMesh ||
            !rcBuildPolyMesh(&l_Context, *l_ContourSet,
                             l_Config.maxVertsPerPoly, *l_PolyMesh)) {
          cleanup();
          return false;
        }

        l_DetailMesh = rcAllocPolyMeshDetail();
        if (!l_DetailMesh ||
            !rcBuildPolyMeshDetail(
                &l_Context, *l_PolyMesh, *l_CompactHeightfield,
                l_Config.detailSampleDist,
                l_Config.detailSampleMaxError, *l_DetailMesh)) {
          cleanup();
          return false;
        }

        for (int i = 0; i < l_PolyMesh->npolys; ++i) {
          if (l_PolyMesh->areas[i] == RC_WALKABLE_AREA) {
            l_PolyMesh->areas[i] = LOW_NAV_AREA_NORMAL;
          }
          if (is_walkable_area(l_PolyMesh->areas[i])) {
            l_PolyMesh->flags[i] = LOW_NAV_POLYFLAG_WALK;
          }
        }

        dtNavMeshCreateParams l_Params;
        std::memset(&l_Params, 0, sizeof(l_Params));
        l_Params.verts = l_PolyMesh->verts;
        l_Params.vertCount = l_PolyMesh->nverts;
        l_Params.polys = l_PolyMesh->polys;
        l_Params.polyAreas = l_PolyMesh->areas;
        l_Params.polyFlags = l_PolyMesh->flags;
        l_Params.polyCount = l_PolyMesh->npolys;
        l_Params.nvp = l_PolyMesh->nvp;
        l_Params.detailMeshes = l_DetailMesh->meshes;
        l_Params.detailVerts = l_DetailMesh->verts;
        l_Params.detailVertsCount = l_DetailMesh->nverts;
        l_Params.detailTris = l_DetailMesh->tris;
        l_Params.detailTriCount = l_DetailMesh->ntris;
        l_Params.walkableHeight =
            static_cast<float>(l_Settings.agent_height);
        l_Params.walkableRadius =
            static_cast<float>(l_Settings.agent_radius);
        l_Params.walkableClimb =
            static_cast<float>(l_Settings.agent_max_climb);
        rcVcopy(l_Params.bmin, l_PolyMesh->bmin);
        rcVcopy(l_Params.bmax, l_PolyMesh->bmax);
        l_Params.cs = l_Config.cs;
        l_Params.ch = l_Config.ch;
        l_Params.buildBvTree = true;

        if (!dtCreateNavMeshData(&l_Params, &l_NavData,
                                 &l_NavDataSize)) {
          cleanup();
          return false;
        }

        clear_navmesh(p_World);
        p_World->navmesh = dtAllocNavMesh();
        p_World->navmesh_query = dtAllocNavMeshQuery();
        if (!p_World->navmesh || !p_World->navmesh_query) {
          if (l_NavData) {
            dtFree(l_NavData);
          }
          clear_navmesh(p_World);
          cleanup();
          return false;
        }

        if (!dtStatusSucceed(p_World->navmesh->init(
                l_NavData, l_NavDataSize, DT_TILE_FREE_DATA))) {
          dtFree(l_NavData);
          clear_navmesh(p_World);
          cleanup();
          return false;
        }
        l_NavData = nullptr;

        if (!dtStatusSucceed(p_World->navmesh_query->init(
                p_World->navmesh, 2048))) {
          clear_navmesh(p_World);
          cleanup();
          return false;
        }

        cleanup();
        return true;
      }

      bool collect_navmesh_geometry(WorldBackend *p_World,
                                    BuildGeometry *p_Geometry)
      {
        LOW_ASSERT(p_World,
                   "Cannot collect navmesh geometry from null "
                   "navigation world");

        if (!p_Geometry || !p_World->navmesh) {
          return false;
        }

        p_Geometry->vertices.clear();
        p_Geometry->indices.clear();
        p_Geometry->triangle_area_types.clear();
        p_Geometry->bounds = Bounds();

        const dtNavMesh *l_Navmesh = p_World->navmesh;
        for (int i_TileIndex = 0;
             i_TileIndex < l_Navmesh->getMaxTiles(); ++i_TileIndex) {
          const dtMeshTile *i_Tile = l_Navmesh->getTile(i_TileIndex);
          if (!i_Tile || !i_Tile->header || !i_Tile->polys ||
              !i_Tile->verts || !i_Tile->detailMeshes ||
              !i_Tile->detailTris) {
            continue;
          }

          for (int i_PolyIndex = 0;
               i_PolyIndex < i_Tile->header->polyCount;
               ++i_PolyIndex) {
            const dtPoly &i_Poly = i_Tile->polys[i_PolyIndex];
            if (i_Poly.getType() != DT_POLYTYPE_GROUND) {
              continue;
            }

            const dtPolyDetail &i_Detail =
                i_Tile->detailMeshes[i_PolyIndex];
            for (uint32_t i_TriangleIndex = 0u;
                 i_TriangleIndex < i_Detail.triCount;
                 ++i_TriangleIndex) {
              const unsigned char *i_Triangle =
                  &i_Tile->detailTris[(i_Detail.triBase +
                                       i_TriangleIndex) *
                                      4u];
              p_Geometry->triangle_area_types.push_back(
                  get_navigation_area_type(i_Poly.getArea()));

              for (uint32_t i_VertexIndex = 0u; i_VertexIndex < 3u;
                   ++i_VertexIndex) {
                Math::Vector3 i_Vertex;
                if (i_Triangle[i_VertexIndex] < i_Poly.vertCount) {
                  const unsigned short l_VertexIndex =
                      i_Poly.verts[i_Triangle[i_VertexIndex]];
                  i_Vertex =
                      read_vector(&i_Tile->verts[l_VertexIndex * 3u]);
                } else {
                  const uint32_t l_DetailVertexIndex =
                      i_Detail.vertBase +
                      (i_Triangle[i_VertexIndex] - i_Poly.vertCount);
                  i_Vertex = read_vector(
                      &i_Tile->detailVerts[l_DetailVertexIndex * 3u]);
                }

                p_Geometry->indices.push_back(static_cast<uint32_t>(
                    p_Geometry->vertices.size()));
                p_Geometry->vertices.push_back(i_Vertex);

                if (p_Geometry->vertices.size() == 1u) {
                  p_Geometry->bounds.minimum = i_Vertex;
                  p_Geometry->bounds.maximum = i_Vertex;
                } else {
                  p_Geometry->bounds.minimum.x = std::min(
                      p_Geometry->bounds.minimum.x, i_Vertex.x);
                  p_Geometry->bounds.minimum.y = std::min(
                      p_Geometry->bounds.minimum.y, i_Vertex.y);
                  p_Geometry->bounds.minimum.z = std::min(
                      p_Geometry->bounds.minimum.z, i_Vertex.z);
                  p_Geometry->bounds.maximum.x = std::max(
                      p_Geometry->bounds.maximum.x, i_Vertex.x);
                  p_Geometry->bounds.maximum.y = std::max(
                      p_Geometry->bounds.maximum.y, i_Vertex.y);
                  p_Geometry->bounds.maximum.z = std::max(
                      p_Geometry->bounds.maximum.z, i_Vertex.z);
                }
              }
            }
          }
        }

        return !p_Geometry->vertices.empty() &&
               !p_Geometry->indices.empty();
      }

      bool find_nearest_point(WorldBackend *p_World,
                              const Math::Vector3 &p_Position,
                              const Math::Vector3 &p_HalfExtents,
                              NearestPointResult &p_Result)
      {
        LOW_ASSERT(
            p_World,
            "Cannot query nearest point in null navigation world");
        if (!p_World->navmesh || !p_World->navmesh_query) {
          return false;
        }

        float l_Position[3];
        float l_HalfExtents[3];
        float l_NearestPoint[3];
        copy_vector(p_Position, l_Position);
        copy_vector(p_HalfExtents, l_HalfExtents);

        dtQueryFilter l_Filter;
        l_Filter.setIncludeFlags(LOW_NAV_POLYFLAG_WALK);
        l_Filter.setExcludeFlags(0);
        configure_filter_costs(l_Filter);

        dtPolyRef l_PolyRef = 0;
        const dtStatus l_Status =
            p_World->navmesh_query->findNearestPoly(
                l_Position, l_HalfExtents, &l_Filter, &l_PolyRef,
                l_NearestPoint);

        if (!dtStatusSucceed(l_Status) || l_PolyRef == 0) {
          return false;
        }

        p_Result.position = Math::Vector3(
            l_NearestPoint[0], l_NearestPoint[1], l_NearestPoint[2]);
        return true;
      }

      bool find_path(WorldBackend *p_World,
                     const Math::Vector3 &p_Start,
                     const Math::Vector3 &p_End,
                     const Math::Vector3 &p_HalfExtents,
                     PathResult &p_Result)
      {
        LOW_ASSERT(p_World,
                   "Cannot find path in null navigation world");
        p_Result.points.clear();
        p_Result.partial = false;

        if (!p_World->navmesh || !p_World->navmesh_query) {
          return false;
        }

        float l_Start[3];
        float l_End[3];
        float l_HalfExtents[3];
        float l_NearestStart[3];
        float l_NearestEnd[3];
        copy_vector(p_Start, l_Start);
        copy_vector(p_End, l_End);
        copy_vector(p_HalfExtents, l_HalfExtents);

        dtQueryFilter l_Filter;
        l_Filter.setIncludeFlags(LOW_NAV_POLYFLAG_WALK);
        l_Filter.setExcludeFlags(0);
        configure_filter_costs(l_Filter);

        dtPolyRef l_StartRef = 0;
        dtPolyRef l_EndRef = 0;
        dtStatus l_Status = p_World->navmesh_query->findNearestPoly(
            l_Start, l_HalfExtents, &l_Filter, &l_StartRef,
            l_NearestStart);
        if (!dtStatusSucceed(l_Status) || l_StartRef == 0) {
          return false;
        }

        l_Status = p_World->navmesh_query->findNearestPoly(
            l_End, l_HalfExtents, &l_Filter, &l_EndRef, l_NearestEnd);
        if (!dtStatusSucceed(l_Status) || l_EndRef == 0) {
          return false;
        }

        dtPolyRef l_Path[LOW_NAV_MAX_PATH_POLYS];
        int l_PathCount = 0;
        l_Status = p_World->navmesh_query->findPath(
            l_StartRef, l_EndRef, l_NearestStart, l_NearestEnd,
            &l_Filter, l_Path, &l_PathCount, LOW_NAV_MAX_PATH_POLYS);
        if (!dtStatusSucceed(l_Status) || l_PathCount <= 0) {
          return false;
        }

        p_Result.partial = l_Path[l_PathCount - 1] != l_EndRef;

        float l_PathEnd[3] = {l_NearestEnd[0], l_NearestEnd[1],
                              l_NearestEnd[2]};
        if (p_Result.partial) {
          const dtStatus l_ClosestStatus =
              p_World->navmesh_query->closestPointOnPoly(
                  l_Path[l_PathCount - 1], l_NearestEnd, l_PathEnd,
                  nullptr);
          if (!dtStatusSucceed(l_ClosestStatus)) {
            return false;
          }
        }

        float l_StraightPath[LOW_NAV_MAX_STRAIGHT_PATH_POINTS * 3];
        unsigned char
            l_StraightPathFlags[LOW_NAV_MAX_STRAIGHT_PATH_POINTS];
        dtPolyRef
            l_StraightPathRefs[LOW_NAV_MAX_STRAIGHT_PATH_POINTS];
        int l_StraightPathCount = 0;
        l_Status = p_World->navmesh_query->findStraightPath(
            l_NearestStart, l_PathEnd, l_Path, l_PathCount,
            l_StraightPath, l_StraightPathFlags, l_StraightPathRefs,
            &l_StraightPathCount, LOW_NAV_MAX_STRAIGHT_PATH_POINTS);
        if (!dtStatusSucceed(l_Status) || l_StraightPathCount <= 0) {
          return false;
        }

        p_Result.points.reserve(
            static_cast<uint32_t>(l_StraightPathCount));
        for (int i = 0; i < l_StraightPathCount; ++i) {
          const float *i_Point = &l_StraightPath[i * 3];
          p_Result.points.push_back(
              Math::Vector3(i_Point[0], i_Point[1], i_Point[2]));
        }

        return !p_Result.points.empty();
      }
    } // namespace Navigation
  } // namespace Core
} // namespace Low
