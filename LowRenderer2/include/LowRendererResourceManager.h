#pragma once

#include "LowRendererMesh.h"
#include "LowRendererImageResource.h"

namespace Low {
  namespace Renderer {
    namespace ResourceManager {
      bool load_mesh(Mesh p_Mesh);
      bool upload_mesh(Mesh p_Mesh);
      // bool unload_mesh_resource(MeshResource p_MeshResource);

      bool load_texture(Texture p_Texture);

      void tick(float p_Delta);
    } // namespace ResourceManager
  }   // namespace Renderer
} // namespace Low
