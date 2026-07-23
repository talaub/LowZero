#pragma once

#include "LowCoreApi.h"

#include "LowMath.h"
#include "LowUtilContainers.h"
#include "LowCoreNavigationAreaType.h"

#include <cstdint>

namespace Low {
  namespace Core {
    struct Scene;

    namespace Navigation {
      struct Source;
      struct World;

      struct BuildSettings
      {
        float cell_size = 0.3f;
        float cell_height = 0.2f;
        float agent_height = 2.0f;
        float agent_radius = 0.5f;
        float agent_max_climb = 0.4f;
        float agent_max_slope = 45.0f;
        int tile_size = 32;
      };

      using Bounds = Math::Bounds;

      struct TileCoord
      {
        int x = 0;
        int z = 0;

        bool operator==(const TileCoord &p_Other) const
        {
          return x == p_Other.x && z == p_Other.z;
        }

        bool operator!=(const TileCoord &p_Other) const
        {
          return !(*this == p_Other);
        }

        bool operator<(const TileCoord &p_Other) const
        {
          if (x != p_Other.x) {
            return x < p_Other.x;
          }
          return z < p_Other.z;
        }
      };

      struct TileRange
      {
        TileCoord minimum;
        TileCoord maximum;
      };

      enum class TileState : uint8_t
      {
        Empty,
        Dirty,
        Queued,
        Building,
        Ready,
        Failed
      };

      struct Tile
      {
        TileCoord coord;
        Bounds bounds;
        TileState state = TileState::Empty;
        uint64_t backend_tile_ref = 0u;
      };

      struct BuildGeometry
      {
        Low::Util::List<Math::Vector3> vertices;
        Low::Util::List<uint32_t> indices;
        Low::Util::List<AreaType> triangle_area_types;
        Bounds bounds;
      };

      struct NearestPointResult
      {
        Math::Vector3 position = Math::Vector3(0.0f);
      };

      struct PathResult
      {
        Low::Util::List<Math::Vector3> points;
        uint64_t navmesh_revision = 0ull;
        bool partial = false;
      };

      LOW_CORE_API bool
      collect_build_geometry(BuildGeometry *p_Geometry);

      LOW_CORE_API bool
      collect_build_geometry_for_bounds(const Bounds &p_Bounds,
                                        BuildGeometry *p_Geometry);

      LOW_CORE_API bool collect_build_geometry_for_world_bounds(
          World p_World, const Bounds &p_Bounds,
          BuildGeometry *p_Geometry);

      LOW_CORE_API bool refresh_source_bounds(Source p_Source);

      LOW_CORE_API bool get_source_bounds(Source p_Source,
                                          Bounds *p_Bounds);

      LOW_CORE_API bool mark_source_dirty(Source p_Source);

      LOW_CORE_API TileCoord
      world_to_tile_coord(Math::Vector3 p_Position,
                          float p_TileWorldSize);

      LOW_CORE_API Bounds
      tile_coord_to_bounds(TileCoord p_Coord,
                           float p_TileWorldSize, float p_MinY,
                           float p_MaxY);

      LOW_CORE_API TileRange
      tile_range_for_bounds(const Bounds &p_Bounds,
                            float p_TileWorldSize);

      LOW_CORE_API TileRange
      tile_range_for_radius(Math::Vector3 p_Position, float p_Radius,
                            float p_TileWorldSize);

      LOW_CORE_API bool bounds_overlap(const Bounds &p_A,
                                       const Bounds &p_B);

      LOW_CORE_API Bounds expand_bounds(const Bounds &p_Bounds,
                                        float p_Amount);

      LOW_CORE_API float
      get_tile_world_size(const BuildSettings &p_BuildSettings);

      LOW_CORE_API void clear_tile_registry(World p_World);

      LOW_CORE_API bool ensure_tile(World p_World, TileCoord p_Coord,
                                    float p_MinY, float p_MaxY,
                                    Tile *p_Tile = nullptr);

      LOW_CORE_API bool set_tile_state(World p_World,
                                       TileCoord p_Coord,
                                       TileState p_State);

      LOW_CORE_API bool get_tile(World p_World, TileCoord p_Coord,
                                 Tile *p_Tile);

      LOW_CORE_API void collect_tiles(World p_World,
                                      Low::Util::List<Tile> *p_Tiles);

      LOW_CORE_API void collect_dirty_tiles(
          World p_World, Low::Util::List<Tile> *p_Tiles);

      LOW_CORE_API bool remove_tile(World p_World,
                                    TileCoord p_Coord);

      LOW_CORE_API bool queue_tile(World p_World,
                                   TileCoord p_Coord);

      LOW_CORE_API uint32_t update_invoker_tiles(World p_World,
                                                 float p_MinY,
                                                 float p_MaxY);

      LOW_CORE_API uint32_t evict_tiles_outside_invokers(
          World p_World, uint32_t p_MaxTilesToEvict = 0u);

      LOW_CORE_API bool should_run_eviction(
          World p_World, uint32_t p_EvictionIntervalTicks);

      LOW_CORE_API uint32_t queue_dirty_tiles(
          World p_World, uint32_t p_MaxTilesToQueue = 0u);

      LOW_CORE_API uint32_t get_queued_tile_count(World p_World);

      LOW_CORE_API uint64_t get_navmesh_revision(World p_World);

      LOW_CORE_API uint32_t update_tile_builds(
          World p_World, uint32_t p_MaxTilesToBuild);

      LOW_CORE_API bool build_tile(World p_World,
                                   TileCoord p_Coord);

      LOW_CORE_API bool build_tile_from_geometry(
          World p_World, TileCoord p_Coord,
          const BuildGeometry &p_Geometry);

      LOW_CORE_API BuildSettings get_project_build_settings();

      LOW_CORE_API bool
      save_project_build_settings(const BuildSettings &p_BuildSettings);

      LOW_CORE_API void
      apply_build_settings(World p_World,
                           const BuildSettings &p_BuildSettings);

      LOW_CORE_API BuildSettings get_build_settings(World p_World);

      LOW_CORE_API void
      debug_render_build_geometry(const BuildGeometry &p_Geometry);

      LOW_CORE_API bool
      collect_navmesh_geometry(World p_World,
                               BuildGeometry *p_Geometry);

      LOW_CORE_API bool is_build_geometry_debug_rendering_enabled();

      LOW_CORE_API void
      set_build_geometry_debug_rendering_enabled(bool p_Enabled);

      LOW_CORE_API void
      debug_render_navmesh_geometry(const BuildGeometry &p_Geometry);

      LOW_CORE_API bool is_navmesh_debug_rendering_enabled();

      LOW_CORE_API void
      set_navmesh_debug_rendering_enabled(bool p_Enabled);

      LOW_CORE_API void debug_render_tile_registry(World p_World);

      LOW_CORE_API bool is_tile_debug_rendering_enabled();

      LOW_CORE_API void
      set_tile_debug_rendering_enabled(bool p_Enabled);

      LOW_CORE_API void debug_render_invokers(World p_World);

      LOW_CORE_API bool is_invoker_debug_rendering_enabled();

      LOW_CORE_API void
      set_invoker_debug_rendering_enabled(bool p_Enabled);

      LOW_CORE_API void render_debug_geometry();

      LOW_CORE_API bool
      collect_build_geometry_from_scene(Scene p_Scene,
                                        BuildGeometry *p_Geometry);
    } // namespace Navigation
  } // namespace Core
} // namespace Low
