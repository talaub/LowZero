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

constexpr unsigned char LOW_NAV_AREA_PREFERRED = 1u;
constexpr unsigned char LOW_NAV_AREA_NORMAL = 2u;
constexpr unsigned char LOW_NAV_AREA_ROUGH = 3u;
constexpr unsigned char LOW_NAV_AREA_DIFFICULT = 4u;
constexpr unsigned short LOW_NAV_POLYFLAG_WALK = 1u;
constexpr int LOW_NAV_MAX_PATH_POLYS = 256;
constexpr int LOW_NAV_MAX_STRAIGHT_PATH_POINTS = 256;
constexpr int LOW_NAV_DEFAULT_MAX_TILES = 4096;
constexpr int LOW_NAV_DEFAULT_MAX_POLYS_PER_TILE = 1024;

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
namespace Low {
  namespace Core {
    namespace Navigation {
      struct WorldBackend
      {
        rcContext recast_context;
        dtNavMesh *navmesh = nullptr;
        dtNavMeshQuery *navmesh_query = nullptr;
        BuildSettings build_settings;
        Low::Util::Map<TileCoord, Tile> tiles;
        Low::Util::List<TileCoord> build_queue;
        Low::Util::List<TileCoord> dirty_tiles;
        uint64_t navmesh_revision = 0ull;
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
        clear_navmesh(p_World);
        p_World->tiles.clear();
        p_World->build_queue.clear();
        p_World->dirty_tiles.clear();
        ++p_World->navmesh_revision;
      }

      const BuildSettings &
      get_world_build_settings(const WorldBackend *p_World)
      {
        LOW_ASSERT(
            p_World,
            "Cannot get build settings from null navmesh world");
        return p_World->build_settings;
      }

      void clear_tile_registry(WorldBackend *p_World)
      {
        LOW_ASSERT(p_World,
                   "Cannot clear tile registry on null navigation world");
        clear_navmesh(p_World);
        p_World->tiles.clear();
        p_World->build_queue.clear();
        p_World->dirty_tiles.clear();
        ++p_World->navmesh_revision;
      }

      static bool is_tile_queued(WorldBackend *p_World,
                                 TileCoord p_Coord)
      {
        for (const TileCoord &i_Coord : p_World->build_queue) {
          if (i_Coord == p_Coord) {
            return true;
          }
        }
        return false;
      }

      static bool is_tile_dirty_tracked(WorldBackend *p_World,
                                        TileCoord p_Coord)
      {
        for (const TileCoord &i_Coord : p_World->dirty_tiles) {
          if (i_Coord == p_Coord) {
            return true;
          }
        }
        return false;
      }

      static void remove_tile_from_queue(WorldBackend *p_World,
                                         TileCoord p_Coord)
      {
        for (auto it = p_World->build_queue.begin();
             it != p_World->build_queue.end();) {
          if (*it == p_Coord) {
            it = p_World->build_queue.erase(it);
          } else {
            ++it;
          }
        }
      }

      static void remove_tile_from_dirty_list(WorldBackend *p_World,
                                              TileCoord p_Coord)
      {
        for (auto it = p_World->dirty_tiles.begin();
             it != p_World->dirty_tiles.end();) {
          if (*it == p_Coord) {
            it = p_World->dirty_tiles.erase(it);
          } else {
            ++it;
          }
        }
      }

      static void add_tile_to_dirty_list(WorldBackend *p_World,
                                         TileCoord p_Coord)
      {
        if (!is_tile_dirty_tracked(p_World, p_Coord)) {
          p_World->dirty_tiles.push_back(p_Coord);
        }
      }

      bool ensure_tile(WorldBackend *p_World, TileCoord p_Coord,
                       float p_MinY, float p_MaxY, Tile *p_Tile)
      {
        LOW_ASSERT(p_World,
                   "Cannot ensure tile on null navigation world");

        auto i_It = p_World->tiles.find(p_Coord);
        if (i_It == p_World->tiles.end()) {
          Tile l_Tile;
          l_Tile.coord = p_Coord;
          l_Tile.bounds = tile_coord_to_bounds(
              p_Coord, get_tile_world_size(p_World->build_settings),
              p_MinY, p_MaxY);
          l_Tile.state = TileState::Empty;
          i_It = p_World->tiles.insert(eastl::make_pair(
                                           p_Coord, l_Tile))
                     .first;
        }

        if (p_Tile) {
          *p_Tile = i_It->second;
        }
        return true;
      }

      bool set_tile_state(WorldBackend *p_World, TileCoord p_Coord,
                          TileState p_State)
      {
        LOW_ASSERT(p_World,
                   "Cannot set tile state on null navigation world");

        auto i_It = p_World->tiles.find(p_Coord);
        if (i_It == p_World->tiles.end()) {
          return false;
        }

        i_It->second.state = p_State;
        if (p_State == TileState::Dirty) {
          add_tile_to_dirty_list(p_World, p_Coord);
        } else {
          remove_tile_from_dirty_list(p_World, p_Coord);
        }
        return true;
      }

      bool get_tile(WorldBackend *p_World, TileCoord p_Coord,
                    Tile *p_Tile)
      {
        LOW_ASSERT(p_World,
                   "Cannot get tile from null navigation world");
        if (!p_Tile) {
          return false;
        }

        auto i_It = p_World->tiles.find(p_Coord);
        if (i_It == p_World->tiles.end()) {
          return false;
        }

        *p_Tile = i_It->second;
        return true;
      }

      void collect_tiles(WorldBackend *p_World,
                         Low::Util::List<Tile> *p_Tiles)
      {
        LOW_ASSERT(p_World,
                   "Cannot collect tiles from null navigation world");
        if (!p_Tiles) {
          return;
        }

        p_Tiles->clear();
        p_Tiles->reserve(static_cast<uint32_t>(p_World->tiles.size()));
        for (const auto &i_Entry : p_World->tiles) {
          p_Tiles->push_back(i_Entry.second);
        }
      }

      void collect_dirty_tiles(WorldBackend *p_World,
                               Low::Util::List<Tile> *p_Tiles)
      {
        LOW_ASSERT(p_World,
                   "Cannot collect dirty tiles from null navigation world");
        if (!p_Tiles) {
          return;
        }

        p_Tiles->clear();
        p_Tiles->reserve(
            static_cast<uint32_t>(p_World->dirty_tiles.size()));
        for (const TileCoord &i_Coord : p_World->dirty_tiles) {
          auto i_It = p_World->tiles.find(i_Coord);
          if (i_It == p_World->tiles.end() ||
              i_It->second.state != TileState::Dirty) {
            continue;
          }
          p_Tiles->push_back(i_It->second);
        }
      }

      bool remove_tile(WorldBackend *p_World, TileCoord p_Coord)
      {
        LOW_ASSERT(p_World,
                   "Cannot remove tile from null navigation world");

        auto i_It = p_World->tiles.find(p_Coord);
        if (i_It == p_World->tiles.end()) {
          remove_tile_from_queue(p_World, p_Coord);
          remove_tile_from_dirty_list(p_World, p_Coord);
          return false;
        }

        if (p_World->navmesh &&
            i_It->second.backend_tile_ref != 0u) {
          unsigned char *l_RemovedData = nullptr;
          int l_RemovedDataSize = 0;
          p_World->navmesh->removeTile(
              static_cast<dtTileRef>(
                  i_It->second.backend_tile_ref),
              &l_RemovedData, &l_RemovedDataSize);
          if (l_RemovedData) {
            dtFree(l_RemovedData);
          }
        }

        remove_tile_from_queue(p_World, p_Coord);
        remove_tile_from_dirty_list(p_World, p_Coord);
        p_World->tiles.erase(i_It);
        ++p_World->navmesh_revision;
        return true;
      }

      bool queue_tile(WorldBackend *p_World, TileCoord p_Coord)
      {
        LOW_ASSERT(p_World,
                   "Cannot queue tile on null navigation world");

        auto i_It = p_World->tiles.find(p_Coord);
        if (i_It == p_World->tiles.end() ||
            is_tile_queued(p_World, p_Coord)) {
          return false;
        }

        i_It->second.state = TileState::Queued;
        remove_tile_from_dirty_list(p_World, p_Coord);
        p_World->build_queue.push_back(p_Coord);
        return true;
      }

      uint32_t queue_dirty_tiles(WorldBackend *p_World,
                                 uint32_t p_MaxTilesToQueue)
      {
        LOW_ASSERT(p_World,
                   "Cannot queue dirty tiles on null navigation world");

        uint32_t l_QueuedCount = 0u;
        Low::Util::List<TileCoord> l_DirtyTiles =
            p_World->dirty_tiles;
        for (const TileCoord &i_Coord : l_DirtyTiles) {
          if (p_MaxTilesToQueue > 0u &&
              l_QueuedCount >= p_MaxTilesToQueue) {
            break;
          }

          auto i_It = p_World->tiles.find(i_Coord);
          if (i_It == p_World->tiles.end() ||
              i_It->second.state != TileState::Dirty ||
              is_tile_queued(p_World, i_Coord)) {
            remove_tile_from_dirty_list(p_World, i_Coord);
            continue;
          }

          if (queue_tile(p_World, i_Coord)) {
            ++l_QueuedCount;
          }
        }
        return l_QueuedCount;
      }

      uint32_t get_queued_tile_count(WorldBackend *p_World)
      {
        LOW_ASSERT(p_World,
                   "Cannot count queued tiles on null navigation world");

        return static_cast<uint32_t>(p_World->build_queue.size());
      }

      uint64_t get_navmesh_revision(WorldBackend *p_World)
      {
        LOW_ASSERT(
            p_World,
            "Cannot get navmesh revision from null navigation world");

        return p_World->navmesh_revision;
      }

      bool pop_next_queued_tile(WorldBackend *p_World,
                                TileCoord *p_Coord)
      {
        LOW_ASSERT(p_World,
                   "Cannot pop queued tile from null navigation world");
        if (!p_Coord || p_World->build_queue.empty()) {
          return false;
        }

        *p_Coord = p_World->build_queue.front();
        p_World->build_queue.erase(p_World->build_queue.begin());
        return true;
      }

      static bool ensure_tiled_navmesh(WorldBackend *p_World)
      {
        LOW_ASSERT(p_World,
                   "Cannot initialize null navigation world");

        if (p_World->navmesh && p_World->navmesh_query) {
          return true;
        }

        clear_navmesh(p_World);

        p_World->navmesh = dtAllocNavMesh();
        p_World->navmesh_query = dtAllocNavMeshQuery();
        if (!p_World->navmesh || !p_World->navmesh_query) {
          clear_navmesh(p_World);
          return false;
        }

        dtNavMeshParams l_Params;
        std::memset(&l_Params, 0, sizeof(l_Params));
        l_Params.orig[0] = 0.0f;
        l_Params.orig[1] = 0.0f;
        l_Params.orig[2] = 0.0f;
        const float l_TileWorldSize =
            get_tile_world_size(p_World->build_settings);
        l_Params.tileWidth = l_TileWorldSize;
        l_Params.tileHeight = l_TileWorldSize;
        l_Params.maxTiles = LOW_NAV_DEFAULT_MAX_TILES;
        l_Params.maxPolys = LOW_NAV_DEFAULT_MAX_POLYS_PER_TILE;

        if (!dtStatusSucceed(p_World->navmesh->init(&l_Params)) ||
            !dtStatusSucceed(p_World->navmesh_query->init(
                p_World->navmesh, 2048))) {
          clear_navmesh(p_World);
          return false;
        }

        return true;
      }

      static bool build_tile_data_from_geometry(
          WorldBackend *p_World, TileCoord p_Coord,
          const BuildGeometry &p_Geometry, unsigned char **p_NavData,
          int *p_NavDataSize)
      {
        LOW_ASSERT(p_World,
                   "Cannot build navmesh in null navigation world");
        LOW_ASSERT(p_NavData && p_NavDataSize,
                   "Cannot write tile navmesh data to null output");

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
        l_Config.tileSize = l_Settings.tile_size;
        l_Config.borderSize = l_Config.walkableRadius + 3;
        l_Config.width =
            l_Config.tileSize + l_Config.borderSize * 2;
        l_Config.height =
            l_Config.tileSize + l_Config.borderSize * 2;

        copy_vector(p_Geometry.bounds.min, l_Config.bmin);
        copy_vector(p_Geometry.bounds.max, l_Config.bmax);

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
        l_Params.tileX = p_Coord.x;
        l_Params.tileY = p_Coord.z;
        l_Params.tileLayer = 0;

        if (!dtCreateNavMeshData(&l_Params, &l_NavData,
                                 &l_NavDataSize)) {
          cleanup();
          return false;
        }

        *p_NavData = l_NavData;
        *p_NavDataSize = l_NavDataSize;
        l_NavData = nullptr;

        cleanup();
        return true;
      }

      bool build_navmesh_tile_from_geometry(
          WorldBackend *p_World, TileCoord p_Coord,
          const BuildGeometry &p_Geometry)
      {
        LOW_ASSERT(p_World,
                   "Cannot build navmesh tile in null navigation world");

        auto i_It = p_World->tiles.find(p_Coord);
        if (i_It == p_World->tiles.end()) {
          return false;
        }

        i_It->second.state = TileState::Building;
        remove_tile_from_dirty_list(p_World, p_Coord);

        if (!ensure_tiled_navmesh(p_World)) {
          i_It->second.state = TileState::Failed;
          remove_tile_from_dirty_list(p_World, p_Coord);
          return false;
        }

        if (i_It->second.backend_tile_ref != 0u) {
          unsigned char *l_RemovedData = nullptr;
          int l_RemovedDataSize = 0;
          p_World->navmesh->removeTile(
              static_cast<dtTileRef>(
                  i_It->second.backend_tile_ref),
              &l_RemovedData, &l_RemovedDataSize);
          if (l_RemovedData) {
            dtFree(l_RemovedData);
          }
          i_It->second.backend_tile_ref = 0u;
        }

        unsigned char *l_NavData = nullptr;
        int l_NavDataSize = 0;
        if (!build_tile_data_from_geometry(
                p_World, p_Coord, p_Geometry, &l_NavData,
                &l_NavDataSize)) {
          i_It->second.state = TileState::Failed;
          remove_tile_from_dirty_list(p_World, p_Coord);
          return false;
        }

        dtTileRef l_TileRef = 0;
        if (!dtStatusSucceed(p_World->navmesh->addTile(
                l_NavData, l_NavDataSize, DT_TILE_FREE_DATA, 0,
                &l_TileRef))) {
          dtFree(l_NavData);
          i_It->second.state = TileState::Failed;
          remove_tile_from_dirty_list(p_World, p_Coord);
          return false;
        }

        i_It->second.backend_tile_ref =
            static_cast<uint64_t>(l_TileRef);
        i_It->second.state = TileState::Ready;
        remove_tile_from_dirty_list(p_World, p_Coord);
        ++p_World->navmesh_revision;
        return true;
      }

      bool
      build_navmesh_from_geometry(WorldBackend *p_World,
                                  const BuildGeometry &p_Geometry)
      {
        LOW_ASSERT(p_World,
                   "Cannot build navmesh in null navigation world");

        clear_navmesh(p_World);
        p_World->tiles.clear();
        p_World->build_queue.clear();
        p_World->dirty_tiles.clear();
        ++p_World->navmesh_revision;

        if (p_Geometry.vertices.empty() ||
            p_Geometry.indices.empty()) {
          LOW_LOG_WARN << "Invalid navigation build geometry"
                       << LOW_LOG_END;
          return false;
        }

        const float l_TileWorldSize =
            get_tile_world_size(p_World->build_settings);
        const int l_WalkableRadius = static_cast<int>(std::ceil(
            p_World->build_settings.agent_radius /
            p_World->build_settings.cell_size));
        const float l_BorderPadding =
            static_cast<float>(l_WalkableRadius + 3) *
            p_World->build_settings.cell_size;
        const TileRange l_Range =
            tile_range_for_bounds(p_Geometry.bounds,
                                  l_TileWorldSize);

        bool l_BuiltAnyTile = false;
        for (int x = l_Range.minimum.x; x <= l_Range.maximum.x;
             ++x) {
          for (int z = l_Range.minimum.z; z <= l_Range.maximum.z;
               ++z) {
            const TileCoord i_Coord{x, z};
            Tile i_Tile;
            ensure_tile(p_World, i_Coord, p_Geometry.bounds.min.y,
                        p_Geometry.bounds.max.y, &i_Tile);

            BuildGeometry i_TileGeometry = p_Geometry;
            i_TileGeometry.bounds =
                expand_bounds(i_Tile.bounds, l_BorderPadding);
            if (build_navmesh_tile_from_geometry(
                    p_World, i_Coord, i_TileGeometry)) {
              l_BuiltAnyTile = true;
            }
          }
        }

        return l_BuiltAnyTile;
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
                  p_Geometry->bounds.min = i_Vertex;
                  p_Geometry->bounds.max = i_Vertex;
                } else {
                  p_Geometry->bounds.min.x = std::min(
                      p_Geometry->bounds.min.x, i_Vertex.x);
                  p_Geometry->bounds.min.y = std::min(
                      p_Geometry->bounds.min.y, i_Vertex.y);
                  p_Geometry->bounds.min.z = std::min(
                      p_Geometry->bounds.min.z, i_Vertex.z);
                  p_Geometry->bounds.max.x = std::max(
                      p_Geometry->bounds.max.x, i_Vertex.x);
                  p_Geometry->bounds.max.y = std::max(
                      p_Geometry->bounds.max.y, i_Vertex.y);
                  p_Geometry->bounds.max.z = std::max(
                      p_Geometry->bounds.max.z, i_Vertex.z);
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
        p_Result.navmesh_revision = get_navmesh_revision(p_World);
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
