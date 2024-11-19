#pragma once

#include "LowRendererMeshResource.h"
#include "LowRendererImageResource.h"

namespace Low {
  namespace Renderer {
    namespace ResourceManager {
      bool load_mesh_resource(MeshResource p_MeshResource);
      bool unload_mesh_resource(MeshResource p_MeshResource);

      bool load_image_resource(ImageResource p_ImageResource);

      void tick(float p_Delta);
    } // namespace ResourceManager
  }   // namespace Renderer
} // namespace Low
