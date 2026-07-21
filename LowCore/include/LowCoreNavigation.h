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

      struct Bounds
      {
        Math::Vector3 minimum = Math::Vector3(0.0f);
        Math::Vector3 maximum = Math::Vector3(0.0f);
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
        bool partial = false;
      };

      LOW_CORE_API bool
      collect_build_geometry(BuildGeometry *p_Geometry);

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

      LOW_CORE_API void render_debug_geometry();

      LOW_CORE_API bool
      collect_build_geometry_from_scene(Scene p_Scene,
                                        BuildGeometry *p_Geometry);
    } // namespace Navigation
  } // namespace Core
} // namespace Low
