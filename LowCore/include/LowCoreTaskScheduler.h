#pragma once

namespace Low {
  namespace Core {
    struct MeshResource;

    namespace TaskScheduler {
      void schedule_mesh_resource_load(MeshResource p_MeshResource);

      void tick();
    } // namespace TaskScheduler
  }   // namespace Core
} // namespace Low
