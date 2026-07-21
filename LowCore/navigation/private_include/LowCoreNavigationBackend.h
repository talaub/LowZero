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

      bool build_navmesh_from_geometry(
          WorldBackend *p_World, const BuildGeometry &p_Geometry);

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
