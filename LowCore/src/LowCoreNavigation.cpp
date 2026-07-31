#include "LowCoreNavigation.h"
#include "LowCore.h"

namespace Low {
  namespace Core {
    namespace Navigation {
      bool find_nearest_point(const Math::Vector3 &p_Position,
                              const Math::Vector3 &p_HalfExtents,
                              NearestPointResult *p_Result)
      {
        Scene l_Scene = get_loaded_scene();
        Navigation::World l_World = l_Scene.get_navigation_world();

        return l_World.find_nearest_point(p_Position, p_HalfExtents,
                                          p_Result);
      }

      bool find_path(const Math::Vector3 &p_Position,
                     const Math::Vector3 &p_End,
                     const Math::Vector3 &p_HalfExtents,
                     PathResult *p_Result)
      {
        Scene l_Scene = get_loaded_scene();
        Navigation::World l_World = l_Scene.get_navigation_world();

        return l_World.find_path(p_Position, p_End, p_HalfExtents,
                                 p_Result);
      }
    } // namespace Navigation
  } // namespace Core
} // namespace Low
