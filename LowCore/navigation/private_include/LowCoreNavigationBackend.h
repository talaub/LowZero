#pragma once

#include "LowCoreNavigation.h"

namespace Low {
  namespace Core {
    namespace Navigation {
      struct WorldBackend;

      WorldBackend *
      create_world_backend(const BuildSettings &p_BuildSettings);
      void destroy_world_backend(WorldBackend *p_World);

      void set_world_build_settings(
          WorldBackend *p_World,
          const BuildSettings &p_BuildSettings);
      const BuildSettings &
      get_world_build_settings(const WorldBackend *p_World);

      void clear_tile_registry(WorldBackend *p_World);
      bool ensure_tile(WorldBackend *p_World, TileCoord p_Coord,
                       float p_MinY, float p_MaxY,
                       Tile *p_Tile = nullptr);
      bool set_tile_state(WorldBackend *p_World, TileCoord p_Coord,
                          TileState p_State);
      bool get_tile(WorldBackend *p_World, TileCoord p_Coord,
                    Tile *p_Tile);
      void collect_tiles(WorldBackend *p_World,
                         Low::Util::List<Tile> *p_Tiles);
      void collect_dirty_tiles(WorldBackend *p_World,
                               Low::Util::List<Tile> *p_Tiles);
      bool remove_tile(WorldBackend *p_World, TileCoord p_Coord);
      bool queue_tile(WorldBackend *p_World, TileCoord p_Coord);
      uint32_t queue_dirty_tiles(WorldBackend *p_World,
                                 uint32_t p_MaxTilesToQueue = 0u);
      uint32_t get_queued_tile_count(WorldBackend *p_World);
      uint64_t get_navmesh_revision(WorldBackend *p_World);
      bool pop_next_queued_tile(WorldBackend *p_World,
                                TileCoord *p_Coord);

      bool build_navmesh_from_geometry(
          WorldBackend *p_World, const BuildGeometry &p_Geometry);
      bool build_navmesh_tile_from_geometry(
          WorldBackend *p_World, TileCoord p_Coord,
          const BuildGeometry &p_Geometry);

      bool collect_navmesh_geometry(WorldBackend *p_World,
                                    BuildGeometry *p_Geometry);

      bool find_nearest_point(
          WorldBackend *p_World, const Math::Vector3 &p_Position,
          const Math::Vector3 &p_HalfExtents,
          NearestPointResult &p_Result);

      bool find_path(WorldBackend *p_World,
                     const Math::Vector3 &p_Start,
                     const Math::Vector3 &p_End,
                     const Math::Vector3 &p_HalfExtents,
                     PathResult &p_Result);
    } // namespace Navigation
  }   // namespace Core
} // namespace Low
